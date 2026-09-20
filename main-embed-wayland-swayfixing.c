#define _GNU_SOURCE
#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-client.h>
// clang-format off
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
// clang-format on

#define WM_NAME "boomer-c"
#define WM_CLASS "boomer-c"
#define PROGRAM_NAME "boomer-c"

#define MAX_PATH_SIZE 4096
#define VELOCITY_THRESHOLD 15.0f
#define DELTA_RADIUS_DECEL 10.0f
#define SHADOW_ACCEL 6.0f
#define MAX_SHADOW 0.8f
#define MIN_GLX_MAJOR 1
#define MIN_GLX_MINOR 3
#define UNUSED(var)                                                            \
  do {                                                                         \
    (void)(var);                                                               \
  } while (0)
#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define BETWEEN(X, A, B) ((A) <= (X) && (X) <= (B))
#define LENGTH(X) (sizeof(X) / sizeof(X)[0])
#define CLAMP(A, X, B) (((X) < (A)) ? (A) : ((B) < (X)) ? (B) : (X))

/* ========================================================================
 * wlr-screencopy-unstable-v1  —  手写协议绑定
 * 来源: https://github.com/swaywm/wlr-protocols/blob/master/unstable/
 *       wlr-screencopy-unstable-v1.xml
 * ======================================================================== */

struct zwlr_screencopy_manager_v1;
struct zwlr_screencopy_frame_v1;

extern const struct wl_interface zwlr_screencopy_manager_v1_interface;
extern const struct wl_interface zwlr_screencopy_frame_v1_interface;

struct zwlr_screencopy_frame_v1_listener {
  void (*buffer)(void *, struct zwlr_screencopy_frame_v1 *, uint32_t, uint32_t,
                 uint32_t, uint32_t);
  void (*flags)(void *, struct zwlr_screencopy_frame_v1 *, uint32_t);
  void (*ready)(void *, struct zwlr_screencopy_frame_v1 *, uint32_t, uint32_t,
                uint32_t);
  void (*failed)(void *, struct zwlr_screencopy_frame_v1 *);
  void (*damage)(void *, struct zwlr_screencopy_frame_v1 *, uint32_t, uint32_t,
                 uint32_t, uint32_t);
  void (*linux_dmabuf)(void *, struct zwlr_screencopy_frame_v1 *, uint32_t,
                       uint32_t, uint32_t);
  void (*buffer_done)(void *, struct zwlr_screencopy_frame_v1 *);
};

static const struct wl_interface *sc_types[] = {
    &zwlr_screencopy_frame_v1_interface, /* 0: new_id (frame) */
    NULL,                                /* 1: int */
    &wl_output_interface,                /* 2: object (output) */
    NULL,
    NULL,
    NULL,
    NULL,                 /* 3-6: int x4 (region) */
    &wl_buffer_interface, /* 7: object (buffer) */
};

static const struct wl_message sc_manager_requests[] = {
    {"capture_output", "nio", sc_types + 0},
    {"capture_output_region", "nioiiii", sc_types + 0},
    {"destroy", "", NULL},
};

const struct wl_interface zwlr_screencopy_manager_v1_interface = {
    "zwlr_screencopy_manager_v1", 3, 3, sc_manager_requests, 0, NULL,
};

static const struct wl_message sc_frame_requests[] = {
    {"copy", "o", sc_types + 7},
    {"destroy", "", NULL},
    {"copy_with_damage", "o", sc_types + 7},
};

static const struct wl_message sc_frame_events[] = {
    {"buffer", "uuuu", NULL},  {"flags", "u", NULL},
    {"ready", "uuu", NULL},    {"failed", "", NULL},
    {"damage", "uuuu", NULL},  {"linux_dmabuf", "uuu", NULL},
    {"buffer_done", "", NULL},
};

const struct wl_interface zwlr_screencopy_frame_v1_interface = {
    "zwlr_screencopy_frame_v1", 3, 3, sc_frame_requests, 7, sc_frame_events,
};

enum {
  SC_MGR_CAPTURE_OUTPUT = 0,
  SC_MGR_CAPTURE_OUTPUT_REGION = 1,
  SC_MGR_DESTROY = 2,
};
enum { SC_FRAME_COPY = 0, SC_FRAME_DESTROY = 1, SC_FRAME_COPY_WITH_DAMAGE = 2 };

static inline struct zwlr_screencopy_frame_v1 *
sc_manager_capture_output(struct zwlr_screencopy_manager_v1 *m,
                          int32_t overlay_cursor, struct wl_output *out) {
  struct wl_proxy *id;
  id = wl_proxy_marshal_flags((struct wl_proxy *)m, SC_MGR_CAPTURE_OUTPUT,
                              &zwlr_screencopy_frame_v1_interface,
                              wl_proxy_get_version((struct wl_proxy *)m), 0,
                              NULL, overlay_cursor, out);
  return (struct zwlr_screencopy_frame_v1 *)id;
}

