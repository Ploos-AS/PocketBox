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
struct terminal { int fd; unsigned inputs; };
static const char *terminal_menu = "\r\n*** POCKETBOX M1 ***\r\n[F] Files (HTTP: /files/welcome.txt)\r\n[Q] Quit\r\nChoice: ";
static volatile sig_atomic_t running = 1;
static void on_signal(int sig) { (void)sig; running = 0; }
static int send_all(int fd, const char *s, size_t len) {
  while (len) { ssize_t n = send(fd, s, len, 0); if (n <= 0) return -1; s += n; len -= (size_t)n; }
  return 0;
}
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
static void http_client(int fd, int root) {
  char req[BUFSZ], path[BUFSZ], buf[BUFSZ], header[256];
  ssize_t n = recv(fd, req, sizeof req - 1, 0);
  if (n <= 0) return;
  req[n] = 0;
  char *end = strstr(req, "\r
");
  if (!end) { send_all(fd, "HTTP/1.0 400 Bad Request\r
Content-Length: 0\r
\r
", 47); return; }
  *end = 0;
  if (sscanf(req, "GET %2047s", path) != 1 || strchr(path, '%') || strchr(path, '?')) {
    const char *msg = "HTTP/1.0 400 Bad Request\r
Content-Length: 0\r
\r
";
    send_all(fd, msg, strlen(msg)); return;
  }
  if (!strcmp(path, "/")) {
    const char *page = "<!doctype html><title>PocketBox</title><h1>PocketBox M0</h1><p>Offline library prototype</p><a href='/files/welcome.txt'>Welcome file</a>";
    int size = snprintf(header, sizeof header, "HTTP/1.0 200 OK\r
Content-Type: text/html\r
Content-Length: %zu\r
Connection: close\r
\r
", strlen(page));
    if (size > 0) { send_all(fd, header, (size_t)size); send_all(fd, page, strlen(page)); }
    return;
  }
  /* M0: one basename only; openat + O_NOFOLLOW prevents traversal and symlink escapes. */
  const char *prefix = "/files/";
  if (strncmp(path, prefix, strlen(prefix))) {
    const char *msg = "HTTP/1.0 404 Not Found\r
Content-Length: 0\r
\r
";
    send_all(fd, msg, strlen(msg)); return;
  }
  const char *name = path + strlen(prefix);
  if (!*name || !strcmp(name, ".") || !strcmp(name, "..") || strchr(name, '/')) {
    const char *msg = "HTTP/1.0 400 Bad Request\r
Content-Length: 0\r
\r
";
    send_all(fd, msg, strlen(msg)); return;
  }
  int file = openat(root, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
  struct stat st;
  if (file < 0 || fstat(file, &st) || !S_ISREG(st.st_mode)) {
    if (file >= 0) close(file);
    const char *msg = "HTTP/1.0 404 Not Found\r
Content-Length: 0\r
\r
";
    send_all(fd, msg, strlen(msg)); return;
  }
  int size = snprintf(header, sizeof header, "HTTP/1.0 200 OK\r
Content-Type: application/octet-stream\r
Content-Length: %lld\r
Connection: close\r
\r
", (long long)st.st_size);
  if (size > 0 && send_all(fd, header, (size_t)size) == 0)
    while ((n = read(file, buf, sizeof buf)) > 0)
      if (send_all(fd, buf, (size_t)n)) break;
  close(file);
}
/* Terminal sockets are nonblocking and processed by poll(), not recv loops. */
static void terminal_input(struct terminal *c) {
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
    if (ch == 'f' || ch == 'F') {
      const char *msg = "\r\nDownload /files/welcome.txt via HTTP\r\nChoice: ";
      send(c->fd, msg, strlen(msg), MSG_DONTWAIT | MSG_NOSIGNAL);
    }
  }
}
int main(int argc, char **argv) {
  const char *dir = "."; unsigned http = 8080, telnet = 2323;
  for (int i = 1; i < argc; ++i) {
    if (i + 1 >= argc) { fprintf(stderr, "Missing value for %s
", argv[i]); return 2; }
    if (!strcmp(argv[i], "--root")) dir = argv[++i];
    else if (!strcmp(argv[i], "--http-port")) http = (unsigned)atoi(argv[++i]);
    else if (!strcmp(argv[i], "--telnet-port")) telnet = (unsigned)atoi(argv[++i]);
    else { fprintf(stderr, "Unknown option: %s
", argv[i]); return 2; }
  }
  if (!http || http > 65535 || !telnet || telnet > 65535 || http == telnet) return 2;
  int root = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (root < 0) { perror("root"); return 1; }
  int h = listener(http), t = listener(telnet);
  if (h < 0 || t < 0) { perror("listen"); if (h >= 0) close(h); if (t >= 0) close(t); close(root); return 1; }
  signal(SIGINT, on_signal); signal(SIGTERM, on_signal);
  signal(SIGPIPE, SIG_IGN);
  fprintf(stderr, "PocketBox M0 listening on 127.0.0.1 HTTP:%u Telnet:%u
", http, telnet);
  /* HTTP request handling remains synchronous with a two-second timeout.
     Terminal clients are nonblocking and bounded to MAX_TERMINALS. */ 
  struct terminal clients[MAX_TERMINALS];
  for (unsigned i = 0; i < MAX_TERMINALS; ++i) clients[i].fd = -1;
  while (running) {
    struct pollfd fds[2 + MAX_TERMINALS];
    nfds_t count = 2;
    fds[0] = (struct pollfd){h, POLLIN, 0};
    fds[1] = (struct pollfd){t, POLLIN, 0};
    for (unsigned i = 0; i < MAX_TERMINALS; ++i)
      if (clients[i].fd >= 0) fds[count++] = (struct pollfd){clients[i].fd, POLLIN, 0};
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
          } else if (fds[i].revents & POLLIN) terminal_input(&clients[j]);
          break;
        }
    }
    if (fds[0].revents & POLLIN) {
      int fd = accept(h, NULL, NULL);
      if (fd >= 0) {
        struct timeval tv = {2, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
        http_client(fd, root);
        close(fd);
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
          clients[slot] = (struct terminal){fd, 0};
          send(fd, terminal_menu, strlen(terminal_menu), MSG_DONTWAIT | MSG_NOSIGNAL);
        }
      }
    }
  }
  for (unsigned i = 0; i < MAX_TERMINALS; ++i)
    if (clients[i].fd >= 0) close(clients[i].fd);
  close(h); close(t); close(root); return 0;
}
