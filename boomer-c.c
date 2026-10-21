#define _GNU_SOURCE
#include <X11/X.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
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
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <png.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#if defined(__has_include)
#  if __has_include(<wayland-client.h>)
#    define HAVE_WAYLAND 1
#  else
#    define HAVE_WAYLAND 0
#  endif
#else
#  define HAVE_WAYLAND 0
#endif

#if HAVE_WAYLAND
#include <wayland-client.h>
#endif

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

#if HAVE_WAYLAND

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
    &zwlr_screencopy_frame_v1_interface, NULL, &wl_output_interface,
    NULL, NULL, NULL, NULL, &wl_buffer_interface,
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

enum { SC_MGR_CAPTURE_OUTPUT = 0, SC_MGR_CAPTURE_OUTPUT_REGION = 1, SC_MGR_DESTROY = 2 };
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

static inline void sc_frame_copy(struct zwlr_screencopy_frame_v1 *f, struct wl_buffer *b) {
  wl_proxy_marshal_flags((struct wl_proxy *)f, SC_FRAME_COPY, NULL,
                         wl_proxy_get_version((struct wl_proxy *)f), 0, b);
}

static inline void sc_frame_destroy(struct zwlr_screencopy_frame_v1 *f) {
  wl_proxy_marshal_flags((struct wl_proxy *)f, SC_FRAME_DESTROY, NULL,
                         wl_proxy_get_version((struct wl_proxy *)f),
                         WL_MARSHAL_FLAG_DESTROY);
}

static inline int sc_frame_add_listener(struct zwlr_screencopy_frame_v1 *f,
                      const struct zwlr_screencopy_frame_v1_listener *l, void *data) {
  return wl_proxy_add_listener((struct wl_proxy *)f, (void (**)(void))l, data);
}

typedef struct {
  uint32_t width; uint32_t height; uint32_t stride; uint32_t format;
  void *data; size_t size; bool y_invert;
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
  (void)d; (void)o; (void)x; (void)y; (void)pw; (void)ph;
  (void)sp; (void)mk; (void)md; (void)t;
}
static void wl_out_mode(void *d, struct wl_output *o, uint32_t f, int32_t w,
                        int32_t h, int32_t r) {
  (void)d; (void)o; (void)f; (void)w; (void)h; (void)r;
}
static void wl_out_done(void *d, struct wl_output *o) { (void)d; (void)o; }
static void wl_out_scale(void *d, struct wl_output *o, int32_t f) {
  (void)d; (void)o; (void)f;
}
static void wl_out_name(void *d, struct wl_output *o, const char *n) { (void)d; (void)o; (void)n; }
static void wl_out_desc(void *d, struct wl_output *o, const char *s) { (void)d; (void)o; (void)s; }

static const struct wl_output_listener wl_out_listener = {
    .geometry = wl_out_geometry, .mode = wl_out_mode, .done = wl_out_done,
    .scale = wl_out_scale, .name = wl_out_name, .description = wl_out_desc,
};

static struct wl_buffer *make_shm_buffer(uint32_t fmt, int w, int h, int stride) {
  int size = stride * h;
  static const char name[] = "/boomer-c-sc";
  int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd < 0) return NULL;
  shm_unlink(name);
  int ret;
  while ((ret = ftruncate(fd, size)) == -1 && errno == EINTR) {}
  if (ret < 0) { close(fd); return NULL; }
  void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) { close(fd); return NULL; }
  struct wl_shm_pool *pool = wl_shm_create_pool(wl_shm_obj, fd, size);
  close(fd);
  struct wl_buffer *buf = wl_shm_pool_create_buffer(pool, 0, w, h, stride, (enum wl_shm_format)fmt);
  wl_shm_pool_destroy(pool);
  g_buffer_data = data; g_buffer_size = (size_t)size;
  return buf;
}

static void fr_buffer(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t fmt,
                      uint32_t w, uint32_t h, uint32_t stride) {
  (void)d;
  g_buf_fmt = fmt; g_buf_w = w; g_buf_h = h; g_buf_stride = stride;
  g_buffer = make_shm_buffer(fmt, (int)w, (int)h, (int)stride);
  if (!g_buffer) { g_frame_failed = true; return; }
  sc_frame_copy(f, g_buffer);
}
static void fr_flags(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t fl) {
  (void)d; (void)f; g_buf_y_invert = (fl & 1) != 0;
}
static void fr_ready(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t a, uint32_t b, uint32_t c) {
  (void)d; (void)f; (void)a; (void)b; (void)c; g_frame_done = true;
}
static void fr_failed(void *d, struct zwlr_screencopy_frame_v1 *f) {
  (void)d; (void)f; g_frame_failed = true;
}
static void fr_damage(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
  (void)d; (void)f; (void)x; (void)y; (void)w; (void)h;
}
static void fr_dmabuf(void *d, struct zwlr_screencopy_frame_v1 *f, uint32_t a, uint32_t b, uint32_t c) {
  (void)d; (void)f; (void)a; (void)b; (void)c;
}
static void fr_bufdone(void *d, struct zwlr_screencopy_frame_v1 *f) { (void)d; (void)f; }

static const struct zwlr_screencopy_frame_v1_listener fr_listener = {
    .buffer = fr_buffer, .flags = fr_flags, .ready = fr_ready, .failed = fr_failed,
    .damage = fr_damage, .linux_dmabuf = fr_dmabuf, .buffer_done = fr_bufdone,
};

static void reg_global(void *d, struct wl_registry *r, uint32_t name, const char *iface, uint32_t ver) {
  (void)d;
  if (!strcmp(iface, wl_shm_interface.name)) {
    wl_shm_obj = wl_registry_bind(r, name, &wl_shm_interface, 1);
  } else if (!strcmp(iface, wl_output_interface.name)) {
    if (!wl_out) {
      wl_out = wl_registry_bind(r, name, &wl_output_interface, ver < 4 ? ver : 4);
      wl_output_add_listener(wl_out, &wl_out_listener, NULL);
    }
  } else if (!strcmp(iface, zwlr_screencopy_manager_v1_interface.name)) {
    wl_sc_mgr = wl_registry_bind(r, name, &zwlr_screencopy_manager_v1_interface, 1);
  }
}
static void reg_remove(void *d, struct wl_registry *r, uint32_t name) { (void)d; (void)r; (void)name; }
static const struct wl_registry_listener reg_listener = { .global = reg_global, .global_remove = reg_remove };

static int wl_screencopy_init(void) {
  wl_dpy = wl_display_connect(NULL);
  if (!wl_dpy) return -1;
  wl_reg = wl_display_get_registry(wl_dpy);
  wl_registry_add_listener(wl_reg, &reg_listener, NULL);
  wl_display_roundtrip(wl_dpy);
  wl_display_roundtrip(wl_dpy);
  if (!wl_shm_obj || !wl_sc_mgr || !wl_out) return -1;
  return 0;
}

static int wl_screencopy_capture(WlScreencopyResult *res) {
  if (!wl_sc_mgr || !wl_out) return -1;
  g_frame_done = g_frame_failed = false;
  g_buffer = NULL; g_buffer_data = NULL; g_buffer_size = 0;

  g_frame = sc_manager_capture_output(wl_sc_mgr, 0, wl_out);
  sc_frame_add_listener(g_frame, &fr_listener, NULL);

  while (!g_frame_done && !g_frame_failed) {
    if (wl_display_dispatch(wl_dpy) < 0) { g_frame_failed = true; break; }
  }
  if (g_frame_failed || !g_buffer_data) {
    if (g_frame) { sc_frame_destroy(g_frame); g_frame = NULL; }
    return -1;
  }
  res->width = g_buf_w; res->height = g_buf_h; res->stride = g_buf_stride;
  res->format = g_buf_fmt; res->data = g_buffer_data; res->size = g_buffer_size;
  res->y_invert = g_buf_y_invert;

  sc_frame_destroy(g_frame); g_frame = NULL;
  return 0;
}

