// SPDX-License-Identifier: MIT
/*
 * Compton - a compositor for X11
 *
 * Based on `xcompmgr` - Copyright (c) 2003, Keith Packard
 *
 * Copyright (c) 2011-2013, Christopher Jeffrey
 * Copyright (c) 2018, Yuxuan Shui <yshuiv7@gmail.com>
 *
 * See LICENSE-mit for more information.
 *
 */

#pragma once

// === Options ===

// Debug options, enable them using -D in CFLAGS
// #define DEBUG_BACKTRACE  1
// #define DEBUG_REPAINT    1
// #define DEBUG_EVENTS     1
// #define DEBUG_RESTACK    1
// #define DEBUG_WINTYPE    1
// #define DEBUG_CLIENTWIN  1
// #define DEBUG_WINDATA    1
// #define DEBUG_WINMATCH   1
// #define DEBUG_REDIR      1
// #define DEBUG_ALLOC_REG  1
// #define DEBUG_FRAME      1
// #define DEBUG_LEADER     1
// #define DEBUG_C2         1
// #define DEBUG_GLX        1
// #define DEBUG_GLX_GLSL   1
// #define DEBUG_GLX_ERR    1
// #define DEBUG_GLX_MARK   1
// #define DEBUG_GLX_PAINTREG 1

// Whether to enable PCRE regular expression support in blacklists, enabled
// by default
// #define CONFIG_REGEX_PCRE 1
// Whether to enable JIT support of libpcre. This may cause problems on PaX
// kernels.
// #define CONFIG_REGEX_PCRE_JIT 1
// Whether to enable parsing of configuration files using libconfig.
// #define CONFIG_LIBCONFIG 1
// Whether to enable DRM VSync support
// #define CONFIG_VSYNC_DRM 1
// Whether to enable OpenGL support (include GLSL, FBO)
// #define CONFIG_OPENGL 1
// Whether to enable DBus support with libdbus.
// #define CONFIG_DBUS 1
// Whether to enable X Sync support.
// #define CONFIG_XSYNC 1
// Whether to enable GLX Sync support.
// #define CONFIG_GLX_XSYNC 1

#ifndef COMPTON_VERSION
#define COMPTON_VERSION "unknown"
#endif

#if defined(DEBUG_ALLOC_REG)
#define DEBUG_BACKTRACE 1
#endif

#define MAX_ALPHA (255)

// === Includes ===

// For some special functions
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>

#include <xcb/composite.h>
#include <xcb/render.h>
#include <xcb/damage.h>
#include <xcb/randr.h>
#include <xcb/shape.h>

#ifdef CONFIG_XINERAMA
#include <xcb/xinerama.h>
#endif
#include <ev.h>
#include <pixman.h>

// libdbus
#ifdef CONFIG_DBUS
#include <dbus/dbus.h>
#endif

#ifdef CONFIG_OPENGL
// libGL
#define GL_GLEXT_PROTOTYPES

#include <GL/glx.h>

// Workarounds for missing definitions in some broken GL drivers, thanks to
// douglasp and consolers for reporting
#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif

#ifndef GLX_BACK_BUFFER_AGE_EXT
#define GLX_BACK_BUFFER_AGE_EXT 0x20F4
#endif

#endif

// === Macros ===

#define MSTR_(s)        #s
#define MSTR(s)         MSTR_(s)

// Use #s here to prevent macro expansion
/// Macro used for shortening some debugging code.
#define CASESTRRET(s)   case s: return #s

// X resource checker
#ifdef DEBUG_XRC
#include "xrescheck.h"
#endif

// FIXME This list of includes should get shorter
#include "types.h"
#include "win.h"
#include "x.h"
#include "region.h"
#include "log.h"
#include "utils.h"
#include "compiler.h"
#include "kernel.h"

// === Constants ===

/// @brief Length of generic buffers.
#define BUF_LEN 80

#define ROUNDED_PERCENT 0.05
#define ROUNDED_PIXELS  10

#define OPAQUE 0xffffffff
#define REGISTER_PROP "_NET_WM_CM_S"

#define TIME_MS_MAX LONG_MAX
#define FADE_DELTA_TOLERANCE 0.2
#define SWOPTI_TOLERANCE 3000
#define WIN_GET_LEADER_MAX_RECURSION 20

#define SEC_WRAP (15L * 24L * 60L * 60L)

#define NS_PER_SEC 1000000000L
#define US_PER_SEC 1000000L
#define MS_PER_SEC 1000

#define XRFILTER_CONVOLUTION  "convolution"
#define XRFILTER_GAUSSIAN     "gaussian"
#define XRFILTER_BINOMIAL     "binomial"

/// @brief Maximum OpenGL FBConfig depth.
#define OPENGL_MAX_DEPTH 32

/// @brief Maximum OpenGL buffer age.
#define CGLX_MAX_BUFFER_AGE 5

/// @brief Maximum passes for blur.
#define MAX_BLUR_PASS 5

// Window flags

// Window size is changed
#define WFLAG_SIZE_CHANGE   0x0001
// Window size/position is changed
#define WFLAG_POS_CHANGE    0x0002
// Window opacity / dim state changed
#define WFLAG_OPCT_CHANGE   0x0004

// xcb-render specific macros
#define XFIXED_TO_DOUBLE(value) (((double) (value)) / 65536)
#define DOUBLE_TO_XFIXED(value) ((xcb_render_fixed_t) (((double) (value)) * 65536))

// === Types ===

typedef long time_ms_t;
typedef struct _c2_lptr c2_lptr_t;

/// Structure representing needed window updates.
typedef struct {
  bool shadow       : 1;
  bool fade         : 1;
  bool focus        : 1;
  bool invert_color : 1;
} win_upd_t;

typedef struct _ignore {
  struct _ignore *next;
  unsigned long sequence;
} ignore_t;

enum wincond_target {
  CONDTGT_NAME,
  CONDTGT_CLASSI,
  CONDTGT_CLASSG,
  CONDTGT_ROLE,
};

enum wincond_type {
  CONDTP_EXACT,
  CONDTP_ANYWHERE,
  CONDTP_FROMSTART,
  CONDTP_WILDCARD,
  CONDTP_REGEX_PCRE,
};

#define CONDF_IGNORECASE 0x0001

