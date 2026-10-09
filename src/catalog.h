/* Shared, bounded file catalog for HTTP and ASCII terminal. */
#include <dirent.h>
#define CATALOG_MAX 32
#define CATALOG_NAME 128
struct catalog { unsigned count; char names[CATALOG_MAX][CATALOG_NAME]; };
static int catalog_valid(const char *name) {
  size_t n = strlen(name);
  if (!n || n >= CATALOG_NAME || name[0] == '.' || strchr(name, '/') ||
      strchr(name, '%') || strchr(name, '?') || strchr(name, '#')) return 0;
  for (size_t i = 0; i < n; ++i)
    if ((unsigned char)name[i] < 33 || (unsigned char)name[i] > 126 ||
        name[i] == '<' || name[i] == '>' || name[i] == '&' ||
        name[i] == '"' || name[i] == '\'') return 0;
  return 1;
}
static void catalog_load(int root, struct catalog *cat) {
  cat->count = 0;
  int copy = openat(root, ".", O_RDONLY | O_DIRECTORY);
  if (copy < 0) return;
  DIR *dir = fdopendir(copy);
  if (!dir) { close(copy); return; }
  struct dirent *entry;
  while (cat->count < CATALOG_MAX && (entry = readdir(dir))) {
    if (!catalog_valid(entry->d_name)) continue;
    int fd = openat(root, entry->d_name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    struct stat st;
    if (fd < 0) continue;
    int regular = !fstat(fd, &st) && S_ISREG(st.st_mode);
    close(fd);
    if (!regular) continue;
    strcpy(cat->names[cat->count++], entry->d_name);
  }
  closedir(dir);
}
static size_t catalog_html(const struct catalog *cat, char *out, size_t cap) {
  size_t used = 0;
  const char *prefix = "<!doctype html><title>PocketBox</title><h1>Files</h1><ul>";
  size_t n = strlen(prefix);
  if (n >= cap) return 0;
  memcpy(out, prefix, n); used = n;
  for (unsigned i = 0; i < cat->count; ++i) {
    int written = snprintf(out + used, cap - used,
        "<li><a href='/files/%s'>%s</a></li>", cat->names[i], cat->names[i]);
    if (written < 0 || (size_t)written >= cap - used) break;
    used += (size_t)written;
  }
  const char *suffix = "</ul>";
  if (strlen(suffix) >= cap - used) return 0;
  memcpy(out + used, suffix, strlen(suffix));
  return used + strlen(suffix);
}
