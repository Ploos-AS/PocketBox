#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSZ 2048
#define MAX_TERMINALS 12
struct terminal { int fd; unsigned inputs; char cursor[128]; char output[8192]; size_t out_len, out_pos; };
static const char *terminal_menu = "\r\n*** POCKETBOX M1 ***\r\n[F] Files  [N] Next page  [Q] Quit\r\nChoice: ";
static volatile sig_atomic_t running = 1;
static void on_signal(int sig) { (void)sig; running = 0; }
static int listener(unsigned port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0), opt = 1;
  struct sockaddr_in a;
  if (fd < 0) return -1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET; a.sin_port = htons((unsigned short)port);
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(fd, (struct sockaddr *)&a, sizeof a) || listen(fd, 8)) { close(fd); return -1; }
  return fd;
}
#include "catalog.h"
#include "http_async.h"
static void terminal_flush(struct terminal *c) {
  if (c->out_pos == c->out_len) { c->out_pos = c->out_len = 0; return; }
  ssize_t n = send(c->fd, c->output + c->out_pos, c->out_len - c->out_pos,
                   MSG_DONTWAIT | MSG_NOSIGNAL);
  if (n > 0) c->out_pos += (size_t)n;
  else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
    close(c->fd); c->fd = -1; return;
  }
  if (c->out_pos == c->out_len) c->out_pos = c->out_len = 0;
}
static void terminal_text(struct terminal *c, const char *s) {
  size_t n = strlen(s);
  if (c->out_pos) {
    memmove(c->output, c->output + c->out_pos, c->out_len - c->out_pos);
    c->out_len -= c->out_pos; c->out_pos = 0;
  }
  if (n > sizeof c->output - c->out_len) { close(c->fd); c->fd = -1; return; }
  memcpy(c->output + c->out_len, s, n); c->out_len += n;
}
/* Terminal sockets are nonblocking and processed by poll(), not recv loops. */
static void terminal_input(struct terminal *c, int root) {
  char input[64];
  ssize_t n = recv(c->fd, input, sizeof input, 0);
  if (n <= 0) { close(c->fd); c->fd = -1; return; }
  for (ssize_t i = 0; i < n; ++i) {
    char ch = input[i];
    if (ch == 'q' || ch == 'Q' || ++c->inputs > 256) {
      const char *bye = "\r\nGoodbye.\r\n";
      send(c->fd, bye, strlen(bye), MSG_DONTWAIT | MSG_NOSIGNAL);
      close(c->fd); c->fd = -1; return;
    }
    if (ch == 'f' || ch == 'F' || ch == 'n' || ch == 'N') {
      if (ch == 'f' || ch == 'F') c->cursor[0] = 0;
      struct catalog cat;
      catalog_load_page(root, &cat, c->cursor);
      const char *header = "\r\nPocketBox files:\r\n";
      terminal_text(c, header);
      for (unsigned k = 0; c->fd >= 0 && k < cat.count; ++k) {
        terminal_text(c, cat.names[k]);
        terminal_text(c, "\r\n");
      }
      if (c->fd < 0) return;
      if (cat.count) snprintf(c->cursor, sizeof c->cursor, "%s",
                              cat.names[cat.count - 1]);
      const char *footer = cat.has_more
          ? "[N] Next page  [F] First page  [Q] Quit\r\nChoice: "
          : "[F] First page  [Q] Quit\r\nChoice: ";
      terminal_text(c, footer);
    }
  }
}
int main(int argc, char **argv) {
  const char *dir = "."; unsigned http = 8080, telnet = 2323;
  for (int i = 1; i < argc; ++i) {
    if (i + 1 >= argc) { fprintf(stderr, "Missing value for %s\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--root")) dir = argv[++i];
    else if (!strcmp(argv[i], "--http-port")) http = (unsigned)atoi(argv[++i]);
    else if (!strcmp(argv[i], "--telnet-port")) telnet = (unsigned)atoi(argv[++i]);
    else { fprintf(stderr, "Unknown option: %s\n", argv[i]); return 2; }
  }
  if (!http || http > 65535 || !telnet || telnet > 65535 || http == telnet) return 2;
  int root = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (root < 0) { perror("root"); return 1; }
  int h = listener(http), t = listener(telnet);
  if (h < 0 || t < 0) { perror("listen"); if (h >= 0) close(h); if (t >= 0) close(t); close(root); return 1; }
  signal(SIGINT, on_signal); signal(SIGTERM, on_signal);
  signal(SIGPIPE, SIG_IGN);
  fprintf(stderr, "PocketBox M0 listening on 127.0.0.1 HTTP:%u Telnet:%u\n", http, telnet);
  /* Both HTTP and terminal sockets are bounded and polled. */ 
  struct http_conn web[MAX_HTTP];
  for (unsigned i = 0; i < MAX_HTTP; ++i) { web[i].fd = -1; web[i].file = -1; }
  struct terminal clients[MAX_TERMINALS];
  for (unsigned i = 0; i < MAX_TERMINALS; ++i) clients[i].fd = -1;
  while (running) {
    struct pollfd fds[2 + MAX_TERMINALS + MAX_HTTP];
    nfds_t count = 2;
    fds[0] = (struct pollfd){h, POLLIN, 0};
    fds[1] = (struct pollfd){t, POLLIN, 0};
    for (unsigned i = 0; i < MAX_TERMINALS; ++i)
      if (clients[i].fd >= 0) fds[count++] = (struct pollfd){clients[i].fd, (short)(POLLIN | (clients[i].out_len > clients[i].out_pos ? POLLOUT : 0)), 0};
    for (unsigned i = 0; i < MAX_HTTP; ++i)
      if (web[i].fd >= 0) fds[count++] = (struct pollfd){web[i].fd,
          web[i].state == 0 ? POLLIN : POLLOUT, 0};
    int ready = poll(fds, count, 1000);
    if (ready < 0) { if (errno == EINTR) continue; perror("poll"); break; }
    if (!ready) continue;
    /* Process existing terminals before accepting more work. */
    for (nfds_t i = 2; i < count; ++i) {
      if (!fds[i].revents) continue;
      for (unsigned j = 0; j < MAX_TERMINALS; ++j)
        if (clients[j].fd == fds[i].fd) {
          if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            close(clients[j].fd); clients[j].fd = -1;
          } else { if (fds[i].revents & POLLIN) terminal_input(&clients[j], root); if (clients[j].fd >= 0 && (fds[i].revents & POLLOUT)) terminal_flush(&clients[j]); }
          break;
        }
    }
    for (nfds_t i = 2; i < count; ++i) {
      if (!fds[i].revents) continue;
      for (unsigned j = 0; j < MAX_HTTP; ++j) {
        if (web[j].fd != fds[i].fd) continue;
        http_step(&web[j], fds[i].revents, root);
        if (web[j].state == 3) http_close(&web[j]);
        break;
      }
    }
    time_t now = time(NULL);
    for (unsigned i = 0; i < MAX_HTTP; ++i)
      if (web[i].fd >= 0 && now - web[i].last > HTTP_IDLE_SECONDS)
        http_close(&web[i]);
    if (fds[0].revents & POLLIN) {
      int fd = accept(h, NULL, NULL);
      if (fd >= 0) {
        unsigned slot = MAX_HTTP;
        for (unsigned i = 0; i < MAX_HTTP; ++i)
          if (web[i].fd < 0) { slot = i; break; }
        if (slot == MAX_HTTP || fcntl(fd, F_SETFL, O_NONBLOCK) < 0) close(fd);
        else {
          memset(&web[slot], 0, sizeof web[slot]);
          web[slot].fd = fd; web[slot].file = -1; web[slot].last = time(NULL);
        }
      }
    }
    if (fds[1].revents & POLLIN) {
      int fd = accept(t, NULL, NULL);
      if (fd >= 0) {
        unsigned slot = MAX_TERMINALS;
        for (unsigned i = 0; i < MAX_TERMINALS; ++i)
          if (clients[i].fd < 0) { slot = i; break; }
        if (slot == MAX_TERMINALS || fcntl(fd, F_SETFL, O_NONBLOCK) < 0) close(fd);
        else {
          clients[slot] = (struct terminal){.fd = fd};
          terminal_text(&clients[slot], terminal_menu);
        }
      }
    }
  }
  for (unsigned i = 0; i < MAX_TERMINALS; ++i)
    if (clients[i].fd >= 0) close(clients[i].fd);
  for (unsigned i = 0; i < MAX_HTTP; ++i) if (web[i].fd >= 0) http_close(&web[i]);
  close(h); close(t); close(root); return 0;
}
