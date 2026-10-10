/* Shared, bounded file catalog for HTTP and ASCII terminal. */
#include <dirent.h>
#define CATALOG_MAX 32
#define CATALOG_NAME 128
struct catalog { unsigned count, has_more; char names[CATALOG_MAX][CATALOG_NAME]; };
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
static void catalog_load_page(int root, struct catalog *cat, const char *after) {
  cat->count = 0; cat->has_more = 0;
  char cursor[CATALOG_NAME]; snprintf(cursor, sizeof cursor, "%s", after ? after : "");
  for (unsigned slot = 0; slot <= CATALOG_MAX; ++slot) {
    int copy = openat(root, ".", O_RDONLY | O_DIRECTORY);
    if (copy < 0) return;
    DIR *dir = fdopendir(copy);
    if (!dir) { close(copy); return; }
    char best[CATALOG_NAME] = "";
    struct dirent *entry;
    while ((entry = readdir(dir))) {
      const char *name = entry->d_name;
      if (!catalog_valid(name) || strcmp(name, cursor) <= 0 ||
          (best[0] && strcmp(name, best) >= 0)) continue;
      int fd = openat(root, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
      struct stat st;
      if (fd < 0) continue;
      int regular = !fstat(fd, &st) && S_ISREG(st.st_mode);
      close(fd);
      if (regular) strcpy(best, name);
    }
    closedir(dir);
    if (!best[0]) return;
    if (slot == CATALOG_MAX) { cat->has_more = 1; return; }
    strcpy(cat->names[cat->count++], best);
    strcpy(cursor, best);
  }
}
static size_t catalog_html(const struct catalog *cat, char *out, size_t cap) {
  size_t used = 0;
  const char *prefix = "<!doctype html><title>PocketBox</title><h1>Files</h1><ul>";
  const char *suffix = "</ul>";
  /* Reserve enough room for the longest possible next-page link and suffix. */
  const size_t reserve = sizeof "<li><a href='/?after=" - 1
      + CATALOG_NAME - 1 + sizeof "'>Next page</a></li>" - 1
      + sizeof "</ul>" - 1;
  size_t n = strlen(prefix);
  if (n + reserve >= cap) return 0;
  memcpy(out, prefix, n); used = n;
  unsigned shown = 0;
  for (unsigned i = 0; i < cat->count; ++i) {
    int written = snprintf(out + used, cap - used,
        "<li><a href='/files/%s'>%s</a></li>", cat->names[i], cat->names[i]);
    if (written < 0 || (size_t)written >= cap - used ||
        (size_t)written + reserve >= cap - used) break;
    used += (size_t)written;
    shown++;
  }
  if (shown < cat->count || cat->has_more) {
    if (!shown) return 0;
    int written = snprintf(out + used, cap - used,
        "<li><a href='/?after=%s'>Next page</a></li>", cat->names[shown - 1]);
    if (written < 0 || (size_t)written >= cap - used) return 0;
    used += (size_t)written;
  }
  n = strlen(suffix);
  if (n >= cap - used) return 0;
  memcpy(out + used, suffix, n);
  return used + n;
}