static inline void sc_manager_destroy(struct zwlr_screencopy_manager_v1 *m) {
  wl_proxy_marshal_flags((struct wl_proxy *)m, SC_MGR_DESTROY, NULL,
                         wl_proxy_get_version((struct wl_proxy *)m),
                         WL_MARSHAL_FLAG_DESTROY);
}

static inline void sc_frame_copy(struct zwlr_screencopy_frame_v1 *f,
                                 struct wl_buffer *b) {
  wl_proxy_marshal_flags((struct wl_proxy *)f, SC_FRAME_COPY, NULL,
                         wl_proxy_get_version((struct wl_proxy *)f), 0, b);
}

static inline void sc_frame_destroy(struct zwlr_screencopy_frame_v1 *f) {
  wl_proxy_marshal_flags((struct wl_proxy *)f, SC_FRAME_DESTROY, NULL,
                         wl_proxy_get_version((struct wl_proxy *)f),
                         WL_MARSHAL_FLAG_DESTROY);
}

static inline int
sc_frame_add_listener(struct zwlr_screencopy_frame_v1 *f,
                      const struct zwlr_screencopy_frame_v1_listener *l,
                      void *data) {
  return wl_proxy_add_listener((struct wl_proxy *)f, (void (**)(void))l, data);
}

/* ========================================================================
 * Wayland screencopy 运行时
 * ======================================================================== */

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint32_t format;
  void *data;
  size_t size;
  bool y_invert;
} WlScreencopyResult;

static struct wl_display *wl_dpy = NULL;
static struct wl_registry *wl_reg = NULL;
static struct wl_shm *wl_shm_obj = NULL;
static struct wl_output *wl_out = NULL;
static struct zwlr_screencopy_manager_v1 *wl_sc_mgr = NULL;

static struct zwlr_screencopy_frame_v1 *g_frame = NULL;
static struct wl_buffer *g_buffer = NULL;
static void *g_buffer_data = NULL;
static size_t g_buffer_size = 0;
static uint32_t g_buf_fmt = 0, g_buf_w = 0, g_buf_h = 0, g_buf_stride = 0;
static bool g_buf_y_invert = false;
static bool g_frame_done = false;
static bool g_frame_failed = false;

static void wl_out_geometry(void *d, struct wl_output *o, int32_t x, int32_t y,
                            int32_t pw, int32_t ph, int32_t sp, const char *mk,
                            const char *md, int32_t t) {
  (void)d;
  (void)o;
  (void)x;
  (void)y;
  (void)pw;
  (void)ph;
  (void)sp;
  (void)mk;
  (void)md;
  (void)t;
}
static void wl_out_mode(void *d, struct wl_output *o, uint32_t f, int32_t w,
                        int32_t h, int32_t r) {
  (void)d;
  (void)o;
  (void)f;
  (void)w;
  (void)h;
  (void)r;
}
static void wl_out_done(void *d, struct wl_output *o) {
  (void)d;
  (void)o;
}
static void wl_out_scale(void *d, struct wl_output *o, int32_t f) {
  (void)d;
  (void)o;
  (void)f;
}
static void wl_out_name(void *d, struct wl_output *o, const char *n) {
  (void)d;
  (void)o;
  (void)n;
}
static void wl_out_desc(void *d, struct wl_output *o, const char *s) {
  (void)d;
  (void)o;
  (void)s;
}
static const struct wl_output_listener wl_out_listener = {
    .geometry = wl_out_geometry,
    .mode = wl_out_mode,
    .done = wl_out_done,
    .scale = wl_out_scale,
    .name = wl_out_name,
    .description = wl_out_desc,
};

static struct wl_buffer *make_shm_buffer(uint32_t fmt, int w, int h,
                                         int stride) {
  int size = stride * h;
  static const char name[] = "/boomer-c-sc";
  int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd < 0)
    return NULL;
  shm_unlink(name);
  int ret;
  while ((ret = ftruncate(fd, size)) == -1 && errno == EINTR) {
  }
  if (ret < 0) {
    close(fd);
    return NULL;
  }
  void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return NULL;
  }
  struct wl_shm_pool *pool = wl_shm_create_pool(wl_shm_obj, fd, size);
  close(fd);
  struct wl_buffer *buf =
      wl_shm_pool_create_buffer(pool, 0, w, h, stride, (enum wl_shm_format)fmt);
  wl_shm_pool_destroy(pool);
  g_buffer_data = data;
  g_buffer_size = (size_t)size;
  return buf;
}

static void fr_buffer(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t fmt,
                      uint32_t w, uint32_t h, uint32_t stride) {
  (void)d;
  g_buf_fmt = fmt;
  g_buf_w = w;
  g_buf_h = h;
  g_buf_stride = stride;
  g_buffer = make_shm_buffer(fmt, (int)w, (int)h, (int)stride);
  if (!g_buffer) {
    g_frame_failed = true;
    return;
  }
  sc_frame_copy(f, g_buffer);
}
static void fr_flags(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t fl) {
  (void)d;
  (void)f;
  g_buf_y_invert = (fl & 1) != 0;
}
static void fr_ready(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t a,
                     uint32_t b, uint32_t c) {
  (void)d;
  (void)f;
  (void)a;
  (void)b;
  (void)c;
  g_frame_done = true;
}
static void fr_failed(void *d, struct zwlr_screencopy_frame_v1 *f) {
  (void)d;
  (void)f;
  g_frame_failed = true;
}
static void fr_damage(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t x,
                      uint32_t y, uint32_t w, uint32_t h) {
  (void)d;
  (void)f;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}
