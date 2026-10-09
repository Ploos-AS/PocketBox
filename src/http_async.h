/* Bounded nonblocking HTTP/1.0 GET handler. Included by main.c. */
#define MAX_HTTP 8
#define HTTP_REQ 2048
#define HTTP_OUT 2048
#define HTTP_IDLE_SECONDS 5
struct http_conn {
  int fd, file;
  size_t used, out_len, out_pos;
  char request[HTTP_REQ], output[HTTP_OUT];
  time_t last;
  int state; /* 0=request, 1=write, 2=file, 3=close */
};
static void http_close(struct http_conn *c) {
  if (c->fd >= 0) close(c->fd);
  if (c->file >= 0) close(c->file);
  c->fd = c->file = -1;
}
static void http_response(struct http_conn *c, int status, const char *reason,
                          const char *type, long long length) {
  int n = snprintf(c->output, sizeof c->output,
      "HTTP/1.0 %d %s\r\nContent-Type: %s\r\nContent-Length: %lld\r\nConnection: close\r\n\r\n",
      status, reason, type, length);
  if (n < 0 || (size_t)n >= sizeof c->output) { c->state = 3; return; }
  c->out_len = (size_t)n; c->out_pos = 0; c->state = 1;
}
static void http_error(struct http_conn *c, int status, const char *reason) {
  http_response(c, status, reason, "text/plain", 0);
}
static void http_prepare(struct http_conn *c, int root) {
  const char *line_end = strstr(c->request, "\r\n");
  if (!line_end) { http_error(c, 400, "Bad Request"); return; }
  size_t len = (size_t)(line_end - c->request);
  if (len >= HTTP_REQ) { http_error(c, 400, "Bad Request"); return; }
  char line[HTTP_REQ], path[HTTP_REQ], version[16], method[8], extra;
  memcpy(line, c->request, len); line[len] = 0;
  if (sscanf(line, "%7s %2047s %15s %c", method, path, version, &extra) != 3 ||
      strcmp(method, "GET") ||
      (strcmp(version, "HTTP/1.0") && strcmp(version, "HTTP/1.1")) ||
      path[0] != '/' || strchr(path, '%') || strchr(path, '?')) {
    http_error(c, 400, "Bad Request"); return;
  }
  if (!strcmp(path, "/")) {
    const char *page = "<!doctype html><title>PocketBox</title><h1>PocketBox M1</h1><p>Offline library prototype</p><a href='/files/welcome.txt'>Welcome file</a>";
    size_t page_len = strlen(page);
    http_response(c, 200, "OK", "text/html", (long long)page_len);
    if (c->state == 3 || c->out_len + page_len > sizeof c->output) { c->state = 3; return; }
    memcpy(c->output + c->out_len, page, page_len);
    c->out_len += page_len;
    return;
  }
  const char *prefix = "/files/";
  if (strncmp(path, prefix, strlen(prefix))) { http_error(c, 404, "Not Found"); return; }
  const char *name = path + strlen(prefix);
  if (!*name || !strcmp(name, ".") || !strcmp(name, "..") || strchr(name, '/')) {
    http_error(c, 400, "Bad Request"); return;
  }
  int fd = openat(root, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
  struct stat st;
  if (fd < 0) { http_error(c, 404, "Not Found"); return; }
  if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 0) {
    close(fd); http_error(c, 404, "Not Found"); return;
  }
  c->file = fd;
  http_response(c, 200, "OK", "application/octet-stream", (long long)st.st_size);
}
static void http_step(struct http_conn *c, short events, int root) {
  if (events & (POLLERR | POLLNVAL)) { c->state = 3; return; }
  if (c->state == 0 && (events & (POLLIN | POLLHUP))) {
    ssize_t n = recv(c->fd, c->request + c->used, sizeof c->request - 1 - c->used, 0);
    if (n <= 0) { c->state = 3; return; }
    c->last = time(NULL); c->used += (size_t)n; c->request[c->used] = 0;
    if (strstr(c->request, "\r\n")) http_prepare(c, root);
    else if (c->used == sizeof c->request - 1) http_error(c, 400, "Bad Request");
  }
  if ((c->state == 1 || c->state == 2) && (events & POLLOUT)) {
    if (c->out_pos == c->out_len && c->state == 2) {
      ssize_t n = read(c->file, c->output, sizeof c->output);
      if (n <= 0) { c->state = 3; return; }
      c->out_len = (size_t)n; c->out_pos = 0;
    }
    if (c->out_pos < c->out_len) {
      ssize_t n = send(c->fd, c->output + c->out_pos, c->out_len - c->out_pos,
                       MSG_DONTWAIT | MSG_NOSIGNAL);
      if (n > 0) { c->out_pos += (size_t)n; c->last = time(NULL); }
      else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) c->state = 3;
    }
    if (c->out_pos == c->out_len && c->state == 1) c->state = c->file >= 0 ? 2 : 3;
  }
  if (events & POLLHUP && c->state == 0) c->state = 3;
}
