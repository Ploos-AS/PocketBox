#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>\n#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSZ 2048
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
  char *end = strstr(req, "\r\n");
  if (!end) { send_all(fd, "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n", 47); return; }
  *end = 0;
  if (sscanf(req, "GET %2047s", path) != 1 || strchr(path, '%') || strchr(path, '?')) {
    const char *msg = "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    send_all(fd, msg, strlen(msg)); return;
  }
  if (!strcmp(path, "/")) {
    const char *page = "<!doctype html><title>PocketBox</title><h1>PocketBox M0</h1><p>Offline library prototype</p><a href='/files/welcome.txt'>Welcome file</a>";
    int size = snprintf(header, sizeof header, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", strlen(page));
    if (size > 0) { send_all(fd, header, (size_t)size); send_all(fd, page, strlen(page)); }
    return;
  }
  /* M0: one basename only; openat + O_NOFOLLOW prevents traversal and symlink escapes. */
  const char *prefix = "/files/";
  if (strncmp(path, prefix, strlen(prefix))) {
    const char *msg = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    send_all(fd, msg, strlen(msg)); return;
  }
  const char *name = path + strlen(prefix);
  if (!*name || !strcmp(name, ".") || !strcmp(name, "..") || strchr(name, '/')) {
    const char *msg = "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    send_all(fd, msg, strlen(msg)); return;
  }
  int file = openat(root, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
  struct stat st;
  if (file < 0 || fstat(file, &st) || !S_ISREG(st.st_mode)) {
    if (file >= 0) close(file);
    const char *msg = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    send_all(fd, msg, strlen(msg)); return;
  }
  int size = snprintf(header, sizeof header, "HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %lld\r\nConnection: close\r\n\r\n", (long long)st.st_size);
  if (size > 0 && send_all(fd, header, (size_t)size) == 0)
    while ((n = read(file, buf, sizeof buf)) > 0)
      if (send_all(fd, buf, (size_t)n)) break;
  close(file);
}
static void telnet_client(int fd) {
  const char *menu = "\r\n*** POCKETBOX M0 ***\r\n[F] Files (HTTP: /files/welcome.txt)\r\n[L] Library (planned)\r\n[Q] Quit\r\nChoice: ";
  char choice = 0;
  if (send_all(fd, menu, strlen(menu))) return;
  /* Basic ASCII proof-of-concept; Telnet IAC negotiation arrives in M3. */
  for (unsigned i = 0; i < 16; ++i) {
    ssize_t n = recv(fd, &choice, 1, 0);
    if (n <= 0) return;
    if (choice == 'q' || choice == 'Q') break;
    if (choice == 'f' || choice == 'F')
      send_all(fd, "\r\nM0: download /files/welcome.txt via HTTP\r\nChoice: ", 56);
  }
  send_all(fd, "\r\nGoodbye.\r\n", 12);
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
  signal(SIGINT, on_signal); signal(SIGTERM, on_signal);\n  signal(SIGPIPE, SIG_IGN);
  fprintf(stderr, "PocketBox M0 listening on 127.0.0.1 HTTP:%u Telnet:%u\n", http, telnet);
  /* M1 step: poll() listeners, but per-client I/O remains synchronous and
     bounded by socket timeouts. Full nonblocking client states come next. */
  while (running) {
    struct pollfd fds[2] = {{h, POLLIN, 0}, {t, POLLIN, 0}};
    int ready = poll(fds, 2, 1000);
    if (ready < 0) { if (errno == EINTR) continue; perror("poll"); break; }
    if (!ready) continue;
    for (int i = 0; i < 2; ++i) {
      if (!(fds[i].revents & POLLIN)) continue;
      int fd = accept(fds[i].fd, NULL, NULL);
      if (fd < 0) continue;
      struct timeval tv = {2, 0};
      setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
      setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
      if (i) telnet_client(fd); else http_client(fd, root);
      close(fd);
    }
  }
  close(h); close(t); close(root); return 0;
}