static void fr_dmabuf(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t a,
                      uint32_t b, uint32_t c) {
  (void)d;
  (void)f;
  (void)a;
  (void)b;
  (void)c;
}
static void fr_bufdone(void *d, struct zwlr_screencopy_frame_v1 *f) {
  (void)d;
  (void)f;
}
static const struct zwlr_screencopy_frame_v1_listener fr_listener = {
    .buffer = fr_buffer,
    .flags = fr_flags,
    .ready = fr_ready,
    .failed = fr_failed,
    .damage = fr_damage,
    .linux_dmabuf = fr_dmabuf,
    .buffer_done = fr_bufdone,
};

static void reg_global(void *d, struct wl_registry *r, uint32_t name,
                       const char *iface, uint32_t ver) {
  (void)d;
  if (!strcmp(iface, wl_shm_interface.name)) {
    wl_shm_obj = wl_registry_bind(r, name, &wl_shm_interface, 1);
  } else if (!strcmp(iface, wl_output_interface.name)) {
    if (!wl_out) {
      wl_out =
          wl_registry_bind(r, name, &wl_output_interface, ver < 4 ? ver : 4);
      wl_output_add_listener(wl_out, &wl_out_listener, NULL);
    }
  } else if (!strcmp(iface, zwlr_screencopy_manager_v1_interface.name)) {
    wl_sc_mgr =
        wl_registry_bind(r, name, &zwlr_screencopy_manager_v1_interface, 1);
  }
}
static void reg_remove(void *d, struct wl_registry *r, uint32_t name) {
  (void)d;
  (void)r;
  (void)name;
}
static const struct wl_registry_listener reg_listener = {
    .global = reg_global,
    .global_remove = reg_remove,
};

static int wl_screencopy_init(void) {
  wl_dpy = wl_display_connect(NULL);
  if (!wl_dpy) {
    fprintf(stderr, "wl: connect failed\n");
    return -1;
  }
  wl_reg = wl_display_get_registry(wl_dpy);
  wl_registry_add_listener(wl_reg, &reg_listener, NULL);
  wl_display_roundtrip(wl_dpy);
  wl_display_roundtrip(wl_dpy);
  if (!wl_shm_obj || !wl_sc_mgr || !wl_out) {
    fprintf(stderr, "wl: missing globals (shm=%p sc=%p out=%p)\n",
            (void *)wl_shm_obj, (void *)wl_sc_mgr, (void *)wl_out);
    return -1;
  }
  return 0;
}

static int wl_screencopy_capture(WlScreencopyResult *res) {
  if (!wl_sc_mgr || !wl_out)
    return -1;
  g_frame_done = g_frame_failed = false;
  g_buffer = NULL;
  g_buffer_data = NULL;
  g_buffer_size = 0;

  g_frame = sc_manager_capture_output(wl_sc_mgr, 0, wl_out);
  sc_frame_add_listener(g_frame, &fr_listener, NULL);

  while (!g_frame_done && !g_frame_failed) {
    if (wl_display_dispatch(wl_dpy) < 0) {
      g_frame_failed = true;
      break;
    }
  }
  if (g_frame_failed || !g_buffer_data) {
    if (g_frame) {
      sc_frame_destroy(g_frame);
      g_frame = NULL;
    }
    return -1;
  }
  res->width = g_buf_w;
  res->height = g_buf_h;
  res->stride = g_buf_stride;
  res->format = g_buf_fmt;
  res->data = g_buffer_data;
  res->size = g_buffer_size;
  res->y_invert = g_buf_y_invert;

  sc_frame_destroy(g_frame);
  g_frame = NULL;
  return 0;
}

static void wl_screencopy_release(WlScreencopyResult *res) {
  if (res->data && res->size)
    munmap(res->data, res->size);
  if (g_buffer) {
    wl_buffer_destroy(g_buffer);
    g_buffer = NULL;
  }
  res->data = NULL;
  res->size = 0;
}

static void wl_screencopy_fini(void) {
  if (wl_out) {
    wl_output_destroy(wl_out);
    wl_out = NULL;
  }
  if (wl_sc_mgr) {
    sc_manager_destroy(wl_sc_mgr);
    wl_sc_mgr = NULL;
  }
  if (wl_shm_obj) {
    wl_shm_destroy(wl_shm_obj);
    wl_shm_obj = NULL;
  }
  if (wl_reg) {
    wl_registry_destroy(wl_reg);
    wl_reg = NULL;
  }
  if (wl_dpy) {
    wl_display_disconnect(wl_dpy);
    wl_dpy = NULL;
  }
}

/* ========================================================================
 * 以下是原 boomer-c.c 的主体
 * ======================================================================== */