/// VSync modes.
typedef enum {
  VSYNC_NONE,
  VSYNC_DRM,
  VSYNC_OPENGL,
  VSYNC_OPENGL_OML,
  VSYNC_OPENGL_SWC,
  VSYNC_OPENGL_MSWC,
  NUM_VSYNC,
} vsync_t;

/// @brief Possible backends of compton.
enum backend {
  BKEND_XRENDER,
  BKEND_GLX,
  BKEND_XR_GLX_HYBRID,
  NUM_BKEND,
};

/// @brief Possible swap methods.
enum {
  SWAPM_BUFFER_AGE = -1,
  SWAPM_UNDEFINED = 0,
  SWAPM_COPY = 1,
  SWAPM_EXCHANGE = 2,
};

typedef struct _glx_texture glx_texture_t;

#ifdef CONFIG_OPENGL
#ifdef DEBUG_GLX_DEBUG_CONTEXT
typedef GLXContext (*f_glXCreateContextAttribsARB) (Display *dpy,
    GLXFBConfig config, GLXContext share_context, Bool direct,
    const int *attrib_list);
typedef void (*GLDEBUGPROC) (GLenum source, GLenum type,
    GLuint id, GLenum severity, GLsizei length, const GLchar* message,
    GLvoid* userParam);
typedef void (*f_DebugMessageCallback) (GLDEBUGPROC, void *userParam);
#endif

typedef int (*f_WaitVideoSync) (int, int, unsigned *);
typedef int (*f_GetVideoSync) (unsigned *);

typedef Bool (*f_GetSyncValuesOML) (Display* dpy, GLXDrawable drawable, int64_t* ust, int64_t* msc, int64_t* sbc);
typedef Bool (*f_WaitForMscOML) (Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t* ust, int64_t* msc, int64_t* sbc);

typedef int (*f_SwapIntervalSGI) (int interval);
typedef int (*f_SwapIntervalMESA) (unsigned int interval);

typedef void (*f_BindTexImageEXT) (Display *display, GLXDrawable drawable, int buffer, const int *attrib_list);
typedef void (*f_ReleaseTexImageEXT) (Display *display, GLXDrawable drawable, int buffer);

#ifdef CONFIG_OPENGL
// Looks like duplicate typedef of the same type is safe?
typedef int64_t GLint64;
typedef uint64_t GLuint64;
typedef struct __GLsync *GLsync;

#ifndef GL_SYNC_FLUSH_COMMANDS_BIT
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

#ifndef GL_ALREADY_SIGNALED
#define GL_ALREADY_SIGNALED 0x911A
#endif

#ifndef GL_TIMEOUT_EXPIRED
#define GL_TIMEOUT_EXPIRED 0x911B
#endif

#ifndef GL_CONDITION_SATISFIED
#define GL_CONDITION_SATISFIED 0x911C
#endif

#ifndef GL_WAIT_FAILED
#define GL_WAIT_FAILED 0x911D
#endif

typedef GLsync (*f_FenceSync) (GLenum condition, GLbitfield flags);
typedef GLboolean (*f_IsSync) (GLsync sync);
typedef void (*f_DeleteSync) (GLsync sync);
typedef GLenum (*f_ClientWaitSync) (GLsync sync, GLbitfield flags,
    GLuint64 timeout);
typedef void (*f_WaitSync) (GLsync sync, GLbitfield flags,
    GLuint64 timeout);
typedef GLsync (*f_ImportSyncEXT) (GLenum external_sync_type,
    GLintptr external_sync, GLbitfield flags);
#endif

#ifdef DEBUG_GLX_MARK
typedef void (*f_StringMarkerGREMEDY) (GLsizei len, const void *string);
typedef void (*f_FrameTerminatorGREMEDY) (void);
#endif

/// @brief Wrapper of a GLX FBConfig.
typedef struct {
  GLXFBConfig cfg;
  GLint texture_fmt;
  GLint texture_tgts;
  bool y_inverted;
} glx_fbconfig_t;

/// @brief Wrapper of a binded GLX texture.
struct _glx_texture {
  GLuint texture;
  GLXPixmap glpixmap;
  xcb_pixmap_t pixmap;
  GLenum target;
  unsigned width;
  unsigned height;
  unsigned depth;
  bool y_inverted;
};

#ifdef CONFIG_OPENGL
typedef struct {
  /// Fragment shader for blur.
  GLuint frag_shader;
  /// GLSL program for blur.
  GLuint prog;
  /// Location of uniform "offset_x" in blur GLSL program.
  GLint unifm_offset_x;
  /// Location of uniform "offset_y" in blur GLSL program.
  GLint unifm_offset_y;
  /// Location of uniform "factor_center" in blur GLSL program.
  GLint unifm_factor_center;
} glx_blur_pass_t;

typedef struct glx_prog_main {
  /// GLSL program.
  GLuint prog;
  /// Location of uniform "opacity" in window GLSL program.
  GLint unifm_opacity;
  /// Location of uniform "invert_color" in blur GLSL program.
  GLint unifm_invert_color;
  /// Location of uniform "tex" in window GLSL program.
  GLint unifm_tex;
} glx_prog_main_t;

#define GLX_PROG_MAIN_INIT { \
  .prog = 0, \
  .unifm_opacity = -1, \
  .unifm_invert_color = -1, \
  .unifm_tex = -1, \
}

#endif
#else
struct glx_prog_main { };
#endif

#define PAINT_INIT { .pixmap = None, .pict = None }

/// Linked list type of atoms.
typedef struct _latom {
  Atom atom;
  struct _latom *next;
} latom_t;

#define REG_DATA_INIT { NULL, 0 }

typedef struct win_option_mask {
  bool shadow: 1;
  bool fade: 1;
  bool focus: 1;
  bool full_shadow: 1;
  bool redir_ignore: 1;
  bool opacity: 1;
} win_option_mask_t;

typedef struct win_option {
  bool shadow;
  bool fade;
  bool focus;
  bool full_shadow;
  bool redir_ignore;
  double opacity;
} win_option_t;