static void wl_screencopy_release(WlScreencopyResult *res) {
  if (res->data && res->size) munmap(res->data, res->size);
  if (g_buffer) { wl_buffer_destroy(g_buffer); g_buffer = NULL; }
  res->data = NULL; res->size = 0;
}

static void wl_screencopy_fini(void) {
  if (wl_out) { wl_output_destroy(wl_out); wl_out = NULL; }
  if (wl_sc_mgr) { sc_manager_destroy(wl_sc_mgr); wl_sc_mgr = NULL; }
  if (wl_shm_obj) { wl_shm_destroy(wl_shm_obj); wl_shm_obj = NULL; }
  if (wl_reg) { wl_registry_destroy(wl_reg); wl_reg = NULL; }
  if (wl_dpy) { wl_display_disconnect(wl_dpy); wl_dpy = NULL; }
}

#else

typedef struct { int _dummy; } WlScreencopyResult;
static inline int wl_screencopy_init(void) { return -1; }
static inline int wl_screencopy_capture(WlScreencopyResult *res) { (void)res; return -1; }
static inline void wl_screencopy_release(WlScreencopyResult *res) { (void)res; }
static inline void wl_screencopy_fini(void) {}

#endif

/* ========================================================================
 * 公共逻辑
 * ======================================================================== */

void die(const char *fmt, ...) {
  va_list args; int saved_errno; saved_errno = errno;
  va_start(args, fmt); vfprintf(stderr, fmt, args); va_end(args);
  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') fprintf(stderr, " %s", strerror(saved_errno));
  fputc('\n', stderr); exit(EXIT_FAILURE);
}

void save_png(const char *filename, int width, int height, void *data, int stride, int bpp, bool is_bgra) {
  FILE *fp = fopen(filename, "wb");
  if (!fp) { fprintf(stderr, "Failed to open %s for writing\n", filename); return; }
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) { fclose(fp); return; }
  png_infop info = png_create_info_struct(png);
  if (!info) { png_destroy_write_struct(&png, NULL); fclose(fp); return; }
  if (setjmp(png_jmpbuf(png))) { png_destroy_write_struct(&png, &info); fclose(fp); return; }
  
  png_init_io(png, fp);
  png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);

  png_bytep row = png_malloc(png, width * 3);
  for (int y = 0; y < height; y++) {
    unsigned char *src = (unsigned char *)data + y * stride;
    for (int x = 0; x < width; x++) {
      if (bpp == 4) {
          if (is_bgra) {
              row[x * 3 + 0] = src[x * 4 + 2]; // R
              row[x * 3 + 1] = src[x * 4 + 1]; // G
              row[x * 3 + 2] = src[x * 4 + 0]; // B
          } else {
              row[x * 3 + 0] = src[x * 4 + 0]; // R
              row[x * 3 + 1] = src[x * 4 + 1]; // G
              row[x * 3 + 2] = src[x * 4 + 2]; // B
          }
      } else if (bpp == 3) {
          if (is_bgra) {
              row[x * 3 + 0] = src[x * 3 + 2]; // R
              row[x * 3 + 1] = src[x * 3 + 1]; // G
              row[x * 3 + 2] = src[x * 3 + 0]; // B
          } else {
              row[x * 3 + 0] = src[x * 3 + 0]; // R
              row[x * 3 + 1] = src[x * 3 + 1]; // G
              row[x * 3 + 2] = src[x * 3 + 2]; // B
          }
      }
    }
    png_write_row(png, row);
  }
  png_free(png, row); png_write_end(png, NULL); png_destroy_write_struct(&png, &info);
  fclose(fp);
}

void save_gl_framebuffer(const char *filename, int w, int h) {
  glReadBuffer(GL_BACK);
  unsigned char *pixels = malloc(w * h * 4);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  int row_size = w * 4;
  unsigned char *row = malloc(row_size);
  for (int y = 0; y < h / 2; y++) {
      unsigned char *top = pixels + y * row_size;
      unsigned char *bottom = pixels + (h - 1 - y) * row_size;
      memcpy(row, top, row_size);
      memcpy(top, bottom, row_size);
      memcpy(bottom, row, row_size);
  }
  free(row);

  save_png(filename, w, h, pixels, w * 4, 4, false);
  free(pixels);
}

/* === Xft 渲染系统字体 OSD 提示 === */
static XftFont *osd_font = NULL;
static XftColor osd_color;
static Visual *osd_visual = NULL;
static Colormap osd_cmap = 0;

static GLuint osd_program = 0;
static GLuint osd_vao = 0, osd_vbo = 0;
static GLuint osd_texture = 0;
static char last_osd_text[256] = {0};

void init_xft(Display *dpy, int screen, const char *font_name) {
    osd_visual = DefaultVisual(dpy, screen);
    osd_cmap = DefaultColormap(dpy, screen);
    if (font_name && *font_name) osd_font = XftFontOpenName(dpy, screen, font_name);
    if (!osd_font) osd_font = XftFontOpenName(dpy, screen, "monospace-14");
    if (!osd_font) osd_font = XftFontOpenName(dpy, screen, "sans-14");
    if (!osd_font) die("Failed to load font\n");
    if (!XftColorAllocName(dpy, osd_visual, osd_cmap, "#00FF00", &osd_color)) {
        die("Failed to alloc Xft color\n");
    }
}