void die(const char *fmt, ...) {
  va_list args;
  int saved_errno;
  saved_errno = errno;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
    fprintf(stderr, " %s", strerror(saved_errno));
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

typedef struct {
  GLfloat x;
  GLfloat y;
} Vec2f;
#define UNIT (Vec2f){1.0f, 1.0f}
#define ZERO (Vec2f){0.0f, 0.0f}
#define MUL(v, u)                                                              \
  (Vec2f) { (v).x *(u).x, (v).y *(u).y }
#define DIV(v, u)                                                              \
  (Vec2f) { (v).x / (u).x, (v).y / (u).y }
#define ADD(v, u)                                                              \
  (Vec2f) { (v).x + (u).x, (v).y + (u).y }
#define SUB(v, u)                                                              \
  (Vec2f) { (v).x - (u).x, (v).y - (u).y }
#define EQ(v, u) ((v).x == (u).x && (v).y == (u).y)
#define LEN(v) sqrtf((v).x *(v).x + (v).y * (v).y)
#define NORM(v) LEN(v) == 0.0f ? ZERO : (DIV(v, LEN(v)))
#define MULS(v, s)                                                             \
  (Vec2f) { (v).x *(s), (v).y *(s) }
#define DIVS(v, s)                                                             \
  (Vec2f) { (v).x / (s), (v).y / (s) }
#define ADDS(v, s)                                                             \
  (Vec2f) { (v).x + (s), (v).y + (s) }
#define SUBS(v, s)                                                             \
  (Vec2f) { (v).x - (s), (v).y - (s) }

#if defined(__clang__) && __clang_major__ >= 16
#define HAS_EMBED 1
static const unsigned char vert_src[] = {
#embed "vertex.glsl"
    , '\0'};
static const unsigned char frag_src[] = {
#embed "fragment.glsl"
    , '\0'};
static const unsigned char embedded_config[] = {
#embed "config.conf"
    , '\0'};
#else
#define HAS_EMBED 0
static const unsigned char vert_src[] = "";
static const unsigned char frag_src[] = "";
static const unsigned char embedded_config[] = "";
#endif

typedef struct {
  float min_scale;
  float max_scale;
  float drag_friction;
  float scale_friction;
  float scroll_speed;
  float key_move_speed;
  bool windowed;
  char *fragment_shader_file;
  char *vertex_shader_file;
} Config;

typedef struct {
  Vec2f position;
  Vec2f velocity;
  Vec2f scale_pivot;
  GLfloat scale;
  GLfloat delta_scale;
  float dt;
} Camera;

typedef struct {
  bool is_enabled;
  GLfloat shadow;
  GLfloat radius;
  GLfloat delta_radius;
} Flashlight;

typedef struct {
  Vec2f current;
  Vec2f previous;
  bool dragging;
} Mouse;

Config get_default_config(void) {
  return (Config){
      .min_scale = 0.1,
      .max_scale = 6.0,
      .drag_friction = 6.0,
      .scale_friction = 4.0,
      .scroll_speed = 1.5,
      .key_move_speed = 400.0,
      .windowed = false,
      .vertex_shader_file = NULL,
      .fragment_shader_file = NULL,
  };
}

int parse_bool(char *arg) {
  if (arg == NULL)
    return -1;
  char buff[8] = {0};
  strncpy(buff, arg, sizeof(buff) - 1);
  for (int i = 0; buff[i]; i++) {
    buff[i] = tolower(buff[i]);
    if (buff[i] == '\n' || buff[i] == '\r') {
      buff[i] = '\0';
      break;
    }
  }
  if (!strcmp(buff, "true") || !strcmp(buff, "t") || !strcmp(buff, "yes") ||
      !strcmp(buff, "y") || !strcmp(buff, "1"))
    return 1;
  if (!strcmp(buff, "false") || !strcmp(buff, "f") || !strcmp(buff, "no") ||
      !strcmp(buff, "n") || !strcmp(buff, "0"))
    return 0;
  return -1;
}

static void parse_config_from_memory(Config *conf, const char *raw) {
  char *buf = strdup(raw);
  char *line = buf;
  char *ptr;
  while ((ptr = strchr(line, '\n')) != NULL) {
    *ptr = '\0';
    if (line[0] == '#' || line[0] == '\0') {
      line = ptr + 1;
      continue;
    }
    char *tok = strtok(line, " \t=");
    while (tok != NULL) {
      char *arg = tok;
      tok = strtok(NULL, " \t=");
      if (tok == NULL)
        die("Expected value for argument %s\n", arg);
      if (!strcmp(arg, "min_scale")) {
        conf->min_scale = strtof(tok, NULL);
      } else if (!strcmp(arg, "max_scale")) {
        conf->max_scale = strtof(tok, NULL);
      } else if (!strcmp(arg, "scroll_speed")) {
        conf->scroll_speed = strtof(tok, NULL);
      } else if (!strcmp(arg, "drag_friction")) {
        conf->drag_friction = strtof(tok, NULL);
      } else if (!strcmp(arg, "scale_friction")) {
        conf->scale_friction = strtof(tok, NULL);
      } else if (!strcmp(arg, "key_move_speed")) {
        conf->key_move_speed = strtof(tok, NULL);
      } else if (!strcmp(arg, "windowed")) {
        int b = parse_bool(tok);
        if (b != -1)
          conf->windowed = (bool)b;
      } else {
        die("Unexpected configuration key '%s'\n", arg);
      }
      tok = strtok(NULL, " \t=");
    }
    line = ptr + 1;
  }
  free(buf);
}

void parse_config(Config *conf, FILE *f) {
  char *line = NULL;
  size_t len = 0;
  while (getline(&line, &len, f) != -1) {
    if (line[0] == '#' || line[0] == '\n') {
      continue;
    }
    char *c;
    c = strtok(line, " \t=");
    while (c != NULL) {
      char *arg = c;
      c = strtok(NULL, " \t=");
      if (c == NULL)
        die("Expected value for argument %s\n", arg);
      if (!strcmp(arg, "min_scale")) {
        conf->min_scale = strtof(c, NULL);
      } else if (!strcmp(arg, "max_scale")) {
        conf->max_scale = strtof(c, NULL);
      } else if (!strcmp(arg, "scroll_speed")) {
        conf->scroll_speed = strtof(c, NULL);
      } else if (!strcmp(arg, "drag_friction")) {
        conf->drag_friction = strtof(c, NULL);
      } else if (!strcmp(arg, "scale_friction")) {
        conf->scale_friction = strtof(c, NULL);
      } else if (!strcmp(arg, "key_move_speed")) {
        conf->key_move_speed = strtof(c, NULL);
      } else if (!strcmp(arg, "windowed")) {
        if (parse_bool(c) != -1) {
          conf->windowed = (bool)parse_bool(c);
        }
      } else {
        die("Unexpected configuration key '%s'\n", arg);
      }
      c = strtok(NULL, " \t=");
    }
  }
  free(line);
  fclose(f);
}

Config load_config(void) {
  const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
  char *allocated_xdg = NULL;
  if (xdg_config_home == NULL) {
    const char *home = getenv("HOME");
    if (home == NULL)
      die("Error: HOME environment variable not set!\n");
    size_t bufsz = strlen(home) + strlen("/.config") + 1;
    allocated_xdg = malloc(bufsz);
    if (!allocated_xdg)
      die("Malloc failed for xdg path");
    snprintf(allocated_xdg, bufsz, "%s/.config", home);
    xdg_config_home = allocated_xdg;
  }

  char config_dir[MAX_PATH_SIZE - 15];
  snprintf(config_dir, sizeof(config_dir), "%s/%s", xdg_config_home,
           PROGRAM_NAME);
  if (mkdir(config_dir, 0755) == -1 && errno != EEXIST) {
    if (allocated_xdg)
      free(allocated_xdg);
    die("Failed to create directory '%s': %s", config_dir, strerror(errno));
  }

  char config_file[MAX_PATH_SIZE];
  snprintf(config_file, MAX_PATH_SIZE, "%s/config.conf", config_dir);
  if (access(config_file, F_OK | R_OK)) {
    strncpy(config_file, "./config.conf", sizeof(config_file) - 1);
    config_file[MAX_PATH_SIZE - 1] = '\0';
  }

  Config conf = get_default_config();
  FILE *f = fopen(config_file, "r");
  if (f != NULL) {
    parse_config(&conf, f);
  } else {
#if HAS_EMBED
    parse_config_from_memory(&conf, (const char *)embedded_config);
#else
    die("Config file missing and #embed unavailable, cannot load config\n");
#endif
  }

  char *vertex_shader_file = malloc(MAX_PATH_SIZE);
  char *fragment_shader_file = malloc(MAX_PATH_SIZE);
  snprintf(fragment_shader_file, MAX_PATH_SIZE, "%s/fragment.glsl", config_dir);
  snprintf(vertex_shader_file, MAX_PATH_SIZE, "%s/vertex.glsl", config_dir);

  if (!fopen(vertex_shader_file, "r")) {
    strncpy(vertex_shader_file, "./vertex.glsl", MAX_PATH_SIZE - 1);
    vertex_shader_file[MAX_PATH_SIZE - 1] = '\0';
  }
  if (!fopen(fragment_shader_file, "r")) {
    strncpy(fragment_shader_file, "./fragment.glsl", MAX_PATH_SIZE - 1);
    fragment_shader_file[MAX_PATH_SIZE - 1] = '\0';
  }

  conf.fragment_shader_file = fragment_shader_file;
  conf.vertex_shader_file = vertex_shader_file;

  if (allocated_xdg)
    free(allocated_xdg);
  return conf;
}

void update_flashlight(Flashlight *fl, float dt) {
  if (fabsf(fl->delta_radius) > 1.0f) {
    fl->radius = MAX(0.0f, fl->radius + fl->delta_radius * dt);
    fl->delta_radius -= fl->delta_radius * DELTA_RADIUS_DECEL * dt;
  }
  if (fl->is_enabled)
    fl->shadow = MIN(fl->shadow + SHADOW_ACCEL * dt, MAX_SHADOW);
  else
    fl->shadow = MAX(fl->shadow - SHADOW_ACCEL * dt, 0.0f);
}

void initialize_mouse(Display *dpy, Mouse *m) {
  Window _root, _child;
  int _win_x, _win_y;
  unsigned int _mask;
  int root_x, root_y;
  root_x = root_y = 0;
  XQueryPointer(dpy, DefaultRootWindow(dpy), &_root, &_child, &root_x, &root_y,
                &_win_x, &_win_y, &_mask);
  m->current = m->previous = (Vec2f){root_x, root_y};
  m->dragging = false;
}

void update_camera(Camera *cam, Config *config, Mouse *mouse,
                   Vec2f window_size) {
  if (fabsf(cam->delta_scale) > 0.5f) {
    Vec2f p0 = DIVS(SUB(cam->scale_pivot, MULS(window_size, 0.5)), cam->scale);
    cam->scale =
        CLAMP(config->min_scale, cam->scale + cam->delta_scale * cam->dt,
              config->max_scale);
    Vec2f p1 = DIVS(SUB(cam->scale_pivot, MULS(window_size, 0.5)), cam->scale);
    cam->position = ADD(cam->position, SUB(p0, p1));
    cam->delta_scale -= cam->delta_scale * cam->dt * config->scale_friction;
  }
  if (!mouse->dragging && LEN(cam->velocity) > VELOCITY_THRESHOLD) {
    cam->position = ADD(cam->position, MULS(cam->velocity, cam->dt));
    cam->velocity = SUB(cam->velocity,
                        MULS(cam->velocity, cam->dt * config->drag_friction));
  }
}

Vec2f world(Camera *cam, Vec2f v) { return DIVS(v, cam->scale); }

static Display *dpy = NULL;
static int screen = 0;
static XWindowAttributes wa;
static Window w;
static bool running = true;
static GLuint shader_program;
static Flashlight flashlight;
static Camera camera;
static Mouse mouse;
static Config config;

void keypress(XEvent *);
void motion_notify(XEvent *);
void scroll_down(unsigned int, bool);
void scroll_up(unsigned int, bool);
void button_press(XEvent *);
void button_release(XEvent *);
static void (*handler[LASTEvent])(XEvent *) = {
    [MotionNotify] = motion_notify,
    [KeyPress] = keypress,
    [ButtonPress] = button_press,
    [ButtonRelease] = button_release,
};

static const char *try_load_file(const char *path, char **out_buf) {
  *out_buf = NULL;
  FILE *fp = fopen(path, "rb");
  if (!fp)
    return NULL;
  fseek(fp, 0, SEEK_END);
  long sz = ftell(fp);
  rewind(fp);
  char *buf = malloc(sz + 1);
  if (!buf) {
    fclose(fp);
    return NULL;
  }
  size_t rd = fread(buf, 1, sz, fp);
  buf[rd] = '\0';
  fclose(fp);
  *out_buf = buf;
  return buf;
}

GLuint load_shader(GLenum type, const char *file_path) {
  const GLchar *src = NULL;
  char *heap_buf = NULL;

#if HAS_EMBED
  if (!file_path || !*file_path) {
    if (type == GL_VERTEX_SHADER)
      src = (const GLchar *)vert_src;
    if (type == GL_FRAGMENT_SHADER)
      src = (const GLchar *)frag_src;
  }
#endif

  if (file_path && *file_path) {
    src = try_load_file(file_path, &heap_buf);
#if HAS_EMBED
    if (!src) {
      if (type == GL_VERTEX_SHADER)
        src = (const GLchar *)vert_src;
      if (type == GL_FRAGMENT_SHADER)
        src = (const GLchar *)frag_src;
    }
#endif
  }

  if (!src)
    die("No shader source available");

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  int ok;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    die("Shader compile error:\n%s", log);
  }

  if (heap_buf)
    free(heap_buf);
  return shader;
}