/// Structure representing all options.
typedef struct options_t {
  // === Debugging ===
  bool monitor_repaint;
  bool print_diagnostics;
  // === General ===
  /// The configuration file we used.
  char *config_file;
  /// Path to write PID to.
  char *write_pid_path;
  /// The display name we used. NULL means we are using the value of the
  /// <code>DISPLAY</code> environment variable.
  char *display;
  /// Safe representation of display name.
  char *display_repr;
  /// The backend in use.
  enum backend backend;
  /// Whether to sync X drawing to avoid certain delay issues with
  /// GLX backend.
  bool xrender_sync;
  /// Whether to sync X drawing with X Sync fence.
  bool xrender_sync_fence;
  /// Whether to avoid using stencil buffer under GLX backend. Might be
  /// unsafe.
  bool glx_no_stencil;
  /// Whether to avoid rebinding pixmap on window damage.
  bool glx_no_rebind_pixmap;
  /// GLX swap method we assume OpenGL uses.
  int glx_swap_method;
  /// Whether to use GL_EXT_gpu_shader4 to (hopefully) accelerates blurring.
  bool glx_use_gpushader4;
  /// Custom fragment shader for painting windows, as a string.
  char *glx_fshader_win_str;
  /// Custom GLX program used for painting window.
  glx_prog_main_t glx_prog_win;
  /// Whether to fork to background.
  bool fork_after_register;
  /// Whether to detect rounded corners.
  bool detect_rounded_corners;
  /// Force painting of window content with blending.
  bool force_win_blend;
  /// Resize damage for a specific number of pixels.
  int resize_damage;
  /// Whether to unredirect all windows if a full-screen opaque window
  /// is detected.
  bool unredir_if_possible;
  /// List of conditions of windows to ignore as a full-screen window
  /// when determining if a window could be unredirected.
  c2_lptr_t *unredir_if_possible_blacklist;
  /// Delay before unredirecting screen.
  time_ms_t unredir_if_possible_delay;
  /// Forced redirection setting through D-Bus.
  switch_t redirected_force;
  /// Whether to stop painting. Controlled through D-Bus.
  switch_t stoppaint_force;
  /// Whether to re-redirect screen on root size change.
  bool reredir_on_root_change;
  /// Whether to reinitialize GLX on root size change.
  bool glx_reinit_on_root_change;
  /// Whether to enable D-Bus support.
  bool dbus;
  /// Path to log file.
  char *logpath;
  /// Number of cycles to paint in benchmark mode. 0 for disabled.
  int benchmark;
  /// Window to constantly repaint in benchmark mode. 0 for full-screen.
  Window benchmark_wid;
  /// A list of conditions of windows not to paint.
  c2_lptr_t *paint_blacklist;
  /// Whether to avoid using xcb_composite_name_window_pixmap(), for debugging.
  bool no_name_pixmap;
  /// Whether to work under synchronized mode for debugging.
  bool synchronize;
  /// Whether to show all X errors.
  bool show_all_xerrors;
  /// Whether to avoid acquiring X Selection.
  bool no_x_selection;
  /// Window type option override.
  win_option_t wintype_option[NUM_WINTYPES];

  // === VSync & software optimization ===
  /// User-specified refresh rate.
  int refresh_rate;
  /// Whether to enable refresh-rate-based software optimization.
  bool sw_opti;
  /// VSync method to use;
  vsync_t vsync;
  /// Whether to do VSync aggressively.
  bool vsync_aggressive;
  /// Whether to use glFinish() instead of glFlush() for (possibly) better
  /// VSync yet probably higher CPU usage.
  bool vsync_use_glfinish;

  // === Shadow ===
  /// Red, green and blue tone of the shadow.
  double shadow_red, shadow_green, shadow_blue;
  int shadow_radius;
  int shadow_offset_x, shadow_offset_y;
  double shadow_opacity;
  /// argument string to shadow-exclude-reg option
  char *shadow_exclude_reg_str;
  /// Shadow blacklist. A linked list of conditions.
  c2_lptr_t *shadow_blacklist;
  /// Whether bounding-shaped window should be ignored.
  bool shadow_ignore_shaped;
  /// Whether to respect _COMPTON_SHADOW.
  bool respect_prop_shadow;
  /// Whether to crop shadow to the very Xinerama screen.
  bool xinerama_shadow_crop;

  // === Fading ===
  /// How much to fade in in a single fading step.
  opacity_t fade_in_step;
  /// How much to fade out in a single fading step.
  opacity_t fade_out_step;
  /// Fading time delta. In milliseconds.
  time_ms_t fade_delta;
  /// Whether to disable fading on window open/close.
  bool no_fading_openclose;
  /// Whether to disable fading on ARGB managed destroyed windows.
  bool no_fading_destroyed_argb;
  /// Fading blacklist. A linked list of conditions.
  c2_lptr_t *fade_blacklist;

  // === Opacity ===
  /// Default opacity for inactive windows.
  /// 32-bit integer with the format of _NET_WM_OPACITY. 0 stands for
  /// not enabled, default.
  opacity_t inactive_opacity;
  /// Default opacity for inactive windows.
  opacity_t active_opacity;
  /// Whether inactive_opacity overrides the opacity set by window
  /// attributes.
  bool inactive_opacity_override;
  /// Frame opacity. Relative to window opacity, also affects shadow
  /// opacity.
  double frame_opacity;
  /// Whether to detect _NET_WM_OPACITY on client windows. Used on window
  /// managers that don't pass _NET_WM_OPACITY to frame windows.
  bool detect_client_opacity;

  // === Other window processing ===
  /// Whether to blur background of semi-transparent / ARGB windows.
  bool blur_background;
  /// Whether to blur background when the window frame is not opaque.
  /// Implies blur_background.
  bool blur_background_frame;
  /// Whether to use fixed blur strength instead of adjusting according
  /// to window opacity.
  bool blur_background_fixed;
  /// Background blur blacklist. A linked list of conditions.
  c2_lptr_t *blur_background_blacklist;
  /// Blur convolution kernel.
  xcb_render_fixed_t *blur_kerns[MAX_BLUR_PASS];
  /// How much to dim an inactive window. 0.0 - 1.0, 0 to disable.
  double inactive_dim;
  /// Whether to use fixed inactive dim opacity, instead of deciding
  /// based on window opacity.
  bool inactive_dim_fixed;
  /// Conditions of windows to have inverted colors.
  c2_lptr_t *invert_color_list;
  /// Rules to change window opacity.
  c2_lptr_t *opacity_rules;

  // === Focus related ===
  /// Whether to try to detect WM windows and mark them as focused.
  bool mark_wmwin_focused;
  /// Whether to mark override-redirect windows as focused.
  bool mark_ovredir_focused;
  /// Whether to use EWMH _NET_ACTIVE_WINDOW to find active window.
  bool use_ewmh_active_win;
  /// A list of windows always to be considered focused.
  c2_lptr_t *focus_blacklist;
  /// Whether to do window grouping with <code>WM_TRANSIENT_FOR</code>.
  bool detect_transient;
  /// Whether to do window grouping with <code>WM_CLIENT_LEADER</code>.
  bool detect_client_leader;

  // === Calculated ===
  /// Whether compton needs to track focus changes.
  bool track_focus;
  /// Whether compton needs to track window name and class.
  bool track_wdata;
  /// Whether compton needs to track window leaders.
  bool track_leader;
} options_t;