void init_osd_shader() {
    const char *vsrc = 
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "layout(location = 1) in vec2 aTex;\n"
        "uniform vec2 screenSize;\n"
        "out vec2 vTex;\n"
        "void main() {\n"
        "    vec2 ndc = (aPos / screenSize) * 2.0 - 1.0;\n"
        "    ndc.y = -ndc.y;\n"
        "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
        "    vTex = aTex;\n"
        "}\n";
    const char *fsrc = 
        "#version 330 core\n"
        "in vec2 vTex;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D tex;\n"
        "void main() {\n"
        "    FragColor = texture(tex, vTex);\n"
        "}\n";
    
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsrc, NULL); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsrc, NULL); glCompileShader(fs);
    osd_program = glCreateProgram();
    glAttachShader(osd_program, vs); glAttachShader(osd_program, fs);
    glLinkProgram(osd_program);
    glDeleteShader(vs); glDeleteShader(fs);

    glGenVertexArrays(1, &osd_vao);
    glGenBuffers(1, &osd_vbo);
    glBindVertexArray(osd_vao);
    glBindBuffer(GL_ARRAY_BUFFER, osd_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

static int overlay_until = 0;
static char overlay_msg[256] = {0};

void draw_text_overlay(Display *dpy, Window win, const char *text, int window_w, int window_h, int margin) {
    UNUSED(win);
    if (!text || !*text || !osd_font || !osd_program) return;

    if (strcmp(last_osd_text, text) != 0) {
        if (osd_texture) glDeleteTextures(1, &osd_texture);

        int len = strlen(text);
        XGlyphInfo ext;
        XftTextExtentsUtf8(dpy, osd_font, (FcChar8 *)text, len, &ext);
        int tex_w = ext.xOff + 20;
        int tex_h = osd_font->ascent + osd_font->descent + 10;

        Pixmap pix = XCreatePixmap(dpy, DefaultRootWindow(dpy), tex_w, tex_h, 24);
        GC gc = XCreateGC(dpy, pix, 0, NULL);
        
        // 背景色（黑）
        XSetForeground(dpy, gc, 0x000000);
        XFillRectangle(dpy, pix, gc, 0, 0, tex_w, tex_h);

        // 画字
        XftDraw *xftdraw = XftDrawCreate(dpy, pix, osd_visual, osd_cmap);
        XftDrawStringUtf8(xftdraw, &osd_color, osd_font, 10, osd_font->ascent + 5, (FcChar8 *)text, len);
        
        XImage *img = XGetImage(dpy, pix, 0, 0, tex_w, tex_h, AllPlanes, ZPixmap);

        glGenTextures(1, &osd_texture);
        glBindTexture(GL_TEXTURE_2D, osd_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->data);

        XDestroyImage(img);
        XftDrawDestroy(xftdraw);
        XFreeGC(dpy, gc);
        XFreePixmap(dpy, pix);

        snprintf(last_osd_text, sizeof(last_osd_text), "%s", text);
    }

    if (!osd_texture) return;

    GLint tex_w, tex_h;
    glBindTexture(GL_TEXTURE_2D, osd_texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_h);

    // 像 Emacs Mode Line 一样，放在整个窗口最底部（留 margin 像素边距）
    int x = margin;
    int y = window_h - tex_h - margin;
    
    // 修正了 UV 映射：XImage 是 Top-Left 原点，映射到 GL 的顶点时左上角对应 (0,0) 即可。
    float verts[] = {
        (float)x, (float)y, 0.0f, 0.0f,         // 左上角
        (float)(x + tex_w), (float)y, 1.0f, 0.0f, // 右上角
        (float)(x + tex_w), (float)(y + tex_h), 1.0f, 1.0f, // 右下角

        (float)x, (float)y, 0.0f, 0.0f,         // 左上角
        (float)(x + tex_w), (float)(y + tex_h), 1.0f, 1.0f, // 右下角
        (float)x, (float)(y + tex_h), 0.0f, 1.0f  // 左下角
    };

    glUseProgram(osd_program);
    glUniform2f(glGetUniformLocation(osd_program, "screenSize"), window_w, window_h);
    glUniform1i(glGetUniformLocation(osd_program, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, osd_texture);

    glBindVertexArray(osd_vao);
    glBindBuffer(GL_ARRAY_BUFFER, osd_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

/* === Centered help box === */
static bool help_visible = false;
static GLuint help_texture = 0;
static int help_tex_w = 0, help_tex_h = 0;

void free_help_texture(void) {
    if (help_texture) { glDeleteTextures(1, &help_texture); help_texture = 0; }
    help_tex_w = help_tex_h = 0;
}

/* Build one big texture containing the title and one line per action. */
void build_help_texture(Display *dpy, const char *title, char lines[][128], int n) {
    free_help_texture();
    if (!osd_font || n <= 0) return;

    int pad_x = 24, pad_y = 16;
    int line_h = osd_font->ascent + osd_font->descent + 6;

    int max_w = 0;
    XGlyphInfo ext;
    XftTextExtentsUtf8(dpy, osd_font, (FcChar8 *)title, strlen(title), &ext);
    max_w = MAX(max_w, ext.xOff);
    for (int i = 0; i < n; i++) {
        XftTextExtentsUtf8(dpy, osd_font, (FcChar8 *)lines[i], strlen(lines[i]), &ext);
        max_w = MAX(max_w, ext.xOff);
    }

    help_tex_w = max_w + pad_x * 2;
    help_tex_h = pad_y * 2 + line_h * (n + 1);

    Pixmap pix = XCreatePixmap(dpy, DefaultRootWindow(dpy), help_tex_w, help_tex_h, 24);
    GC gc = XCreateGC(dpy, pix, 0, NULL);
    XSetForeground(dpy, gc, 0x101010);
    XFillRectangle(dpy, pix, gc, 0, 0, help_tex_w, help_tex_h);

    XftDraw *xftdraw = XftDrawCreate(dpy, pix, osd_visual, osd_cmap);
    int y = pad_y + osd_font->ascent;
    XftDrawStringUtf8(xftdraw, &osd_color, osd_font, pad_x, y, (FcChar8 *)title, strlen(title));
    for (int i = 0; i < n; i++) {
        y += line_h;
        XftDrawStringUtf8(xftdraw, &osd_color, osd_font, pad_x, y, (FcChar8 *)lines[i], strlen(lines[i]));
    }

    XImage *img = XGetImage(dpy, pix, 0, 0, help_tex_w, help_tex_h, AllPlanes, ZPixmap);
    glGenTextures(1, &help_texture);
    glBindTexture(GL_TEXTURE_2D, help_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, help_tex_w, help_tex_h, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->data);

    XDestroyImage(img);
    XftDrawDestroy(xftdraw);
    XFreeGC(dpy, gc);
    XFreePixmap(dpy, pix);
}

void draw_help_box(Display *dpy, Window win, int window_w, int window_h) {
    UNUSED(dpy); UNUSED(win);
    if (!help_texture || !osd_program) return;
    int x = (window_w - help_tex_w) / 2;
    int y = (window_h - help_tex_h) / 2;

    float verts[] = {
        (float)x, (float)y, 0.0f, 0.0f,
        (float)(x + help_tex_w), (float)y, 1.0f, 0.0f,
        (float)(x + help_tex_w), (float)(y + help_tex_h), 1.0f, 1.0f,
        (float)x, (float)y, 0.0f, 0.0f,
        (float)(x + help_tex_w), (float)(y + help_tex_h), 1.0f, 1.0f,
        (float)x, (float)(y + help_tex_h), 0.0f, 1.0f
    };

    glUseProgram(osd_program);
    glUniform2f(glGetUniformLocation(osd_program, "screenSize"), window_w, window_h);
    glUniform1i(glGetUniformLocation(osd_program, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, help_texture);
    glBindVertexArray(osd_vao);
    glBindBuffer(GL_ARRAY_BUFFER, osd_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

typedef struct { GLfloat x; GLfloat y; } Vec2f;
#define UNIT (Vec2f){1.0f, 1.0f}
#define ZERO (Vec2f){0.0f, 0.0f}
#define MUL(v, u)   (Vec2f){ (v).x*(u).x, (v).y*(u).y }
#define DIV(v, u)   (Vec2f){ (v).x/(u).x, (v).y/(u).y }
#define ADD(v, u)   (Vec2f){ (v).x+(u).x, (v).y+(u).y }
#define SUB(v, u)   (Vec2f){ (v).x-(u).x, (v).y-(u).y }
#define EQ(v, u)    ((v).x==(u).x && (v).y==(u).y)
#define LEN(v)      sqrtf((v).x*(v).x + (v).y*(v).y)
#define NORM(v)     (LEN(v)==0.0f ? ZERO : DIV(v, LEN(v)))
#define MULS(v, s)  (Vec2f){ (v).x*(s), (v).y*(s) }
#define DIVS(v, s)  (Vec2f){ (v).x/(s), (v).y/(s) }
#define ADDS(v, s)  (Vec2f){ (v).x+(s), (v).y+(s) }
#define SUBS(v, s)  (Vec2f){ (v).x-(s), (v).y-(s) }

#if defined(__has_embed)
#  if __has_embed("vertex.glsl") && __has_embed("fragment.glsl") && __has_embed("config.lua")
#    define HAS_EMBED 1
#  else
#    define HAS_EMBED 0
#  endif
#elif defined(__clang__) && __clang_major__ >= 16
#  define HAS_EMBED 1
#else
#  define HAS_EMBED 0
#endif

#if HAS_EMBED
static const unsigned char vert_src[] = {
#embed "vertex.glsl"
    , '\0'};
static const unsigned char frag_src[] = {
#embed "fragment.glsl"
    , '\0'};
static const unsigned char embedded_config[] = {
#embed "config.lua"
    , '\0'};
#else
static const unsigned char vert_src[] = "";
static const unsigned char frag_src[] = "";
static const unsigned char embedded_config[] = "";
#endif

typedef struct {
  float min_scale; float max_scale; float drag_friction; float scale_friction;
  float scroll_speed; float key_move_speed; bool windowed;
  char *fragment_shader_file; char *vertex_shader_file;
  float popup_timeout;   // seconds a popup stays visible
  int popup_margin;      // px from bottom edge
  char *output_path;     // screenshot save path (NULL = $HOME)
  bool debug;            // debug logging
  char *font_name;       // Xft font pattern
} Config;

typedef struct {
  Vec2f position; Vec2f velocity; Vec2f scale_pivot;
  GLfloat scale; GLfloat delta_scale; float dt;
} Camera;

typedef struct { bool is_enabled; GLfloat shadow; GLfloat radius; GLfloat delta_radius; } Flashlight;
typedef struct { Vec2f current; Vec2f previous; bool dragging; } Mouse;

static Config config;
static bool debug_mode = false;

typedef enum {
  ACT_MOVE_LEFT, ACT_MOVE_DOWN, ACT_MOVE_UP, ACT_MOVE_RIGHT,
  ACT_ZOOM_OUT, ACT_ZOOM_IN, ACT_QUIT, ACT_RESET,
  ACT_RELOAD, ACT_FLASHLIGHT, ACT_SAVE, ACT_HELP,
  ACT_COUNT
} ActionId;

typedef struct {
  char *help;
} Action;

typedef struct {
  KeySym keysym;
  int action;
} Binding;

static Action actions[ACT_COUNT];
static Binding bindings[64];
static int binding_count = 0;
static bool custom_keys_found = false;
static bool custom_help_found = false;

static void add_binding(KeySym sym, int act) {
  if (binding_count < (int)(sizeof(bindings) / sizeof(bindings[0])))
    bindings[binding_count++] = (Binding){sym, act};
}

static KeySym keyname_to_keysym(const char *name) {
  if (!name) return NoSymbol;
  if (strlen(name) == 1) {
    char c = tolower(name[0]);
    if (c >= 'a' && c <= 'z') return XK_a + (c - 'a');
    if (c >= '0' && c <= '9') return XK_0 + (c - '0');
  }
  if (!strcmp(name, "left")) return XK_Left;
  if (!strcmp(name, "right")) return XK_Right;
  if (!strcmp(name, "up")) return XK_Up;
  if (!strcmp(name, "down")) return XK_Down;
  if (!strcmp(name, "minus")) return XK_minus;
  if (!strcmp(name, "equal")) return XK_equal;
  if (!strcmp(name, "plus")) return XK_plus;
  if (!strcmp(name, "question")) return XK_question;
  if (!strcmp(name, "slash")) return XK_slash;
  if (!strcmp(name, "escape")) return XK_Escape;
  if (!strcmp(name, "space")) return XK_space;
  if (!strcmp(name, "tab")) return XK_Tab;
  if (!strcmp(name, "enter")) return XK_Return;
  if (!strcmp(name, "return")) return XK_Return;
  if (!strcmp(name, "backspace")) return XK_BackSpace;
  if (!strcmp(name, "delete")) return XK_Delete;
  if (!strcmp(name, "home")) return XK_Home;
  if (!strcmp(name, "end")) return XK_End;
  if (!strcmp(name, "pageup")) return XK_Page_Up;
  if (!strcmp(name, "pagedown")) return XK_Page_Down;
  for (int i = 1; i <= 12; i++) {
    char fn[8]; snprintf(fn, sizeof(fn), "f%d", i);
    if (!strcmp(name, fn)) return XK_F1 + (i - 1);
  }
  return NoSymbol;
}

static const char *action_key_name(int act) {
  switch (act) {
    case ACT_MOVE_LEFT: return "move_left";
    case ACT_MOVE_DOWN: return "move_down";
    case ACT_MOVE_UP: return "move_up";
    case ACT_MOVE_RIGHT: return "move_right";
    case ACT_ZOOM_OUT: return "zoom_out";
    case ACT_ZOOM_IN: return "zoom_in";
    case ACT_QUIT: return "quit";
    case ACT_RESET: return "reset";
    case ACT_RELOAD: return "reload";
    case ACT_FLASHLIGHT: return "flashlight";
    case ACT_SAVE: return "save";
    case ACT_HELP: return "help";
    default: return "unknown";
  }
}

static void keysym_to_keyname(KeySym sym, char *buf, size_t n) {
  if (sym >= XK_a && sym <= XK_z) { snprintf(buf, n, "%c", (char)('a' + (sym - XK_a))); return; }
  if (sym >= XK_0 && sym <= XK_9) { snprintf(buf, n, "%c", (char)('0' + (sym - XK_0))); return; }
  const char *name = "?";
  switch (sym) {
    case XK_Left: name = "Left"; break;
    case XK_Right: name = "Right"; break;
    case XK_Up: name = "Up"; break;
    case XK_Down: name = "Down"; break;
    case XK_minus: name = "-"; break;
    case XK_equal: name = "="; break;
    case XK_plus: name = "+"; break;
    case XK_question: name = "?"; break;
    case XK_slash: name = "/"; break;
    case XK_Escape: name = "Esc"; break;
    case XK_space: name = "Space"; break;
    case XK_Tab: name = "Tab"; break;
    case XK_Return: name = "Enter"; break;
    case XK_BackSpace: name = "Bksp"; break;
    case XK_Delete: name = "Del"; break;
    case XK_Home: name = "Home"; break;
    case XK_End: name = "End"; break;
    case XK_Page_Up: name = "PgUp"; break;
    case XK_Page_Down: name = "PgDn"; break;
    default:
      for (int i = 1; i <= 12; i++) {
        if (sym == (KeySym)(XK_F1 + (i - 1))) { snprintf(buf, n, "F%d", i); return; }
      }
      break;
  }
  snprintf(buf, n, "%s", name);
}

static void first_keyname_for_action(int act, char *buf, size_t n) {
  for (int i = 0; i < binding_count; i++) {
    if (bindings[i].action == act) { keysym_to_keyname(bindings[i].keysym, buf, n); return; }
  }
  snprintf(buf, n, "?");
}

void init_default_actions(void) {
  for (int i = 0; i < ACT_COUNT; i++) {
    free(actions[i].help);
    actions[i].help = NULL;
  }
  actions[ACT_MOVE_LEFT].help = strdup("Move view");
  actions[ACT_MOVE_DOWN].help = strdup("Move view");
  actions[ACT_MOVE_UP].help = strdup("Move view");
  actions[ACT_MOVE_RIGHT].help = strdup("Move view");
  actions[ACT_ZOOM_OUT].help = strdup("Zoom out");
  actions[ACT_ZOOM_IN].help = strdup("Zoom in");
  actions[ACT_QUIT].help = strdup("Quit");
  actions[ACT_RESET].help = strdup("Reset view");
  actions[ACT_RELOAD].help = strdup("Reload shaders");
  actions[ACT_FLASHLIGHT].help = strdup("Toggle flashlight");
  actions[ACT_SAVE].help = strdup("Save screenshot");
  actions[ACT_HELP].help = strdup("Show help");

  binding_count = 0;
  add_binding(XK_h, ACT_MOVE_LEFT);
  add_binding(XK_Left, ACT_MOVE_LEFT);
  add_binding(XK_j, ACT_MOVE_DOWN);
  add_binding(XK_Down, ACT_MOVE_DOWN);
  add_binding(XK_k, ACT_MOVE_UP);
  add_binding(XK_Up, ACT_MOVE_UP);
  add_binding(XK_l, ACT_MOVE_RIGHT);
  add_binding(XK_Right, ACT_MOVE_RIGHT);
  add_binding(XK_minus, ACT_ZOOM_OUT);
  add_binding(XK_equal, ACT_ZOOM_IN);
  add_binding(XK_q, ACT_QUIT);
  add_binding(XK_Escape, ACT_QUIT);
  add_binding(XK_0, ACT_RESET);
  add_binding(XK_r, ACT_RELOAD);
  add_binding(XK_f, ACT_FLASHLIGHT);
  add_binding(XK_s, ACT_SAVE);
  add_binding(XK_question, ACT_HELP);
}

void free_actions(void) {
  for (int i = 0; i < ACT_COUNT; i++) {
    free(actions[i].help);
    actions[i].help = NULL;
  }
}

Config get_default_config(void) {
  return (Config){
      .min_scale = 0.1, .max_scale = 6.0, .drag_friction = 6.0,
      .scale_friction = 4.0, .scroll_speed = 1.5, .key_move_speed = 400.0,
      .windowed = false, .vertex_shader_file = NULL, .fragment_shader_file = NULL,
      .popup_timeout = 3.0f, .popup_margin = 8,
      .output_path = NULL, .debug = false, .font_name = NULL,
  };
}

static void lua_read_float(lua_State *L, const char *name, float *field) {
  lua_getglobal(L, name);
  if (lua_isnumber(L, -1)) *field = (float)lua_tonumber(L, -1);
  lua_pop(L, 1);
}

static void lua_read_bool(lua_State *L, const char *name, bool *field) {
  lua_getglobal(L, name);
  if (lua_isboolean(L, -1)) *field = lua_toboolean(L, -1);
  lua_pop(L, 1);
}

static void lua_read_int(lua_State *L, const char *name, int *field) {
  lua_getglobal(L, name);
  if (lua_isinteger(L, -1)) *field = (int)lua_tointeger(L, -1);
  else if (lua_isnumber(L, -1)) *field = (int)lua_tonumber(L, -1);
  lua_pop(L, 1);
}

static void lua_read_string(lua_State *L, const char *name, char **field) {
  lua_getglobal(L, name);
  if (lua_isstring(L, -1)) { free(*field); *field = strdup(lua_tostring(L, -1)); }
  lua_pop(L, 1);
}

static void lua_read_keys(lua_State *L, const char *table_name, bool *found) {
  lua_getglobal(L, table_name);
  if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }
  *found = true;
  /* Custom keys must define every action: wipe defaults first. */
  binding_count = 0;
  for (int i = 0; i < ACT_COUNT; i++) {
    const char *aname = action_key_name(i);
    lua_getfield(L, -1, aname);
    if (lua_istable(L, -1)) {
      int len = (int)lua_rawlen(L, -1);
      for (int k = 1; k <= len; k++) {
        lua_rawgeti(L, -1, k);
        if (lua_isstring(L, -1)) {
          KeySym sym = keyname_to_keysym(lua_tostring(L, -1));
          if (sym != NoSymbol) add_binding(sym, i);
        }
        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

static void lua_read_help(lua_State *L, const char *table_name, bool *found) {
  lua_getglobal(L, table_name);
  if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }
  *found = true;
  /* Custom help must define every action: clear defaults first. */
  for (int i = 0; i < ACT_COUNT; i++) {
    free(actions[i].help);
    actions[i].help = NULL;
  }
  for (int i = 0; i < ACT_COUNT; i++) {
    const char *aname = action_key_name(i);
    lua_getfield(L, -1, aname);
    if (lua_isstring(L, -1)) {
      actions[i].help = strdup(lua_tostring(L, -1));
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

static void lua_load_config(Config *conf, const char *source, bool is_file) {
  lua_State *L = luaL_newstate();
  if (!L) die("Failed to create Lua state\n");
  luaL_openlibs(L);
  int err = is_file ? luaL_loadfile(L, source)
                    : luaL_loadbuffer(L, source, strlen(source), "embedded_config");
  if (err != LUA_OK) {
    const char *msg = lua_tostring(L, -1);
    lua_close(L);
    die("Lua config load error: %s\n", msg ? msg : "unknown");
  }
  err = lua_pcall(L, 0, 0, 0);
  if (err != LUA_OK) {
    const char *msg = lua_tostring(L, -1);
    lua_close(L);
    die("Lua config exec error: %s\n", msg ? msg : "unknown");
  }

  lua_read_float(L, "min_scale", &conf->min_scale);
  lua_read_float(L, "max_scale", &conf->max_scale);
  lua_read_float(L, "drag_friction", &conf->drag_friction);
  lua_read_float(L, "scale_friction", &conf->scale_friction);
  lua_read_float(L, "scroll_speed", &conf->scroll_speed);
  lua_read_float(L, "key_move_speed", &conf->key_move_speed);
  lua_read_bool(L, "windowed", &conf->windowed);
  lua_read_float(L, "popup_timeout", &conf->popup_timeout);
  lua_read_int(L, "popup_margin", &conf->popup_margin);
  lua_read_bool(L, "debug", &conf->debug);
  lua_read_string(L, "output_path", &conf->output_path);
  lua_read_string(L, "font_name", &conf->font_name);
  lua_read_string(L, "vertex_shader", &conf->vertex_shader_file);
  lua_read_string(L, "fragment_shader", &conf->fragment_shader_file);

  lua_read_keys(L, "keys", &custom_keys_found);
  lua_read_help(L, "help", &custom_help_found);

  lua_close(L);

  if (conf->debug || debug_mode) {
    fprintf(stderr, "[config] loaded from %s\n", is_file ? source : "embedded");
    fprintf(stderr, "[config] min_scale=%.2f max_scale=%.2f drag=%.2f scale_fr=%.2f scroll=%.2f key_spd=%.2f\n",
            conf->min_scale, conf->max_scale, conf->drag_friction, conf->scale_friction,
            conf->scroll_speed, conf->key_move_speed);
    fprintf(stderr, "[config] windowed=%d popup_timeout=%.1f popup_margin=%d debug=%d\n",
            conf->windowed, conf->popup_timeout, conf->popup_margin, conf->debug);
    fprintf(stderr, "[config] output_path=%s font=%s vertex=%s fragment=%s\n",
            conf->output_path ? conf->output_path : "(null)",
            conf->font_name ? conf->font_name : "(null)",
            conf->vertex_shader_file ? conf->vertex_shader_file : "(null)",
            conf->fragment_shader_file ? conf->fragment_shader_file : "(null)");
    fprintf(stderr, "[config] custom_keys=%d custom_help=%d bindings=%d\n",
            custom_keys_found, custom_help_found, binding_count);
  }
}

static void validate_actions(void) {
  if (!custom_keys_found && !custom_help_found) return;
  if (custom_keys_found) {
    for (int i = 0; i < ACT_COUNT; i++) {
      bool found = false;
      for (int b = 0; b < binding_count; b++) {
        if (bindings[b].action == i) { found = true; break; }
      }
      if (!found) die("Missing key bind for action '%s' (all keys entries must be defined)\n", action_key_name(i));
    }
  }
  if (custom_help_found) {
    for (int i = 0; i < ACT_COUNT; i++) {
      if (!actions[i].help) die("Missing help text for action '%s' (all help entries must be defined)\n", action_key_name(i));
    }
  }
}

static void show_popup(const char *msg) {
  snprintf(overlay_msg, sizeof(overlay_msg), "%s", msg);
  overlay_until = time(NULL) + (int)config.popup_timeout;
}

Config load_config(void) {
  const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
  char *allocated_xdg = NULL;
  if (xdg_config_home == NULL) {
    const char *home = getenv("HOME");
    if (home == NULL) die("Error: HOME environment variable not set!\n");
    size_t bufsz = strlen(home) + strlen("/.config") + 1;
    allocated_xdg = malloc(bufsz);
    if (!allocated_xdg) die("Malloc failed for xdg path");
    snprintf(allocated_xdg, bufsz, "%s/.config", home);
    xdg_config_home = allocated_xdg;
  }

  char config_dir[MAX_PATH_SIZE - 15];
  snprintf(config_dir, sizeof(config_dir), "%s/%s", xdg_config_home, PROGRAM_NAME);
  if (mkdir(config_dir, 0755) == -1 && errno != EEXIST) {
    if (allocated_xdg) free(allocated_xdg);
    die("Failed to create directory '%s': %s", config_dir, strerror(errno));
  }

  char config_file[MAX_PATH_SIZE];
  snprintf(config_file, MAX_PATH_SIZE, "%s/config.lua", config_dir);
  if (access(config_file, F_OK | R_OK)) {
    strncpy(config_file, "./config.lua", sizeof(config_file) - 1);
    config_file[MAX_PATH_SIZE - 1] = '\0';
  }

  Config conf = get_default_config();
  FILE *f = fopen(config_file, "r");
  if (f != NULL) { fclose(f); lua_load_config(&conf, config_file, true); }
  else {
#if HAS_EMBED
    lua_load_config(&conf, (const char *)embedded_config, false);
#else
    die("Config file missing and #embed unavailable, cannot load config\n");
#endif
  }

  // Resolve shader paths: config.lua wins, else config dir, else cwd, else embedded
  if (!conf.vertex_shader_file) {
    char *p = malloc(MAX_PATH_SIZE);
    snprintf(p, MAX_PATH_SIZE, "%s/vertex.glsl", config_dir);
    FILE *tf = fopen(p, "r");
    if (tf) fclose(tf);
    else { strncpy(p, "./vertex.glsl", MAX_PATH_SIZE - 1); p[MAX_PATH_SIZE - 1] = '\0'; }
    conf.vertex_shader_file = p;
  }
  if (!conf.fragment_shader_file) {
    char *p = malloc(MAX_PATH_SIZE);
    snprintf(p, MAX_PATH_SIZE, "%s/fragment.glsl", config_dir);
    FILE *tf = fopen(p, "r");
    if (tf) fclose(tf);
    else { strncpy(p, "./fragment.glsl", MAX_PATH_SIZE - 1); p[MAX_PATH_SIZE - 1] = '\0'; }
    conf.fragment_shader_file = p;
  }

  if (allocated_xdg) free(allocated_xdg);
  return conf;
}

void update_flashlight(Flashlight *fl, float dt) {
  if (fabsf(fl->delta_radius) > 1.0f) {
    fl->radius = MAX(0.0f, fl->radius + fl->delta_radius * dt);
    fl->delta_radius -= fl->delta_radius * DELTA_RADIUS_DECEL * dt;
  }
  if (fl->is_enabled) fl->shadow = MIN(fl->shadow + SHADOW_ACCEL * dt, MAX_SHADOW);
  else fl->shadow = MAX(fl->shadow - SHADOW_ACCEL * dt, 0.0f);
}

void initialize_mouse(Display *dpy, Mouse *m) {
  Window _root, _child; int _win_x, _win_y; unsigned int _mask; int root_x, root_y;
  root_x = root_y = 0;
  XQueryPointer(dpy, DefaultRootWindow(dpy), &_root, &_child, &root_x, &root_y, &_win_x, &_win_y, &_mask);
  m->current = m->previous = (Vec2f){root_x, root_y}; m->dragging = false;
}

void update_camera(Camera *cam, Config *config, Mouse *mouse, Vec2f window_size) {
  if (fabsf(cam->delta_scale) > 0.5f) {
    Vec2f p0 = DIVS(SUB(cam->scale_pivot, MULS(window_size, 0.5)), cam->scale);
    cam->scale = CLAMP(config->min_scale, cam->scale + cam->delta_scale * cam->dt, config->max_scale);
    Vec2f p1 = DIVS(SUB(cam->scale_pivot, MULS(window_size, 0.5)), cam->scale);
    cam->position = ADD(cam->position, SUB(p0, p1));
    cam->delta_scale -= cam->delta_scale * cam->dt * config->scale_friction;
  }
  if (!mouse->dragging && LEN(cam->velocity) > VELOCITY_THRESHOLD) {
    cam->position = ADD(cam->position, MULS(cam->velocity, cam->dt));
    cam->velocity = SUB(cam->velocity, MULS(cam->velocity, cam->dt * config->drag_friction));
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
static GLuint main_texture = 0; // 保存主画面的纹理，防止状态污染

static char *cli_vertex_shader = NULL;
static char *cli_fragment_shader = NULL;
static char *cli_output_file = NULL;

void keypress(XEvent *);
void motion_notify(XEvent *);
void scroll_down(unsigned int, bool);
void scroll_up(unsigned int, bool);
void button_press(XEvent *);
void button_release(XEvent *);
static void (*handler[LASTEvent])(XEvent *) = {
    [MotionNotify] = motion_notify, [KeyPress] = keypress,
    [ButtonPress] = button_press, [ButtonRelease] = button_release,
};

static const char *try_load_file(const char *path, char **out_buf) {
  *out_buf = NULL; FILE *fp = fopen(path, "rb"); if (!fp) return NULL;
  fseek(fp, 0, SEEK_END); long sz = ftell(fp); rewind(fp);
  char *buf = malloc(sz + 1); if (!buf) { fclose(fp); return NULL; }
  size_t rd = fread(buf, 1, sz, fp); buf[rd] = '\0'; fclose(fp); *out_buf = buf;
  return buf;
}

GLuint load_shader(GLenum type, const char *file_path) {
  const GLchar *src = NULL; char *heap_buf = NULL;
#if HAS_EMBED
  if (!file_path || !*file_path) {
    if (type == GL_VERTEX_SHADER) src = (const GLchar *)vert_src;
    if (type == GL_FRAGMENT_SHADER) src = (const GLchar *)frag_src;
  }
#endif
  if (file_path && *file_path) {
    src = try_load_file(file_path, &heap_buf);
#if HAS_EMBED
    if (!src) {
      if (type == GL_VERTEX_SHADER) src = (const GLchar *)vert_src;
      if (type == GL_FRAGMENT_SHADER) src = (const GLchar *)frag_src;
    }
#endif
  }
  if (!src) die("No shader source available");
  GLuint shader = glCreateShader(type); glShaderSource(shader, 1, &src, NULL); glCompileShader(shader);
  int ok; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) { char log[1024]; glGetShaderInfoLog(shader, sizeof(log), NULL, log); die("Shader compile error:\n%s", log); }
  if (heap_buf) free(heap_buf);
  return shader;
}

void update_program(char *vertex_shader_filename, char *fragment_shader_filename) {
  GLuint vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_filename);
  GLuint fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_filename);
  GLuint new_program = glCreateProgram();
  glAttachShader(new_program, vertex_shader); glAttachShader(new_program, fragment_shader);
  glLinkProgram(new_program);
  int link_success; glGetProgramiv(new_program, GL_LINK_STATUS, &link_success);
  if (link_success) { glDeleteProgram(shader_program); shader_program = new_program; }
  else { GLchar info_log[512]; glGetProgramInfoLog(new_program, 512, NULL, info_log); fprintf(stderr, "Error linking:\n%s", info_log); }
  glDeleteShader(vertex_shader); glDeleteShader(fragment_shader);
}

void check_glx_version(Display *dpy) {
  int glx_major, glx_minor;
  if (!glXQueryVersion(dpy, &glx_major, &glx_minor) || (glx_major == MIN_GLX_MAJOR && glx_minor < MIN_GLX_MINOR) || (glx_major < 1))
    die("Invalid GLX version %d.%d.", glx_major, glx_minor);
}

XImage *get_screenshot(void) {
  Window root_win = DefaultRootWindow(dpy);
  return XGetImage(dpy, root_win, 0, 0, wa.width, wa.height, AllPlanes, ZPixmap);
}
void destroy_screenshot(XImage *screenshot) { XDestroyImage(screenshot); }

void draw_image(Camera *cam, GLuint shader, GLuint vao, Vec2f window_size,
                Vec2f screenshot_size, Mouse *mouse, Flashlight *fl) {
  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(shader);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, main_texture); // 恢复主纹理绑定
  glUniform2f(glGetUniformLocation(shader, "cameraPos"), cam->position.x, cam->position.y);
  glUniform1f(glGetUniformLocation(shader, "cameraScale"), cam->scale);
  glUniform2f(glGetUniformLocation(shader, "screenshotSize"), screenshot_size.x, screenshot_size.y);
  glUniform2f(glGetUniformLocation(shader, "windowSize"), window_size.x, window_size.y);
  glUniform2f(glGetUniformLocation(shader, "cursorPos"), mouse->current.x, mouse->current.y);
  glUniform1f(glGetUniformLocation(shader, "flShadow"), fl->shadow);
  glUniform1f(glGetUniformLocation(shader, "flRadius"), fl->radius);
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
  glBindVertexArray(0);
}

static void execute_action(int act, XEvent *e) {
  XKeyEvent *ev = (XKeyEvent *)&e->xkey;
  switch (act) {
  case ACT_MOVE_LEFT: camera.velocity.x -= config.key_move_speed; break;
  case ACT_MOVE_DOWN: camera.velocity.y += config.key_move_speed; break;
  case ACT_MOVE_UP: camera.velocity.y -= config.key_move_speed; break;
  case ACT_MOVE_RIGHT: camera.velocity.x += config.key_move_speed; break;
  case ACT_ZOOM_OUT: scroll_down(1, ev->state & ControlMask); break;
  case ACT_ZOOM_IN: scroll_up(1, ev->state & ControlMask); break;
  case ACT_QUIT: running = false; break;
  case ACT_RESET:
    camera.scale = 1.0f; camera.delta_scale = 0.0f;
    camera.velocity = camera.position = (Vec2f){0, 0}; break;
  case ACT_RELOAD: {
    /* Free every malloc'd config field so the reload fully replaces state. */
    free(config.vertex_shader_file); config.vertex_shader_file = NULL;
    free(config.fragment_shader_file); config.fragment_shader_file = NULL;
    free(config.output_path); config.output_path = NULL;
    free(config.font_name); config.font_name = NULL;
    free_actions(); init_default_actions();
    custom_keys_found = false; custom_help_found = false;
    config = load_config();
    validate_actions();
    if (cli_vertex_shader) { free(config.vertex_shader_file); config.vertex_shader_file = strdup(cli_vertex_shader); }
    if (cli_fragment_shader) { free(config.fragment_shader_file); config.fragment_shader_file = strdup(cli_fragment_shader); }
    update_program(config.vertex_shader_file, config.fragment_shader_file);
    free_help_texture(); /* keybinds may have changed */
    if (debug_mode) fprintf(stderr, "[reload] config reloaded\n");
    break;
  }
  case ACT_FLASHLIGHT: flashlight.is_enabled = !flashlight.is_enabled; break;
  case ACT_SAVE: {
    char final_path[4096];
    time_t t = time(NULL); struct tm *tm = localtime(&t);
    char timestamp[64]; strftime(timestamp, sizeof(timestamp), "screenshot-%Y%m%d-%H%M%S.png", tm);
    const char *out = cli_output_file ? cli_output_file : config.output_path;
    if (out) {
        struct stat st;
        if (stat(out, &st) == 0 && S_ISDIR(st.st_mode)) {
            snprintf(final_path, sizeof(final_path), "%s/%s", out, timestamp);
        } else {
            snprintf(final_path, sizeof(final_path), "%s", out);
        }
    } else {
        const char *home = getenv("HOME");
        if (home) snprintf(final_path, sizeof(final_path), "%s/%s", home, timestamp);
        else snprintf(final_path, sizeof(final_path), "%s", timestamp);
    }
    save_gl_framebuffer(final_path, wa.width, wa.height);
    snprintf(overlay_msg, sizeof(overlay_msg), "Saved: %s", final_path);
    overlay_until = time(NULL) + (int)config.popup_timeout;
    return;
  }
  case ACT_HELP: {
    help_visible = !help_visible;
    if (help_visible) {
      char lines[ACT_COUNT][128];
      for (int i = 0; i < ACT_COUNT; i++) {
        char keybuf[32];
        first_keyname_for_action(i, keybuf, sizeof(keybuf));
        snprintf(lines[i], sizeof(lines[i]), "%-6s  %s", keybuf,
                 actions[i].help ? actions[i].help : "");
      }
      build_help_texture(dpy, "Keybindings", lines, ACT_COUNT);
    } else {
      free_help_texture();
    }
    return;
  }
  }
  if (actions[act].help) show_popup(actions[act].help);
}

void keypress(XEvent *e) {
  KeySym keysym; XKeyEvent *ev = (XKeyEvent *)&e->xkey;
  keysym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, e->xkey.state & ShiftMask ? 1 : 0);
  for (int i = 0; i < binding_count; i++) {
    if (bindings[i].keysym == keysym) {
      execute_action(bindings[i].action, e);
      return;
    }
  }
}

void scroll_up(unsigned int delta, bool fl_enabled) {
  if (delta > 0 && fl_enabled) flashlight.delta_radius += 250.0f;
  else { camera.delta_scale += config.scroll_speed; camera.scale_pivot = mouse.current; }
}

void scroll_down(unsigned int delta, bool fl_enabled) {
  if (delta > 0 && fl_enabled) flashlight.delta_radius -= 250.0f;
  else { camera.delta_scale -= config.scroll_speed; camera.scale_pivot = mouse.current; }
}

void button_press(XEvent *e) {
  XButtonEvent *ev = (XButtonEvent *)&e->xbutton;
  switch (ev->button) {
  case Button1: mouse.previous = mouse.current; mouse.dragging = true; camera.velocity = ZERO; break;
  case Button4: scroll_up(ev->state & ControlMask, flashlight.is_enabled); break;
  case Button5: scroll_down(ev->state & ControlMask, flashlight.is_enabled); break;
  }
}

void button_release(XEvent *e) {
  XButtonEvent *ev = (XButtonEvent *)&e->xbutton;
  if (ev->button == Button1) mouse.dragging = false;
}

void motion_notify(XEvent *e) {
  XMotionEvent *ev = (XMotionEvent *)&e->xmotion;
  mouse.current = (Vec2f){ev->x, ev->y};
  if (mouse.dragging) {
    Vec2f delta = SUB(world(&camera, mouse.previous), world(&camera, mouse.current));
    camera.position = ADD(camera.position, delta); camera.velocity = MULS(delta, 1.0f / camera.dt);
  }
  mouse.previous = mouse.current;
}

static bool detect_wayland(void) {
  const char *xdg_type = getenv("XDG_SESSION_TYPE");
  if (xdg_type && !strcmp(xdg_type, "wayland")) return true;
  const char *wl_display = getenv("WAYLAND_DISPLAY");
  if (wl_display && *wl_display) return true;
  return false;
}

int main(int argc, char *argv[]) {
  static struct option long_options[] = {
      {"debug", no_argument, 0, 'd'}, {"output", required_argument, 0, 'o'},
      {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "hdo:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'd': debug_mode = true; break;
      case 'o': cli_output_file = optarg; break;
      case 'h':
        printf("boomer-c-1.0\nUsage: %s [options] [vertex.glsl] [fragment.glsl]\n"
               "Options:\n  -d, --debug          Print debug logging.\n"
               "  -o, --output <path>  Specify file or directory to save (default: $HOME).\n"
               "  -h, --help           Show help.\n"
               "\nInteractive Mode: Press 's' to save current view (with shaders applied).\n\n", argv[0]);
        return 0;
      default: fprintf(stderr, "Try '%s --help'\n", argv[0]); return 1;
    }
  }

  if (optind < argc) {
    if (argc - optind == 1) cli_fragment_shader = argv[optind];
    else if (argc - optind >= 2) { cli_vertex_shader = argv[optind]; cli_fragment_shader = argv[optind + 1]; }
  }

  init_default_actions();
  config = load_config();
  validate_actions();
  if (cli_vertex_shader) { free(config.vertex_shader_file); config.vertex_shader_file = strdup(cli_vertex_shader); }
  if (cli_fragment_shader) { free(config.fragment_shader_file); config.fragment_shader_file = strdup(cli_fragment_shader); }

  if (config.debug) debug_mode = true;
  bool use_wayland = false;
  if (detect_wayland()) {
    if (wl_screencopy_init() == 0) use_wayland = true;
    else if (debug_mode) fprintf(stderr, "Wayland detected but screencopy failed, using X11.\n");
  } else if (debug_mode) fprintf(stderr, "Detected X11 session, using X11 capture.\n");

  dpy = XOpenDisplay(NULL); if (dpy == NULL) die("Cannot connect to X display\n");
  check_glx_version(dpy); screen = DefaultScreen(dpy);
  
  init_xft(dpy, screen, config.font_name);

  GLint attrs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *vi = glXChooseVisual(dpy, screen, attrs); if (!vi) die("No visual\n");
  XSetWindowAttributes swa; memset(&swa, 0, sizeof(swa));
  swa.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), vi->visual, AllocNone);
  swa.event_mask = ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ClientMessage;
  if (!config.windowed) { swa.override_redirect = 1; swa.save_under = 1; }
  XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &wa);
  w = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, wa.width, wa.height, 0, vi->depth, InputOutput, vi->visual,
                    CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder, &swa);

  char *wm_name = WM_NAME; char *wm_class = WM_CLASS;
  XClassHint hints = {.res_name = wm_name, .res_class = wm_class};
  XStoreName(dpy, w, wm_name); XSetClassHint(dpy, w, &hints);
  Atom wm_delete_atom = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
  XSetWMProtocols(dpy, w, &wm_delete_atom, 1);
  
  GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE); glXMakeCurrent(dpy, w, glc);
  if (GLEW_OK != glewInit()) die("Failed to init GLEW\n");
  
  init_osd_shader();

  glViewport(0, 0, wa.width, wa.height);
  XSelectInput(dpy, w, ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask);
  int revert_to_parent; Window origin_win;
  XGetInputFocus(dpy, &origin_win, &revert_to_parent);
  update_program(config.vertex_shader_file, config.fragment_shader_file);

  WlScreencopyResult wl_res = {0}; XImage *screenshot = NULL;
  Vec2f window_size = (Vec2f){(float)wa.width, (float)wa.height};
  Vec2f screenshot_size; int tex_w = 0, tex_h = 0; void *tex_data = NULL; bool y_invert = false;

  if (use_wayland) {
    if (wl_screencopy_capture(&wl_res) != 0) { wl_screencopy_fini(); use_wayland = false; }
  }
  if (use_wayland) {
    screenshot_size = window_size; tex_w = (int)wl_res.width; tex_h = (int)wl_res.height;
    tex_data = wl_res.data; y_invert = wl_res.y_invert;
  } else {
    screenshot = get_screenshot(); screenshot_size = (Vec2f){(float)screenshot->width, (float)screenshot->height};
    tex_w = screenshot->width; tex_h = screenshot->height; tex_data = screenshot->data;
  }

  GLenum gl_format = GL_BGRA;
  const char *env_fmt = getenv("BOOMER_GL_FORMAT");
  if (env_fmt) {
    if (!strcmp(env_fmt, "RGBA")) gl_format = GL_RGBA;
    else if (!strcmp(env_fmt, "BGRA")) gl_format = GL_BGRA;
  } else if (use_wayland) {
    if (wl_res.format != 0 && wl_res.format != 1) gl_format = GL_RGBA;
  }

  XMapWindow(dpy, w); XSync(dpy, False);

  int sw = (int)screenshot_size.x; int sh = (int)screenshot_size.y;
  GLfloat ty0 = y_invert ? 0.0f : 1.0f; GLfloat ty1 = y_invert ? 1.0f : 0.0f;

  GLuint vbo, vao, ebo;
  GLfloat vertices[] = {sw, 0,  0.0, 1.0, ty0, sw, sh, 0.0, 1.0, ty1,
                        0,  sh, 0.0, 0.0, ty1, 0,  0,  0.0, 0.0, ty0};
  GLuint indices[] = {0, 1, 3, 1, 2, 3};
  glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  GLsizei stride = 5 * sizeof(GLfloat);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0); glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
  
  glGenTextures(1, &main_texture);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, main_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)tex_w, (GLsizei)tex_h, 0, gl_format, GL_UNSIGNED_BYTE, tex_data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);
  glEnable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  XRRScreenConfiguration *screen_config = XRRGetScreenInfo(dpy, DefaultRootWindow(dpy));
  float rate = 1.0f / XRRConfigCurrentRate(screen_config);

  flashlight = (Flashlight){ .is_enabled = false, .shadow = 0.0f, .radius = 200.0f, .delta_radius = 0.0f };
  camera = (Camera){ .position = ZERO, .velocity = ZERO, .scale_pivot = ZERO, .scale = 1.0f, .dt = rate };
  initialize_mouse(dpy, &mouse);

  bool grabbed = false;
  if (use_wayland) {
    XSync(dpy, False); XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
    XGrabKeyboard(dpy, w, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(dpy, w, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    XSync(dpy, False); grabbed = true;
  }

  XEvent e;
  while (running) {
    if (!config.windowed) XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
    while (XPending(dpy) > 0) {
      XNextEvent(dpy, &e);
      switch (e.type) {
      case ClientMessage: if ((Atom)e.xclient.data.l[0] == wm_delete_atom) running = false; break;
      case KeyPress: case ButtonPress: case ButtonRelease: case MotionNotify: handler[e.type](&e); break;
      default: break;
      }
    }
    update_flashlight(&flashlight, camera.dt);
    update_camera(&camera, &config, &mouse, screenshot_size);
    
    // 绘制底图及特效
    draw_image(&camera, shader_program, vao, window_size, screenshot_size, &mouse, &flashlight);
    
    // 在左下角像 Emacs Scratch 一样绘制提示栏
    if (time(NULL) < overlay_until) {
        draw_text_overlay(dpy, w, overlay_msg, wa.width, wa.height, config.popup_margin);
    }

    if (help_visible) draw_help_box(dpy, w, wa.width, wa.height);

    glXSwapBuffers(dpy, w); glFinish();
  }

  if (grabbed) { XUngrabKeyboard(dpy, CurrentTime); XUngrabPointer(dpy, CurrentTime); }
  if (use_wayland) { wl_screencopy_release(&wl_res); wl_screencopy_fini(); }
  else if (screenshot) { destroy_screenshot(screenshot); }

  free_actions();
  free(config.vertex_shader_file); free(config.fragment_shader_file);
  free(config.output_path); free(config.font_name);
  XSetInputFocus(dpy, origin_win, RevertToParent, CurrentTime);
  glXMakeCurrent(dpy, None, NULL); glXDestroyContext(dpy, glc);
  XCloseDisplay(dpy); return 0;
}