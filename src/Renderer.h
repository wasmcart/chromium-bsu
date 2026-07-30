/*
 * Renderer.h — ES 3.0 2D sprite batch renderer for Chromium B.S.U.
 *
 * Purpose-built 2D renderer using OpenGL ES 3.0 shaders and VBO batching.
 * Replaces the original GL 1.x fixed-function rendering.
 * Implementation in Renderer.cpp.
 */
#ifndef RENDERER_H
#define RENDERER_H

#include "wasmcart.h"

/* ── Drawing modes ───────────────────────────────────────────────── */
#define DRAW_QUADS           0x0007
#define DRAW_TRIANGLES       0x0004
#define DRAW_TRIANGLE_STRIP  0x0005

/* ── Matrix modes ────────────────────────────────────────────────── */
#define MATRIX_MODELVIEW  0x1700
#define MATRIX_PROJECTION 0x1701
#define MATRIX_TEXTURE    0x1702
#ifndef GL_TEXTURE
#define GL_TEXTURE        0x1702
#endif

/* ── Legacy constants used by game code ──────────────────────────── */
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST         0x0BC0
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE          0x0BA1
#endif
#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S      0x0C60
#define GL_TEXTURE_GEN_T      0x0C61
#define GL_SPHERE_MAP         0x2402
#define GL_TEXTURE_GEN_MODE   0x2500
#define GL_S                  0x2000
#define GL_T                  0x2001
#endif
#ifndef GL_COMPILE
#define GL_COMPILE            0x1300
#endif
#ifndef GL_CLAMP
#define GL_CLAMP              GL_CLAMP_TO_EDGE
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE          0x1909
#define GL_LUMINANCE_ALPHA    0x190A
#endif
#ifndef GL_GREATER
#define GL_GREATER            0x0204
#endif
#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif

/* Block original GL headers — this file provides all GL API needed */
#define __gl_h_
#define __GL_H__
#define __glu_h__
#define __GLU_H__
#define _GL_GL_H

#ifdef __cplusplus
extern "C" {
#endif

/* ── Init / frame ────────────────────────────────────────────────── */
void renderer_init(void);
void renderer_flush(void);
void renderer_end_frame(void);

/* ── Matrix stack ────────────────────────────────────────────────── */
void renderer_matrix_mode(unsigned int mode);
void renderer_load_identity(void);
void renderer_push_matrix(void);
void renderer_pop_matrix(void);
void renderer_translate(float x, float y, float z);
void renderer_scale(float x, float y, float z);
void renderer_rotate(float angle, float x, float y, float z);
void renderer_perspective(float fovy, float aspect, float znear, float zfar);
void renderer_ortho2d(float left, float right, float bottom, float top);

/* ── Vertex submission ───────────────────────────────────────────── */
void renderer_begin(unsigned int mode);
void renderer_vertex(float x, float y, float z);
void renderer_vertex_v(const float* xyz);
void renderer_vertex2(float x, float y);
void renderer_end(void);

/* ── Per-vertex attributes ───────────────────────────────────────── */
void renderer_set_color(float r, float g, float b, float a);
void renderer_set_color3(float r, float g, float b);
void renderer_set_color_v(const float* rgba);
void renderer_set_texcoord(float u, float v);
void renderer_set_normal(float x, float y, float z);
void renderer_set_normal_v(const float* n);

/* ── State ───────────────────────────────────────────────────────── */
void renderer_enable(unsigned int cap);
void renderer_disable(unsigned int cap);
void renderer_bind_texture(unsigned int texture);
void renderer_bind_texture_raw(unsigned int target, unsigned int texture);
void renderer_set_blend_func(unsigned int sfactor, unsigned int dfactor);
void renderer_set_alpha_func(unsigned int func, float ref);
void renderer_set_depth_func(unsigned int func);
void renderer_set_depth_mask(unsigned char flag);
void renderer_set_color_mask(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void renderer_set_point_size(float size);
void renderer_set_line_width(float width);
void renderer_set_clear_color(float r, float g, float b, float a);

/* ── Viewport / clear ────────────────────────────────────────────── */
void renderer_viewport(int x, int y, int w, int h);
void renderer_clear(unsigned int mask);
void renderer_scissor(int x, int y, int w, int h);

/* ── Textures ────────────────────────────────────────────────────── */
void renderer_gen_textures(int n, unsigned int* textures);
void renderer_delete_textures(int n, const unsigned int* textures);
void renderer_tex_image_2d(unsigned int target, int level, int ifmt,
    int w, int h, int border, unsigned int fmt, unsigned int type, const void* data);
void renderer_tex_sub_image_2d(unsigned int target, int level,
    int xoff, int yoff, int w, int h, unsigned int fmt, unsigned int type, const void* data);
void renderer_tex_parameteri(unsigned int target, unsigned int pname, int param);
void renderer_tex_parameterf(unsigned int target, unsigned int pname, float param);
void renderer_tex_parameterfv(unsigned int target, unsigned int pname, const float* params);
void renderer_generate_mipmap(unsigned int target);
void renderer_active_texture(unsigned int unit);
void renderer_build_mipmaps(unsigned int target, int ifmt, int w, int h,
    unsigned int fmt, unsigned int type, const void* data);
void renderer_tex_geni(unsigned int coord, unsigned int pname, unsigned int param);

/* ── Display lists (no-op stubs) ─────────────────────────────────── */
unsigned int renderer_gen_lists(int range);
void renderer_new_list(unsigned int list, unsigned int mode);
void renderer_end_list(void);
void renderer_call_list(unsigned int list);
void renderer_delete_lists(unsigned int list, int range);

/* ── Misc ────────────────────────────────────────────────────────── */
void renderer_finish(void);
void renderer_pixel_storei(unsigned int pname, int param);
void renderer_get_integerv(unsigned int pname, int* params);

#ifdef __cplusplus
}
#endif

#endif /* RENDERER_H */