#ifdef CONFIG_OPENGL
/// Structure containing GLX-dependent data for a compton session.
typedef struct {
  // === OpenGL related ===
  /// GLX context.
  GLXContext context;
  /// Whether we have GL_ARB_texture_non_power_of_two.
  bool has_texture_non_power_of_two;
  /// Pointer to glXGetVideoSyncSGI function.
  f_GetVideoSync glXGetVideoSyncSGI;
  /// Pointer to glXWaitVideoSyncSGI function.
  f_WaitVideoSync glXWaitVideoSyncSGI;
   /// Pointer to glXGetSyncValuesOML function.
  f_GetSyncValuesOML glXGetSyncValuesOML;
  /// Pointer to glXWaitForMscOML function.
  f_WaitForMscOML glXWaitForMscOML;
  /// Pointer to glXSwapIntervalSGI function.
  f_SwapIntervalSGI glXSwapIntervalProc;
  /// Pointer to glXSwapIntervalMESA function.
  f_SwapIntervalMESA glXSwapIntervalMESAProc;
  /// Pointer to glXBindTexImageEXT function.
  f_BindTexImageEXT glXBindTexImageProc;
  /// Pointer to glXReleaseTexImageEXT function.
  f_ReleaseTexImageEXT glXReleaseTexImageProc;
  /// Pointer to the glFenceSync() function.
  f_FenceSync glFenceSyncProc;
  /// Pointer to the glIsSync() function.
  f_IsSync glIsSyncProc;
  /// Pointer to the glDeleteSync() function.
  f_DeleteSync glDeleteSyncProc;
  /// Pointer to the glClientWaitSync() function.
  f_ClientWaitSync glClientWaitSyncProc;
  /// Pointer to the glWaitSync() function.
  f_WaitSync glWaitSyncProc;
  /// Pointer to the glImportSyncEXT() function.
  f_ImportSyncEXT glImportSyncEXT;
#ifdef DEBUG_GLX_MARK
  /// Pointer to StringMarkerGREMEDY function.
  f_StringMarkerGREMEDY glStringMarkerGREMEDY;
  /// Pointer to FrameTerminatorGREMEDY function.
  f_FrameTerminatorGREMEDY glFrameTerminatorGREMEDY;
#endif
  /// Current GLX Z value.
  int z;
  /// FBConfig-s for GLX pixmap of different depths.
  glx_fbconfig_t *fbconfigs[OPENGL_MAX_DEPTH + 1];
#ifdef CONFIG_OPENGL
  glx_blur_pass_t blur_passes[MAX_BLUR_PASS];
#endif
} glx_session_t;

#define CGLX_SESSION_INIT { .context = NULL }

#endif

