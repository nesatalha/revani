#include "bridge.h"

#include <errno.h>
#include <fcntl.h>
#include <lvgl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "protocol.h"
#include "ui.h"
#include "ui_proto.h"

#define STATUS_PERIOD_MS 10000
#define FAKE_BATTERY 87  // simülatörün "pili"
#define BUF_SIZE (2 + 65535)

static int listen_fd = -1;
static int client_fd = -1;
static uint8_t buf[BUF_SIZE];
static size_t buf_len;
static uint32_t last_status;
static unsigned frames, errors;

static void set_nonblocking(int fd) { fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK); }

bool bridge_start(uint16_t port) {
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) return false;
  int one = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(port),
                             .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(listen_fd, 1) < 0) {
    perror("köprü");
    close(listen_fd);
    listen_fd = -1;
    return false;
  }
  set_nonblocking(listen_fd);
  ui_set_battery(FAKE_BATTERY);
  printf("Köprü 127.0.0.1:%u dinliyor (telefonda: adb reverse tcp:%u tcp:%u)\n", port, port, port);
  return true;
}

static void send_frame(const uint8_t *msg, size_t len) {
  uint8_t out[2 + 16];
  out[0] = (uint8_t)(len & 0xFF);
  out[1] = (uint8_t)(len >> 8);
  memcpy(&out[2], msg, len);
  if (send(client_fd, out, 2 + len, 0) < 0) perror("köprü gönderme");
}

static void send_status(void) {
  uint8_t msg[3];
  size_t len = proto_encode_status(msg, FAKE_BATTERY, ui_proto_screen());
  send_frame(msg, len);
  last_status = lv_tick_get();
}

static void drop_client(void) {
  close(client_fd);
  client_fd = -1;
  buf_len = 0;
  printf("Telefon ayrıldı (%u çerçeve, %u hata)\n", frames, errors);
  ui_set_connected(false);  // karttaki gibi: navigasyon biter, bekleme ekranı
}

static void handle_frames(void) {
  size_t pos = 0;
  while (buf_len - pos >= 2) {
    size_t len = buf[pos] | (buf[pos + 1] << 8);
    if (buf_len - pos - 2 < len) break;
    proto_result_t r = ui_proto_handle(&buf[pos + 2], len);
    frames++;
    if (r != PROTO_OK) {
      errors++;
      fprintf(stderr, "Mesaj 0x%02x (%zu bayt): %s\n", len ? buf[pos + 2] : 0, len, proto_result_str(r));
    }
    pos += 2 + len;
  }
  memmove(buf, &buf[pos], buf_len - pos);
  buf_len -= pos;
}

void bridge_poll(void) {
  if (listen_fd < 0) return;

  if (client_fd < 0) {
    client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) return;
    set_nonblocking(client_fd);
    int one = 1;
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    frames = errors = 0;
    printf("Telefon bağlandı\n");
    ui_set_connected(true);
    send_status();  // telefon bağlantıyı ilk STATUS ile hazır sayar
  }

  for (;;) {
    ssize_t n = recv(client_fd, buf + buf_len, sizeof buf - buf_len, 0);
    if (n > 0) {
      buf_len += (size_t)n;
      handle_frames();
      if (buf_len == sizeof buf) {  // tek çerçeve tamponu aşamaz; olursa akış bozuk
        fprintf(stderr, "Köprü akışı bozuk, bağlantı kesiliyor\n");
        drop_client();
        return;
      }
      continue;
    }
    if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
      drop_client();
      return;
    }
    break;
  }

  if (lv_tick_elaps(last_status) > STATUS_PERIOD_MS) send_status();
}
