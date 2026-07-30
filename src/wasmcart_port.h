/*
 * wasmcart_port.h — Master compatibility header for Chromium BSU wasmcart port
 *
 * Force-included (-include) before every translation unit to redirect
 * all GL, SDL, and system calls to wasmcart-compatible implementations.
 */
#ifndef CHROMIUM_COMPAT_H
#define CHROMIUM_COMPAT_H

/* ── Prevent original config.h from being loaded ──────────────────── */
#undef HAVE_CONFIG_H
#define chromium_bsu_config_h  /* block include */

/* ── Package defines ─────────────────────────────────────────────── */
#define PACKAGE "chromium-bsu"
#define PACKAGE_STRING "Chromium B.S.U. 0.9.16.1"

/* ── Disable external library backends ───────────────────────────── */
/* Image loading: we provide our own via stb_image (ImageWC.cpp) */
/* Don't define IMAGE_SDL or IMAGE_GLPNG — #ifdef checks need them absent */

/* Windowing: we don't use SDL or GLUT (wasmcart handles it) */
/* Don't define USE_SDL or USE_GLUT */

/* Audio: we provide our own (AudioWasmcart.cpp) */
/* Don't define AUDIO_OPENAL or AUDIO_SDLMIXER */

/* Text: we provide our own (TextBitmap.cpp) */
/* Don't define TEXT_GLC or TEXT_FTGL */

/* ── WASM target marker ──────────────────────────────────────────── */
#ifndef WASM_CART
#define WASM_CART 1
#endif

/* ── GL compatibility layer ──────────────────────────────────────── */
/* Must define WC_USE_GL before including Renderer.h (which includes wasmcart.h) */
#ifndef WC_USE_GL
#define WC_USE_GL
#endif

#include "Renderer.h"

/* ── Block original GL headers ───────────────────────────────────── */
/* Prevent #include <GL/gl.h> and <GL/glu.h> from doing anything */
#define __gl_h_
#define __GL_H__
#define __glu_h__
#define __GLU_H__
#define _GL_GL_H
#define _GL_GLU_H

/* Also block OpenGL framework headers on Mac */
#define __gl_gl_h_
#define __glu_glu_h_

/* ── Gettext stubs ───────────────────────────────────────────────── */
#define ENABLE_NLS 0
#define gettext(s) (s)
#define ngettext(s1, s2, n) ((n) == 1 ? (s1) : (s2))
#define _(s) (s)
#define N_(s) (s)
#define LOCALEDIR ""

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#endif

/* ── System call stubs ───────────────────────────────────────────── */
/* We don't have a filesystem in wasmcart, so stub file I/O used by Config */

/* Override getenv to return NULL (no env vars in wasm) */
/* Note: can't simply #define getenv because cstdlib declares it.
   Instead we'll let it link to wasi-libc which returns NULL anyway. */

/* stat struct — stub for dataLoc() */
#include <sys/stat.h>
/* stat() will naturally fail in wasm, which is fine — dataLoc falls through to its default */

/* ── SDL stubs ───────────────────────────────────────────────────── */
/* The game references SDL types and functions in a few places.
   We stub out the bare minimum that leaks through. */
typedef unsigned int Uint32;
typedef int Sint32;
typedef unsigned char Uint8;

/* ── gluErrorString stub ─────────────────────────────────────────── */
static inline const char* gluErrorString(GLenum error) {
    (void)error;
    return "GL error";
}

/* ── Compatibility header from original source ───────────────────── */
/* The game includes "compatibility.h" which defines snprintf etc.
   We provide an empty version since wasi-libc has everything. */
#define compatibility_h

/* ── Main.h stub ─────────────────────────────────────────────────── */
#define main_h

/* ── Override extern.h to use C linkage ──────────────────────────── */
/* extern.h declares these without extern "C", causing mangled imports.
   We define them here and block extern.h from re-declaring. */
#define extern_h
#ifdef __cplusplus
extern "C" {
#endif
const char *dataLoc(const char *filename, bool doCheck = true);
const char *alterPathForPlatform(char *filename);
void printExtensions(FILE *fstream, const char* extstr_in);
#ifdef __cplusplus
}
#endif

/* ── glGetString stub ────────────────────────────────────────────── */
/* Some debug code calls glGetString(GL_EXTENSIONS) etc.
   We just need it to not crash — real calls go through wasmcart. */

/* ── Image.h enums ───────────────────────────────────────────────── */
/* Make sure Image.h enums are available */

/* ── Forward declare our custom types ────────────────────────────── */
/* MainToolkitWC is defined in chromium_cart.cpp */

/* ── Prevent SDL includes ────────────────────────────────────────── */
#define SDL_h_
#define SDL_H_
#define _SDL_H
#define SDL_image_h_
#define SDL_opengl_h_
/* If code tries to use SDL_VERSION_ATLEAST, make it false */
#define SDL_VERSION_ATLEAST(a,b,c) 0
/* SDL_BYTEORDER — assume little endian */
#define SDL_BYTEORDER 1234
#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321

/* ── mbstowcs stub ───────────────────────────────────────────────── */
/* MenuGL.cpp calls mbstowcs(NULL, str, 0) to get string length.
   wasi-libc should have this, but just in case: */

/* ── File I/O: stubs provided in chromium_cart.cpp ────────────────── */

#endif /* CHROMIUM_COMPAT_H */