/// Structure containing all necessary data for a compton session.
typedef struct session {
  // === Event handlers ===
  /// ev_io for X connection
  ev_io xiow;
  /// Timeout for delayed unredirection.
  ev_timer unredir_timer;
  /// Timer for fading
  ev_timer fade_timer;
  /// Timer for delayed drawing, right now only used by
  /// swopti
  ev_timer delayed_draw_timer;
  /// Use an ev_idle callback for drawing
  /// So we only start drawing when events are processed
  ev_idle draw_idle;
  /// Called everytime we have timeouts or new data on socket,
  /// so we can be sure if xcb read from X socket at anytime during event
  /// handling, we will not left any event unhandled in the queue
  ev_prepare event_check;
  /// Signal handler for SIGUSR1
  ev_signal usr1_signal;
  /// libev mainloop
  struct ev_loop *loop;
  // === Display related ===
  /// Display in use.
  Display *dpy;
  /// Default screen.
  int scr;
  /// XCB connection.
  xcb_connection_t *c;
  /// Default visual.
  xcb_visualid_t vis;
  /// Pict formats info
  xcb_render_query_pict_formats_reply_t *pictfmts;
  /// Default depth.
  int depth;
  /// Root window.
  Window root;
  /// Height of root window.
  int root_height;
  /// Width of root window.
  int root_width;
  // Damage of root window.
  // Damage root_damage;
  /// X Composite overlay window. Used if <code>--paint-on-overlay</code>.
  Window overlay;
  /// Whether the root tile is filled by compton.
  bool root_tile_fill;
  /// Picture of the root window background.
  paint_t root_tile_paint;
  /// A region of the size of the screen.
  region_t screen_reg;
  /// Picture of root window. Destination of painting in no-DBE painting
  /// mode.
  xcb_render_picture_t root_picture;
  /// A Picture acting as the painting target.
  xcb_render_picture_t tgt_picture;
  /// Temporary buffer to paint to before sending to display.
  paint_t tgt_buffer;
  XSyncFence tgt_buffer_fence;
  /// Window ID of the window we register as a symbol.
  Window reg_win;
#ifdef CONFIG_OPENGL
  /// Pointer to GLX data.
  glx_session_t *psglx;
#endif

  // === Operation related ===
  /// Program options.
  options_t o;
  /// Whether we have hit unredirection timeout.
  bool tmout_unredir_hit;
  /// Whether we need to redraw the screen
  bool redraw_needed;
  /// Whether the program is idling. I.e. no fading, no potential window
  /// changes.
  bool fade_running;
  /// Program start time.
  struct timeval time_start;
  /// The region needs to painted on next paint.
  region_t all_damage;
  /// The region damaged on the last paint.
  region_t all_damage_last[CGLX_MAX_BUFFER_AGE];
  /// Whether all windows are currently redirected.
  bool redirected;
  /// Pre-generated alpha pictures.
  xcb_render_picture_t *alpha_picts;
  /// Time of last fading. In milliseconds.
  time_ms_t fade_time;
  /// Head pointer of the error ignore linked list.
  ignore_t *ignore_head;
  /// Pointer to the <code>next</code> member of tail element of the error
  /// ignore linked list.
  ignore_t **ignore_tail;
  // Cached blur convolution kernels.
  xcb_render_fixed_t *blur_kerns_cache[MAX_BLUR_PASS];
  /// Reset program after next paint.
  bool reset;
  /// If compton should quit
  bool quit;

  // === Expose event related ===
  /// Pointer to an array of <code>XRectangle</code>-s of exposed region.
  /// XXX why do we need this array?
  rect_t *expose_rects;
  /// Number of <code>XRectangle</code>-s in <code>expose_rects</code>.
  int size_expose;
  /// Index of the next free slot in <code>expose_rects</code>.
  int n_expose;

  // === Window related ===
  /// Linked list of all windows.
  win *list;
  /// Pointer to <code>win</code> of current active window. Used by
  /// EWMH <code>_NET_ACTIVE_WINDOW</code> focus detection. In theory,
  /// it's more reliable to store the window ID directly here, just in
  /// case the WM does something extraordinary, but caching the pointer
  /// means another layer of complexity.
  win *active_win;
  /// Window ID of leader window of currently active window. Used for
  /// subsidiary window detection.
  Window active_leader;

  // === Shadow/dimming related ===
  /// 1x1 black Picture.
  xcb_render_picture_t black_picture;
  /// 1x1 Picture of the shadow color.
  xcb_render_picture_t cshadow_picture;
  /// 1x1 white Picture.
  xcb_render_picture_t white_picture;
  /// Gaussian map of shadow.
  conv *gaussian_map;
  // for shadow precomputation
  /// Shadow depth on one side.
  int cgsize;
  /// Pre-computed color table for corners of shadow.
  unsigned char *shadow_corner;
  /// Pre-computed color table for a side of shadow.
  unsigned char *shadow_top;
  /// A region in which shadow is not painted on.
  region_t shadow_exclude_reg;

  // === Software-optimization-related ===
  /// Currently used refresh rate.
  short refresh_rate;
  /// Interval between refresh in nanoseconds.
  long refresh_intv;
  /// Nanosecond offset of the first painting.
  long paint_tm_offset;

#ifdef CONFIG_VSYNC_DRM
  // === DRM VSync related ===
  /// File descriptor of DRI device file. Used for DRM VSync.
  int drm_fd;
#endif

  // === X extension related ===
  /// Event base number for X Fixes extension.
  int xfixes_event;
  /// Error base number for X Fixes extension.
  int xfixes_error;
  /// Event base number for X Damage extension.
  int damage_event;
  /// Error base number for X Damage extension.
  int damage_error;
  /// Event base number for X Render extension.
  int render_event;
  /// Error base number for X Render extension.
  int render_error;
  /// Event base number for X Composite extension.
  int composite_event;
  /// Error base number for X Composite extension.
  int composite_error;
  /// Major opcode for X Composite extension.
  int composite_opcode;
  /// Whether X Composite NameWindowPixmap is available. Aka if X
  /// Composite version >= 0.2.
  bool has_name_pixmap;
  /// Whether X Shape extension exists.
  bool shape_exists;
  /// Event base number for X Shape extension.
  int shape_event;
  /// Error base number for X Shape extension.
  int shape_error;
  /// Whether X RandR extension exists.
  bool randr_exists;
  /// Event base number for X RandR extension.
  int randr_event;
  /// Error base number for X RandR extension.
  int randr_error;
  /// Whether X Present extension exists.
  bool present_exists;
#ifdef CONFIG_OPENGL
  /// Whether X GLX extension exists.
  bool glx_exists;
  /// Event base number for X GLX extension.
  int glx_event;
  /// Error base number for X GLX extension.
  int glx_error;
#endif
#ifdef CONFIG_XINERAMA
  /// Whether X Xinerama extension exists.
  bool xinerama_exists;
  /// Xinerama screen info.
  xcb_xinerama_query_screens_reply_t *xinerama_scrs;
  /// Xinerama screen regions.
  region_t *xinerama_scr_regs;
  /// Number of Xinerama screens.
  int xinerama_nscrs;
#endif
  /// Whether X Sync extension exists.
  bool xsync_exists;
  /// Event base number for X Sync extension.
  int xsync_event;
  /// Error base number for X Sync extension.
  int xsync_error;
  /// Whether X Render convolution filter exists.
  bool xrfilter_convolution_exists;

  // === Atoms ===
  /// Atom of property <code>_NET_WM_OPACITY</code>.
  Atom atom_opacity;
  /// Atom of <code>_NET_FRAME_EXTENTS</code>.
  Atom atom_frame_extents;
  /// Property atom to identify top-level frame window. Currently
  /// <code>WM_STATE</code>.
  Atom atom_client;
  /// Atom of property <code>WM_NAME</code>.
  Atom atom_name;
  /// Atom of property <code>_NET_WM_NAME</code>.
  Atom atom_name_ewmh;
  /// Atom of property <code>WM_CLASS</code>.
  Atom atom_class;
  /// Atom of property <code>WM_WINDOW_ROLE</code>.
  Atom atom_role;
  /// Atom of property <code>WM_TRANSIENT_FOR</code>.
  Atom atom_transient;
  /// Atom of property <code>WM_CLIENT_LEADER</code>.
  Atom atom_client_leader;
  /// Atom of property <code>_NET_ACTIVE_WINDOW</code>.
  Atom atom_ewmh_active_win;
  /// Atom of property <code>_COMPTON_SHADOW</code>.
  Atom atom_compton_shadow;
  /// Atom of property <code>_NET_WM_WINDOW_TYPE</code>.
  Atom atom_win_type;
  /// Array of atoms of all possible window types.
  Atom atoms_wintypes[NUM_WINTYPES];
  /// Linked list of additional atoms to track.
  latom_t *track_atom_lst;

#ifdef CONFIG_DBUS
  // === DBus related ===
  // DBus connection.
  DBusConnection *dbus_conn;
  // DBus service name.
  char *dbus_service;
#endif
} session_t;