void update_program(char *vertex_shader_filename,
                    char *fragment_shader_filename) {
  GLuint vertex_shader;
  GLuint fragment_shader;
  vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_filename);
  fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_filename);
  GLuint new_program = glCreateProgram();
  glAttachShader(new_program, vertex_shader);
  glAttachShader(new_program, fragment_shader);
  glLinkProgram(new_program);
  int link_success;
  glGetProgramiv(new_program, GL_LINK_STATUS, &link_success);
  if (link_success) {
    glDeleteProgram(shader_program);
    shader_program = new_program;
  } else {
    GLchar info_log[512];
    glGetProgramInfoLog(new_program, 512, NULL, info_log);
    fprintf(stderr, "Error whilst linking program:\n%s", info_log);
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void check_glx_version(Display *dpy) {
  int glx_major, glx_minor;
  if (!glXQueryVersion(dpy, &glx_major, &glx_minor) ||
      (glx_major == MIN_GLX_MAJOR && glx_minor < MIN_GLX_MINOR) ||
      (glx_major < 1))
    die("Invalid GLX version %d.%d. Requires GLX >= %d.%d", glx_major,
        glx_minor, MIN_GLX_MAJOR, MIN_GLX_MINOR);
}

void draw_image(Camera *cam, GLuint shader, GLuint vao, Vec2f window_size,
                Mouse *mouse, Flashlight *fl) {
  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(shader);
  glUniform2f(glGetUniformLocation(shader, "cameraPos"), cam->position.x,
              cam->position.y);
  glUniform1f(glGetUniformLocation(shader, "cameraScale"), cam->scale);
  glUniform2f(glGetUniformLocation(shader, "screenshotSize"), window_size.x,
              window_size.y);
  glUniform2f(glGetUniformLocation(shader, "windowSize"), window_size.x,
              window_size.y);
  glUniform2f(glGetUniformLocation(shader, "cursorPos"), mouse->current.x,
              mouse->current.y);
  glUniform1f(glGetUniformLocation(shader, "flShadow"), fl->shadow);
  glUniform1f(glGetUniformLocation(shader, "flRadius"), fl->radius);
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
  glBindVertexArray(0);
}

void keypress(XEvent *e) {
  KeySym keysym;
  XKeyEvent *ev;
  ev = (XKeyEvent *)&e->xkey;
  keysym = XkbKeycodeToKeysym(dpy, ev->keycode, 0,
                              e->xkey.state & ShiftMask ? 1 : 0);
  switch (keysym) {
  case XK_Left:
  case XK_h:
    camera.velocity.x -= config.key_move_speed;
    break;
  case XK_Down:
  case XK_j:
    camera.velocity.y += config.key_move_speed;
    break;
  case XK_Up:
  case XK_k:
    camera.velocity.y -= config.key_move_speed;
    break;
  case XK_Right:
  case XK_l:
    camera.velocity.x += config.key_move_speed;
    break;
  case XK_minus:
    scroll_down(1, ev->state & ControlMask);
    break;
  case XK_equal:
    scroll_up(1, ev->state & ControlMask);
    break;
  case XK_q:
  case XK_Escape:
    running = false;
    break;
  case XK_0:
    camera.scale = 1.0f;
    camera.delta_scale = 0.0f;
    camera.velocity = camera.position = (Vec2f){0, 0};
    break;
  case XK_r:
    config = load_config();
    update_program(config.vertex_shader_file, config.fragment_shader_file);
    break;
  case XK_f:
    flashlight.is_enabled = !flashlight.is_enabled;
    break;
  }
}

void scroll_up(unsigned int delta, bool fl_enabled) {
  if (delta > 0 && fl_enabled) {
    flashlight.delta_radius += 250.0f;
  } else {
    camera.delta_scale += config.scroll_speed;
    camera.scale_pivot = mouse.current;
  }
}

void scroll_down(unsigned int delta, bool fl_enabled) {
  if (delta > 0 && fl_enabled) {
    flashlight.delta_radius -= 250.0f;
  } else {
    camera.delta_scale -= config.scroll_speed;
    camera.scale_pivot = mouse.current;
  }
}

void button_press(XEvent *e) {
  XButtonEvent *ev;
  ev = (XButtonEvent *)&e->xbutton;
  switch (ev->button) {
  case Button1:
    mouse.previous = mouse.current;
    mouse.dragging = true;
    camera.velocity = ZERO;
    break;
  case Button4:
    scroll_up(ev->state & ControlMask, flashlight.is_enabled);
    break;
  case Button5:
    scroll_down(ev->state & ControlMask, flashlight.is_enabled);
    break;
  }
}

void button_release(XEvent *e) {
  XButtonEvent *ev;
  ev = (XButtonEvent *)&e->xbutton;
  if (ev->button == Button1)
    mouse.dragging = false;
}

void motion_notify(XEvent *e) {
  XMotionEvent *ev;
  ev = (XMotionEvent *)&e->xmotion;
  mouse.current = (Vec2f){ev->x, ev->y};
  if (mouse.dragging) {
    Vec2f delta =
        SUB(world(&camera, mouse.previous), world(&camera, mouse.current));
    camera.position = ADD(camera.position, delta);
    camera.velocity = MULS(delta, 1.0f / camera.dt);
  }
  mouse.previous = mouse.current;
}

int main(int argc, char *argv[]) {
  UNUSED(argv);
  if (argc > 1) {
    die("boomer-c-1.0\n"
        "Usage: boomer-c\n"
        "Just it, wish you find it fun\n"
        "\n");
  }
  config = load_config();
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL)
    die("Cannot connect to the X display server\n");
  check_glx_version(dpy);
  screen = DefaultScreen(dpy);
  GLint attrs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *vi = glXChooseVisual(dpy, screen, attrs);
  if (vi == NULL)
    die("No appropriate visual found!\n");
  XSetWindowAttributes swa;
  memset(&swa, 0, sizeof(XSetWindowAttributes));
  swa.colormap =
      XCreateColormap(dpy, DefaultRootWindow(dpy), vi->visual, AllocNone);
  swa.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask |
                   KeyReleaseMask | PointerMotionMask | ClientMessage;
  if (!config.windowed) {
    swa.override_redirect = 1;
    swa.save_under = 1;
  }
  XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &wa);
  w = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, wa.width, wa.height, 0,
                    vi->depth, InputOutput, vi->visual,
                    CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder,
                    &swa);
  XMapWindow(dpy, w);
  char *wm_name = WM_NAME;
  char *wm_class = WM_CLASS;
  XClassHint hints = {.res_name = wm_name, .res_class = wm_class};
  XStoreName(dpy, w, wm_name);
  XSetClassHint(dpy, w, &hints);
  Atom wm_delete_atom = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(dpy, w, &wm_delete_atom, 1);
  GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
  glXMakeCurrent(dpy, w, glc);
  if (GLEW_OK != glewInit())
    die("Couldnt initialize glew!\n");
  glViewport(0, 0, wa.width, wa.height);
  XSelectInput(dpy, w,
               ButtonPressMask | ButtonReleaseMask | KeyPressMask |
                   KeyReleaseMask | PointerMotionMask);
  int revert_to_parent;
  Window origin_win;
  XGetInputFocus(dpy, &origin_win, &revert_to_parent);
  update_program(config.vertex_shader_file, config.fragment_shader_file);

  /* --- Wayland 截图初始化 --- */
  if (wl_screencopy_init() != 0)
    die("Failed to init Wayland screencopy\n");
  WlScreencopyResult wl_res = {0};
  if (wl_screencopy_capture(&wl_res) != 0)
    die("Failed to capture screen via Wayland\n");

  /* 世界坐标系统一用 X11 逻辑像素，避免与物理像素混用导致拉伸/模糊 */
  Vec2f screenshot_size = (Vec2f){(float)wa.width, (float)wa.height};
  int sw = wa.width;
  int sh = wa.height;

  GLuint vbo, vao, ebo;
  GLfloat vertices[] = {sw, 0,  0.0, 1.0, 1.0, sw, sh, 0.0, 1.0, 0.0,
                        0,  sh, 0.0, 0.0, 0.0, 0,  0,  0.0, 0.0, 1.0};
  GLuint indices[] = {0, 1, 3, 1, 2, 3};
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);
  GLsizei stride = 5 * sizeof(GLfloat);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  fprintf(stderr, "x11=%dx%d wl=%ux%u\n", wa.width, wa.height, wl_res.width,
          wl_res.height);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)wl_res.width,
               (GLsizei)wl_res.height, 0, GL_BGRA, GL_UNSIGNED_BYTE,
               wl_res.data);

  glGenerateMipmap(GL_TEXTURE_2D);
  glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);
  glEnable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  XRRScreenConfiguration *screen_config =
      XRRGetScreenInfo(dpy, DefaultRootWindow(dpy));
  float rate = 1.0f / XRRConfigCurrentRate(screen_config);
  flashlight = (Flashlight){
      .is_enabled = false,
      .shadow = 0.0f,
      .radius = 200.0f,
      .delta_radius = 0.0f,
  };
  camera = (Camera){
      .position = ZERO,
      .velocity = ZERO,
      .scale_pivot = ZERO,
      .scale = 1.0f,
      .dt = rate,
  };
  initialize_mouse(dpy, &mouse);

  /* --- FIX: 让 XWayland 把键盘/指针事件派发到这个窗口 ---
   * 由于用了 override_redirect，Sway 不会自动把 seat focus 交给这个窗口。
   * 在 X11 层抓取键盘/指针，XWayland 就会拦截并转发事件过来。 */
  XSync(dpy, False);
  XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
  {
    int gr = XGrabKeyboard(dpy, w, True, GrabModeAsync, GrabModeAsync,
                           CurrentTime);
    if (gr != GrabSuccess)
      fprintf(stderr, "XGrabKeyboard failed: %d\n", gr);
    gr = XGrabPointer(dpy, w, True,
                      ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                      GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    if (gr != GrabSuccess)
      fprintf(stderr, "XGrabPointer failed: %d\n", gr);
  }
  XSync(dpy, False);

  XEvent e;
  while (running) {
    if (!config.windowed)
      XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
    while (XPending(dpy) > 0) {
      XNextEvent(dpy, &e);
      switch (e.type) {
      case ClientMessage:
        if ((Atom)e.xclient.data.l[0] == wm_delete_atom)
          running = false;
        break;
      case KeyPress:
      case ButtonPress:
      case ButtonRelease:
      case MotionNotify:
        handler[e.type](&e);
        break;
      default:
        break;
      }
    }
    update_flashlight(&flashlight, camera.dt);
    update_camera(&camera, &config, &mouse, screenshot_size);
    draw_image(&camera, shader_program, vao, screenshot_size, &mouse,
               &flashlight);
    glXSwapBuffers(dpy, w);
    glFinish();
  }

  XUngrabKeyboard(dpy, CurrentTime);
  XUngrabPointer(dpy, CurrentTime);
  wl_screencopy_release(&wl_res);
  wl_screencopy_fini();
  free(config.vertex_shader_file);
  free(config.fragment_shader_file);
  XSetInputFocus(dpy, origin_win, RevertToParent, CurrentTime);
  glXMakeCurrent(dpy, None, NULL);
  glXDestroyContext(dpy, glc);
  XCloseDisplay(dpy);
  return 0;
}