/// Temporary structure used for communication between
/// <code>get_cfg()</code> and <code>parse_config()</code>.
struct options_tmp {
  bool no_dock_shadow;
  bool no_dnd_shadow;
  double menu_opacity;
};

/// Enumeration for window event hints.
typedef enum {
  WIN_EVMODE_UNKNOWN,
  WIN_EVMODE_FRAME,
  WIN_EVMODE_CLIENT
} win_evmode_t;

extern const char * const WINTYPES[NUM_WINTYPES];
extern const char * const VSYNC_STRS[NUM_VSYNC + 1];
extern const char * const BACKEND_STRS[NUM_BKEND + 1];
extern session_t *ps_g;

// == Debugging code ==
static inline void
print_timestamp(session_t *ps);

void
ev_xcb_error(session_t *ps, xcb_generic_error_t *err);

#ifdef DEBUG_BACKTRACE

#include <execinfo.h>
#define BACKTRACE_SIZE  25

/**
 * Print current backtrace.
 *
 * Stolen from glibc manual.
 */
static inline void
print_backtrace(void) {
  void *array[BACKTRACE_SIZE];
  size_t size;
  char **strings;

  size = backtrace(array, BACKTRACE_SIZE);
  strings = backtrace_symbols(array, size);

  for (size_t i = 0; i < size; i++)
     printf ("%s\n", strings[i]);

  free(strings);
}

#endif

// === Functions ===

/**
 * Return whether a struct timeval value is empty.
 */
static inline bool
timeval_isempty(struct timeval *ptv) {
  if (!ptv)
    return false;

  return ptv->tv_sec <= 0 && ptv->tv_usec <= 0;
}

/**
 * Compare a struct timeval with a time in milliseconds.
 *
 * @return > 0 if ptv > ms, 0 if ptv == 0, -1 if ptv < ms
 */
static inline int
timeval_ms_cmp(struct timeval *ptv, time_ms_t ms) {
  assert(ptv);

  // We use those if statement instead of a - expression because of possible
  // truncation problem from long to int.
  {
    long sec = ms / MS_PER_SEC;
    if (ptv->tv_sec > sec)
      return 1;
    if (ptv->tv_sec < sec)
      return -1;
  }

  {
    long usec = ms % MS_PER_SEC * (US_PER_SEC / MS_PER_SEC);
    if (ptv->tv_usec > usec)
      return 1;
    if (ptv->tv_usec < usec)
      return -1;
  }

  return 0;
}

/**
 * Subtracting two struct timeval values.
 *
 * Taken from glibc manual.
 *
 * Subtract the `struct timeval' values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 */
static inline int
timeval_subtract(struct timeval *result,
                 struct timeval *x,
                 struct timeval *y) {
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    long nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }

  if (x->tv_usec - y->tv_usec > 1000000) {
    long nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

/**
 * Subtracting two struct timespec values.
 *
 * Taken from glibc manual.
 *
 * Subtract the `struct timespec' values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 */
static inline int
timespec_subtract(struct timespec *result,
                 struct timespec *x,
                 struct timespec *y) {
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_nsec < y->tv_nsec) {
    long nsec = (y->tv_nsec - x->tv_nsec) / NS_PER_SEC + 1;
    y->tv_nsec -= NS_PER_SEC * nsec;
    y->tv_sec += nsec;
  }

  if (x->tv_nsec - y->tv_nsec > NS_PER_SEC) {
    long nsec = (x->tv_nsec - y->tv_nsec) / NS_PER_SEC;
    y->tv_nsec += NS_PER_SEC * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_nsec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_nsec = x->tv_nsec - y->tv_nsec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

static inline double
get_opacity_percent(win *w) {
  return ((double) w->opacity) / OPAQUE;
}

/**
 * Get current time in struct timeval.
 */
static inline struct timeval
get_time_timeval(void) {
  struct timeval tv = { 0, 0 };

  gettimeofday(&tv, NULL);

  // Return a time of all 0 if the call fails
  return tv;
}

/**
 * Get current time in struct timespec.
 *
 * Note its starting time is unspecified.
 */
static inline struct timespec
get_time_timespec(void) {
  struct timespec tm = { 0, 0 };

  clock_gettime(CLOCK_MONOTONIC, &tm);

  // Return a time of all 0 if the call fails
  return tm;
}


/**
 * Print time passed since program starts execution.
 *
 * Used for debugging.
 */
static inline void
print_timestamp(session_t *ps) {
  struct timeval tm, diff;

  if (gettimeofday(&tm, NULL)) return;

  timeval_subtract(&diff, &tm, &ps->time_start);
  fprintf(stderr, "[ %5ld.%06ld ] ", diff.tv_sec, diff.tv_usec);
}

/**
 * Parse a VSync option argument.
 */
static inline bool
parse_vsync(session_t *ps, const char *str) {
  for (vsync_t i = 0; VSYNC_STRS[i]; ++i)
    if (!strcasecmp(str, VSYNC_STRS[i])) {
      ps->o.vsync = i;
      return true;
    }

  printf_errf("(\"%s\"): Invalid vsync argument.", str);
  return false;
}

/**
 * Parse a backend option argument.
 */
static inline bool
parse_backend(session_t *ps, const char *str) {
  for (enum backend i = 0; BACKEND_STRS[i]; ++i)
    if (!strcasecmp(str, BACKEND_STRS[i])) {
      ps->o.backend = i;
      return true;
    }
  // Keep compatibility with an old revision containing a spelling mistake...
  if (!strcasecmp(str, "xr_glx_hybird")) {
    ps->o.backend = BKEND_XR_GLX_HYBRID;
    return true;
  }
  // cju wants to use dashes
  if (!strcasecmp(str, "xr-glx-hybrid")) {
    ps->o.backend = BKEND_XR_GLX_HYBRID;
    return true;
  }
  printf_errf("(\"%s\"): Invalid backend argument.", str);
  return false;
}

/**
 * Parse a glx_swap_method option argument.
 */
static inline bool
parse_glx_swap_method(session_t *ps, const char *str) {
  // Parse alias
  if (!strcmp("undefined", str)) {
    ps->o.glx_swap_method = 0;
    return true;
  }

  if (!strcmp("copy", str)) {
    ps->o.glx_swap_method = 1;
    return true;
  }

  if (!strcmp("exchange", str)) {
    ps->o.glx_swap_method = 2;
    return true;
  }

  if (!strcmp("buffer-age", str)) {
    ps->o.glx_swap_method = -1;
    return true;
  }

  // Parse number
  {
    char *pc = NULL;
    int age = strtol(str, &pc, 0);
    if (!pc || str == pc) {
      printf_errf("(\"%s\"): Invalid number.", str);
      return false;
    }

    for (; *pc; ++pc)
      if (!isspace(*pc)) {
        printf_errf("(\"%s\"): Trailing characters.", str);
        return false;
      }

    if (age > CGLX_MAX_BUFFER_AGE + 1 || age < -1) {
      printf_errf("(\"%s\"): Number too large / too small.", str);
      return false;
    }

    ps->o.glx_swap_method = age;
  }

  return true;
}

/**
 * Wrapper of XFree() for convenience.
 *
 * Because a NULL pointer cannot be passed to XFree(), its man page says.
 */
static inline void
cxfree(void *data) {
  if (data)
    XFree(data);
}

_Noreturn static inline void
die(const char *msg) {
  puts(msg);
  exit(1);
}

/**
 * Wrapper of XInternAtom() for convenience.
 */
static inline xcb_atom_t
get_atom(session_t *ps, const char *atom_name) {
  xcb_intern_atom_reply_t *reply =
    xcb_intern_atom_reply(ps->c,
        xcb_intern_atom(ps->c, False, strlen(atom_name), atom_name),
        NULL);

  xcb_atom_t atom = XCB_NONE;
  if (reply) {
    atom = reply->atom;
    free(reply);
  } else
    die("Failed to intern atoms, bail out");
  return atom;
}

/**
 * Return the painting target window.
 */
static inline Window
get_tgt_window(session_t *ps) {
  return ps->overlay != XCB_NONE ? ps->overlay: ps->root;
}

/**
 * Find a window from window id in window linked list of the session.
 */
static inline win *
find_win(session_t *ps, Window id) {
  if (!id)
    return NULL;

  win *w;

  for (w = ps->list; w; w = w->next) {
    if (w->id == id && !w->destroyed)
      return w;
  }

  return 0;
}

/**
 * Find out the WM frame of a client window using existing data.
 *
 * @param id window ID
 * @return struct win object of the found window, NULL if not found
 */
static inline win *
find_toplevel(session_t *ps, Window id) {
  if (!id)
    return NULL;

  for (win *w = ps->list; w; w = w->next) {
    if (w->client_win == id && !w->destroyed)
      return w;
  }

  return NULL;
}

/**
 * Check if current backend uses GLX.
 */
static inline bool
bkend_use_glx(session_t *ps) {
  return BKEND_GLX == ps->o.backend
    || BKEND_XR_GLX_HYBRID == ps->o.backend;
}

/**
 * Check if a window is really focused.
 */
static inline bool
win_is_focused_real(session_t *ps, const win *w) {
  return w->a.map_state == XCB_MAP_STATE_VIEWABLE && ps->active_win == w;
}

/**
 * Find out the currently focused window.
 *
 * @return struct win object of the found window, NULL if not found
 */
static inline win *
find_focused(session_t *ps) {
  if (!ps->o.track_focus) return NULL;

  if (ps->active_win && win_is_focused_real(ps, ps->active_win))
    return ps->active_win;
  return NULL;
}

/**
 * Free all regions in ps->all_damage_last .
 */
static inline void
free_all_damage_last(session_t *ps) {
  for (int i = 0; i < CGLX_MAX_BUFFER_AGE; ++i)
    pixman_region32_clear(&ps->all_damage_last[i]);
}

/**
 * Free a XSync fence.
 */
static inline void
free_fence(session_t *ps, XSyncFence *pfence) {
  if (*pfence)
    XSyncDestroyFence(ps->dpy, *pfence);
  *pfence = None;
}

/**
 * Check if a rectangle includes the whole screen.
 */
static inline bool
rect_is_fullscreen(session_t *ps, int x, int y, int wid, int hei) {
  return (x <= 0 && y <= 0 &&
          (x + wid) >= ps->root_width &&
          (y + hei) >= ps->root_height);
}

static void
set_ignore(session_t *ps, unsigned long sequence) {
  if (ps->o.show_all_xerrors)
    return;

  auto i = cmalloc(ignore_t);
  if (!i) return;

  i->sequence = sequence;
  i->next = 0;
  *ps->ignore_tail = i;
  ps->ignore_tail = &i->next;
}

/**
 * Ignore X errors caused by next X request.
 */
static inline void
set_ignore_next(session_t *ps) {
  set_ignore(ps, NextRequest(ps->dpy));
}

/**
 * Ignore X errors caused by given X request.
 */
static inline void
set_ignore_cookie(session_t *ps, xcb_void_cookie_t cookie) {
  set_ignore(ps, cookie.sequence);
}

/**
 * Check if a window is a fullscreen window.
 *
 * It's not using w->border_size for performance measures.
 */
static inline bool
win_is_fullscreen(session_t *ps, const win *w) {
  return rect_is_fullscreen(ps, w->g.x, w->g.y, w->widthb, w->heightb)
      && (!w->bounding_shaped || w->rounded_corners);
}

/**
 * Check if a window will be painted solid.
 */
static inline bool
win_is_solid(session_t *ps, const win *w) {
  return WMODE_SOLID == w->mode && !ps->o.force_win_blend;
}

/**
 * Determine if a window has a specific property.
 *
 * @param ps current session
 * @param w window to check
 * @param atom atom of property to check
 * @return 1 if it has the attribute, 0 otherwise
 */
static inline bool
wid_has_prop(const session_t *ps, Window w, Atom atom) {
  Atom type = None;
  int format;
  unsigned long nitems, after;
  unsigned char *data;

  if (Success == XGetWindowProperty(ps->dpy, w, atom, 0, 0, False,
        AnyPropertyType, &type, &format, &nitems, &after, &data)) {
    cxfree(data);
    if (type) return true;
  }

  return false;
}

/**
 * Get the numeric property value from a win_prop_t.
 */
static inline long
winprop_get_int(winprop_t prop) {
  long tgt = 0;

  if (!prop.nitems)
    return 0;

  switch (prop.format) {
    case 8:   tgt = *(prop.p8);    break;
    case 16:  tgt = *(prop.p16);   break;
    case 32:  tgt = *(prop.p32);   break;
    default:  assert(0);
              break;
  }

  return tgt;
}

bool
wid_get_text_prop(session_t *ps, Window wid, Atom prop,
    char ***pstrlst, int *pnstr);

void
force_repaint(session_t *ps);

bool
vsync_init(session_t *ps);

void
vsync_deinit(session_t *ps);

#ifdef CONFIG_OPENGL
/** @name GLX
 */
///@{

#endif

/**
 * Add a OpenGL debugging marker.
 */
static inline void
glx_mark_(session_t *ps, const char *func, XID xid, bool start) {
#ifdef DEBUG_GLX_MARK
  if (glx_has_context(ps) && ps->psglx->glStringMarkerGREMEDY) {
    if (!func) func = "(unknown)";
    const char *postfix = (start ? " (start)": " (end)");
    auto str = ccalloc((strlen(func) + 12 + 2
      + strlen(postfix) + 5), char);
    strcpy(str, func);
    sprintf(str + strlen(str), "(%#010lx)%s", xid, postfix);
    ps->psglx->glStringMarkerGREMEDY(strlen(str), str);
    free(str);
  }
#endif
}

#define glx_mark(ps, xid, start) glx_mark_(ps, __func__, xid, start)

/**
 * Add a OpenGL debugging marker.
 */
static inline void
glx_mark_frame(session_t *ps) {
#ifdef DEBUG_GLX_MARK
  if (glx_has_context(ps) && ps->psglx->glFrameTerminatorGREMEDY)
    ps->psglx->glFrameTerminatorGREMEDY();
#endif
}

///@}

/**
 * Synchronizes a X Render drawable to ensure all pending painting requests
 * are completed.
 */
static inline void
xr_sync(session_t *ps, Drawable d, XSyncFence *pfence) {
  if (!ps->o.xrender_sync)
    return;

  x_sync(ps->c);
  if (ps->o.xrender_sync_fence && ps->xsync_exists) {
    // TODO: If everybody just follows the rules stated in X Sync prototype,
    // we need only one fence per screen, but let's stay a bit cautious right
    // now
    XSyncFence tmp_fence = None;
    if (!pfence)
      pfence = &tmp_fence;
    assert(pfence);
    if (!*pfence)
      *pfence = XSyncCreateFence(ps->dpy, d, False);
    if (*pfence) {
      Bool attr_unused triggered = False;
      /* if (XSyncQueryFence(ps->dpy, *pfence, &triggered) && triggered)
        XSyncResetFence(ps->dpy, *pfence); */
      // The fence may fail to be created (e.g. because of died drawable)
      assert(!XSyncQueryFence(ps->dpy, *pfence, &triggered) || !triggered);
      XSyncTriggerFence(ps->dpy, *pfence);
      XSyncAwaitFence(ps->dpy, pfence, 1);
      assert(!XSyncQueryFence(ps->dpy, *pfence, &triggered) || triggered);
    }
    else {
      printf_errf("(%#010lx): Failed to create X Sync fence.", d);
    }
    free_fence(ps, &tmp_fence);
    if (*pfence)
      XSyncResetFence(ps->dpy, *pfence);
  }
}

/** @name DBus handling
 */
///@{
#ifdef CONFIG_DBUS
/** @name DBus handling
 */
///@{
bool
cdbus_init(session_t *ps);

void
cdbus_destroy(session_t *ps);

void
cdbus_loop(session_t *ps);

void
cdbus_ev_win_added(session_t *ps, win *w);

void
cdbus_ev_win_destroyed(session_t *ps, win *w);

void
cdbus_ev_win_mapped(session_t *ps, win *w);

void
cdbus_ev_win_unmapped(session_t *ps, win *w);

void
cdbus_ev_win_focusout(session_t *ps, win *w);

void
cdbus_ev_win_focusin(session_t *ps, win *w);
//!@}

/** @name DBus hooks
 */
///@{
void
win_set_shadow_force(session_t *ps, win *w, switch_t val);

void
win_set_fade_force(session_t *ps, win *w, switch_t val);

void
win_set_focused_force(session_t *ps, win *w, switch_t val);

void
win_set_invert_color_force(session_t *ps, win *w, switch_t val);

void
opts_init_track_focus(session_t *ps);

void
opts_set_no_fading_openclose(session_t *ps, bool newval);
//!@}
#endif

/**
 * @brief Dump the given data to a file.
 */
static inline bool
write_binary_data(const char *path, const unsigned char *data, int length) {
  if (!data)
    return false;
  FILE *f = fopen(path, "wb");
  if (!f) {
    printf_errf("(\"%s\"): Failed to open file for writing.", path);
    return false;
  }
  int wrote_len = fwrite(data, sizeof(unsigned char), length, f);
  fclose(f);
  if (wrote_len != length) {
    printf_errf("(\"%s\"): Failed to write all blocks: %d / %d", path,
        wrote_len, length);
    return false;
  }
  return true;
}

/**
 * @brief Dump raw bytes in HEX format.
 *
 * @param data pointer to raw data
 * @param len length of data
 */
static inline void
hexdump(const char *data, int len) {
  static const int BYTE_PER_LN = 16;

  if (len <= 0)
    return;

  // Print header
  printf("%10s:", "Offset");
  for (int i = 0; i < BYTE_PER_LN; ++i)
    printf(" %2d", i);
  putchar('\n');

  // Dump content
  for (int offset = 0; offset < len; ++offset) {
    if (!(offset % BYTE_PER_LN))
      printf("0x%08x:", offset);

    printf(" %02hhx", data[offset]);

    if ((BYTE_PER_LN - 1) == offset % BYTE_PER_LN)
      putchar('\n');
  }
  if (len % BYTE_PER_LN)
    putchar('\n');

  fflush(stdout);
}

/**
 * Set a <code>bool</code> array of all wintypes to true.
 */
static inline void
wintype_arr_enable(bool arr[]) {
  wintype_t i;

  for (i = 0; i < NUM_WINTYPES; ++i) {
    arr[i] = true;
  }
}
