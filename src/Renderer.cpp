#include "Renderer.h"
/*
 * gl_compat.cpp — GL1.x immediate mode → GLES3 batch renderer (implementation)
 *
 * Single compilation unit for all GL compat state and functions.
 * This avoids the static-in-header problem where each TU gets its own copy.
 *
 * NOTE: This file must NOT include chromium_compat.h or gl_compat.h's macro
 * redirects, because it needs to call the real GL functions directly.
 * It includes wasmcart.h directly for the real GL imports.
 */

#include "wasmcart.h"

/* ── GL1.x constants (duplicated from gl_compat.h to avoid macro pollution) ── */
#define GL_QUADS              0x0007
#define GL_MODELVIEW          0x1700
#define GL_PROJECTION         0x1701
#define GL_TEXTURE            0x1702
#define GL_ALPHA_TEST         0x0BC0
#define GL_NORMALIZE          0x0BA1
#define GL_TEXTURE_GEN_S      0x0C60
#define GL_TEXTURE_GEN_T      0x0C61
#define GL_COMPILE            0x1300
#define GL_GREATER            0x0204
#define GL_LUMINANCE          0x1909
#define GL_LUMINANCE_ALPHA    0x190A

#define GC_MAX_VERTS 65536
#define GC_MATRIX_STACK_DEPTH 32
#define GC_VERT_FLOATS 9

/* ── Math helpers ─────────────────────────────────────────────────── */
static float gc_sin(float x) {
    const float PI = 3.14159265f;
    const float TWO_PI = 6.28318530f;
    x = x - (int)(x / TWO_PI) * TWO_PI;
    if (x > PI) x -= TWO_PI;
    if (x < -PI) x += TWO_PI;
    float a = x < 0 ? -x : x;
    return 16.0f * x * (PI - a) / (5.0f * PI * PI - 4.0f * x * (PI - a));
}
static float gc_cos(float x) { return gc_sin(x + 1.5707963f); }

/* ── 4x4 matrix (column-major) ────────────────────────────────────── */
typedef float gc_mat4[16];

static void gc_mat4_identity(gc_mat4 m) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void gc_mat4_multiply(gc_mat4 out, const gc_mat4 a, const gc_mat4 b) {
    gc_mat4 tmp;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            tmp[c*4+r] = 0;
            for (int k = 0; k < 4; k++)
                tmp[c*4+r] += a[k*4+r] * b[c*4+k];
        }
    for (int i = 0; i < 16; i++) out[i] = tmp[i];
}

static void gc_mat4_translate(gc_mat4 m, float x, float y, float z) {
    m[12] += m[0]*x + m[4]*y + m[8]*z;
    m[13] += m[1]*x + m[5]*y + m[9]*z;
    m[14] += m[2]*x + m[6]*y + m[10]*z;
}

static void gc_mat4_scale(gc_mat4 m, float x, float y, float z) {
    m[0] *= x; m[1] *= x; m[2] *= x; m[3] *= x;
    m[4] *= y; m[5] *= y; m[6] *= y; m[7] *= y;
    m[8] *= z; m[9] *= z; m[10] *= z; m[11] *= z;
}

static void gc_mat4_rotate(gc_mat4 m, float angle_deg, float ax, float ay, float az) {
    float rad = angle_deg * 3.14159265f / 180.0f;
    float c = gc_cos(rad), s = gc_sin(rad), t = 1.0f - c;
    float len = ax*ax + ay*ay + az*az;
    if (len > 0.0001f) {
        float g = len * 0.5f;
        for (int i = 0; i < 5; i++) g = 0.5f * (g + len / g);
        float il = 1.0f / g;
        ax *= il; ay *= il; az *= il;
    }
    gc_mat4 rot;
    rot[0] = t*ax*ax + c;     rot[4] = t*ax*ay - s*az;  rot[8]  = t*ax*az + s*ay;  rot[12] = 0;
    rot[1] = t*ax*ay + s*az;  rot[5] = t*ay*ay + c;     rot[9]  = t*ay*az - s*ax;  rot[13] = 0;
    rot[2] = t*ax*az - s*ay;  rot[6] = t*ay*az + s*ax;  rot[10] = t*az*az + c;     rot[14] = 0;
    rot[3] = 0;               rot[7] = 0;                rot[11] = 0;               rot[15] = 1;
    gc_mat4 tmp2;
    for (int i = 0; i < 16; i++) tmp2[i] = m[i];
    gc_mat4_multiply(m, tmp2, rot);
}

static void gc_mat4_perspective(gc_mat4 m, float fovy_deg, float aspect, float znear, float zfar) {
    float rad = fovy_deg * 3.14159265f / 180.0f;
    float f = gc_cos(rad * 0.5f) / gc_sin(rad * 0.5f);
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (2.0f * zfar * znear) / (znear - zfar);
}

static void gc_mat4_ortho(gc_mat4 m, float l, float r, float b, float t, float n, float f) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = 2.0f / (r - l);
    m[5] = 2.0f / (t - b);
    m[10] = -2.0f / (f - n);
    m[12] = -(r + l) / (r - l);
    m[13] = -(t + b) / (t - b);
    m[14] = -(f + n) / (f - n);
    m[15] = 1.0f;
}

static int gc_mat4_is_identity(const gc_mat4 m) {
    const float e = 0.0001f;
    for (int i = 0; i < 16; i++) {
        float expected = (i == 0 || i == 5 || i == 10 || i == 15) ? 1.0f : 0.0f;
        float d = m[i] - expected;
        if (d > e || d < -e) return 0;
    }
    return 1;
}

/* ── Batch renderer state (single instance) ──────────────────────── */
static struct {
    /* Shader */
    GLuint program;
    GLuint vao, vbo;
    GLint u_mvp;
    GLint u_use_tex;
    int initialized;

    /* Matrix stacks */
    gc_mat4 modelview_stack[GC_MATRIX_STACK_DEPTH];
    gc_mat4 projection_stack[GC_MATRIX_STACK_DEPTH];
    gc_mat4 texture_stack[GC_MATRIX_STACK_DEPTH];
    int mv_top;
    int proj_top;
    int tex_top;
    int matrix_mode;

    /* Current state */
    float cur_color[4];
    float cur_texcoord[2];
    int tex_enabled;
    GLuint cur_texture;

    /* Blend state */
    GLenum blend_src;
    GLenum blend_dst;

    /* Batch buffer */
    float verts[GC_MAX_VERTS * GC_VERT_FLOATS];
    int vert_count;

    /* Immediate mode */
    int im_mode;
    float im_verts[6 * GC_VERT_FLOATS];
    int im_vert_count;

    /* Batch state for auto-flushing */
    GLuint batch_texture;
    int batch_tex_enabled;
    GLenum batch_blend_src;
    GLenum batch_blend_dst;
} gc;

/* ── Access current matrix ────────────────────────────────────────── */
static gc_mat4* gc_current_matrix(void) {
    if (gc.matrix_mode == GL_PROJECTION)
        return &gc.projection_stack[gc.proj_top];
    if (gc.matrix_mode == GL_TEXTURE)
        return &gc.texture_stack[gc.tex_top];
    return &gc.modelview_stack[gc.mv_top];
}

/* ── Shader source ────────────────────────────────────────────────── */
static const char gc_vs[] =
    "#version 300 es\n"
    "in vec3 aPos;\n"
    "in vec2 aUV;\n"
    "in vec4 aColor;\n"
    "out vec2 vUV;\n"
    "out vec4 vColor;\n"
    "uniform mat4 uMVP;\n"
    "void main() {\n"
    "  gl_Position = uMVP * vec4(aPos, 1.0);\n"
    "  vUV = aUV;\n"
    "  vColor = aColor;\n"
    "}\n";

static const char gc_fs[] =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 vUV;\n"
    "in vec4 vColor;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform int uUseTex;\n"
    "void main() {\n"
    "  if (uUseTex != 0) {\n"
    "    fragColor = texture(uTex, vUV) * vColor;\n"
    "  } else {\n"
    "    fragColor = vColor;\n"
    "  }\n"
    "}\n";

/* Forward declaration */
extern "C" void renderer_flush(void);

/* ── Check if batch needs flushing due to state change ────────────── */
static void gc_check_batch_state(void) {
    if (gc.cur_texture != gc.batch_texture ||
        gc.tex_enabled != gc.batch_tex_enabled ||
        gc.blend_src != gc.batch_blend_src ||
        gc.blend_dst != gc.batch_blend_dst) {
        renderer_flush();
        gc.batch_texture = gc.cur_texture;
        gc.batch_tex_enabled = gc.tex_enabled;
        gc.batch_blend_src = gc.blend_src;
        gc.batch_blend_dst = gc.blend_dst;
    }
}

/* ── Add a triangle to the batch ──────────────────────────────────── */
static void gc_emit_triangle(const float* v0, const float* v1, const float* v2) {
    if (gc.vert_count + 3 > GC_MAX_VERTS) renderer_flush();
    float* dst = &gc.verts[gc.vert_count * GC_VERT_FLOATS];

    const gc_mat4* mv = &gc.modelview_stack[gc.mv_top];
    int tex_xform = !gc_mat4_is_identity(gc.texture_stack[gc.tex_top]);
    const gc_mat4* tm = &gc.texture_stack[gc.tex_top];

    for (int i = 0; i < 3; i++) {
        const float* src = (i == 0) ? v0 : (i == 1) ? v1 : v2;
        float x = src[0], y = src[1], z = src[2];
        dst[0] = (*mv)[0]*x + (*mv)[4]*y + (*mv)[8]*z  + (*mv)[12];
        dst[1] = (*mv)[1]*x + (*mv)[5]*y + (*mv)[9]*z  + (*mv)[13];
        dst[2] = (*mv)[2]*x + (*mv)[6]*y + (*mv)[10]*z + (*mv)[14];
        if (tex_xform) {
            float u = src[3], v = src[4];
            dst[3] = (*tm)[0]*u + (*tm)[4]*v + (*tm)[12];
            dst[4] = (*tm)[1]*u + (*tm)[5]*v + (*tm)[13];
        } else {
            dst[3] = src[3]; dst[4] = src[4];
        }
        dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7]; dst[8] = src[8];
        dst += GC_VERT_FLOATS;
    }
    gc.vert_count += 3;
}

/* ── Public API (extern "C") ──────────────────────────────────────── */

extern "C" {

void renderer_init(void) {
    if (gc.initialized) return;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char* vs_src[1] = { gc_vs };
    glShaderSource(vs, 1, vs_src, 0);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fs_src[1] = { gc_fs };
    glShaderSource(fs, 1, fs_src, 0);
    glCompileShader(fs);

    gc.program = glCreateProgram();
    glAttachShader(gc.program, vs);
    glAttachShader(gc.program, fs);
    glLinkProgram(gc.program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    gc.u_mvp = glGetUniformLocation(gc.program, "uMVP");
    gc.u_use_tex = glGetUniformLocation(gc.program, "uUseTex");

    /* Create VAO + VBO */
    glGenVertexArrays(1, &gc.vao);
    glBindVertexArray(gc.vao);
    glGenBuffers(1, &gc.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gc.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gc.verts), 0, GL_DYNAMIC_DRAW);

    GLint a_pos = glGetAttribLocation(gc.program, "aPos");
    glEnableVertexAttribArray(a_pos);
    glVertexAttribPointer(a_pos, 3, GL_FLOAT, GL_FALSE, GC_VERT_FLOATS * 4, (void*)0);

    GLint a_uv = glGetAttribLocation(gc.program, "aUV");
    glEnableVertexAttribArray(a_uv);
    glVertexAttribPointer(a_uv, 2, GL_FLOAT, GL_FALSE, GC_VERT_FLOATS * 4, (void*)12);

    GLint a_color = glGetAttribLocation(gc.program, "aColor");
    glEnableVertexAttribArray(a_color);
    glVertexAttribPointer(a_color, 4, GL_FLOAT, GL_FALSE, GC_VERT_FLOATS * 4, (void*)20);

    /* Init matrix stacks */
    gc.mv_top = 0;
    gc.proj_top = 0;
    gc.tex_top = 0;
    gc_mat4_identity(gc.modelview_stack[0]);
    gc_mat4_identity(gc.projection_stack[0]);
    gc_mat4_identity(gc.texture_stack[0]);
    gc.matrix_mode = GL_MODELVIEW;

    /* Init state */
    gc.cur_color[0] = gc.cur_color[1] = gc.cur_color[2] = gc.cur_color[3] = 1.0f;
    gc.cur_texcoord[0] = gc.cur_texcoord[1] = 0.0f;
    gc.tex_enabled = 1;
    gc.cur_texture = 0;
    gc.vert_count = 0;
    gc.im_mode = 0;
    gc.batch_texture = 0;
    gc.batch_tex_enabled = 1;

    gc.blend_src = GL_SRC_ALPHA;
    gc.blend_dst = GL_ONE_MINUS_SRC_ALPHA;
    gc.batch_blend_src = GL_SRC_ALPHA;
    gc.batch_blend_dst = GL_ONE_MINUS_SRC_ALPHA;

    gc.initialized = 1;

    WC_LOG("renderer_init: done");
}

void renderer_flush(void) {
    if (gc.vert_count == 0) return;

    glUseProgram(gc.program);

    glUniformMatrix4fv(gc.u_mvp, 1, GL_FALSE, gc.projection_stack[gc.proj_top]);
    glUniform1i(gc.u_use_tex, gc.batch_tex_enabled ? 1 : 0);

    if (gc.batch_tex_enabled) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gc.batch_texture);
        glUniform1i(glGetUniformLocation(gc.program, "uTex"), 0);
    }

    glBlendFunc(gc.batch_blend_src, gc.batch_blend_dst);

    glBindVertexArray(gc.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gc.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gc.vert_count * GC_VERT_FLOATS * 4, gc.verts);
    glDrawArrays(GL_TRIANGLES, 0, gc.vert_count);

    gc.vert_count = 0;
}

/* ── Matrix operations ────────────────────────────────────────────── */

void renderer_matrix_mode(GLenum mode) {
    renderer_flush();
    gc.matrix_mode = mode;
}

void renderer_load_identity(void) {
    renderer_flush();
    gc_mat4_identity(*gc_current_matrix());
}

void renderer_push_matrix(void) {
    renderer_flush();
    if (gc.matrix_mode == GL_PROJECTION) {
        if (gc.proj_top < GC_MATRIX_STACK_DEPTH - 1) {
            for (int i = 0; i < 16; i++)
                gc.projection_stack[gc.proj_top + 1][i] = gc.projection_stack[gc.proj_top][i];
            gc.proj_top++;
        }
    } else if (gc.matrix_mode == GL_TEXTURE) {
        if (gc.tex_top < GC_MATRIX_STACK_DEPTH - 1) {
            for (int i = 0; i < 16; i++)
                gc.texture_stack[gc.tex_top + 1][i] = gc.texture_stack[gc.tex_top][i];
            gc.tex_top++;
        }
    } else {
        if (gc.mv_top < GC_MATRIX_STACK_DEPTH - 1) {
            for (int i = 0; i < 16; i++)
                gc.modelview_stack[gc.mv_top + 1][i] = gc.modelview_stack[gc.mv_top][i];
            gc.mv_top++;
        }
    }
}

void renderer_pop_matrix(void) {
    renderer_flush();
    if (gc.matrix_mode == GL_PROJECTION) {
        if (gc.proj_top > 0) gc.proj_top--;
    } else if (gc.matrix_mode == GL_TEXTURE) {
        if (gc.tex_top > 0) gc.tex_top--;
    } else {
        if (gc.mv_top > 0) gc.mv_top--;
    }
}

void renderer_translate(float x, float y, float z) {
    renderer_flush();
    gc_mat4_translate(*gc_current_matrix(), x, y, z);
}

void renderer_scale(float x, float y, float z) {
    renderer_flush();
    gc_mat4_scale(*gc_current_matrix(), x, y, z);
}

void renderer_rotate(float angle, float x, float y, float z) {
    renderer_flush();
    gc_mat4_rotate(*gc_current_matrix(), angle, x, y, z);
}

void renderer_perspective(float fovy, float aspect, float znear, float zfar) {
    renderer_flush();
    gc_mat4 persp;
    gc_mat4_perspective(persp, fovy, aspect, znear, zfar);
    gc_mat4 tmp;
    for (int i = 0; i < 16; i++) tmp[i] = (*gc_current_matrix())[i];
    gc_mat4_multiply(*gc_current_matrix(), tmp, persp);
}

void renderer_ortho2d(float l, float r, float b, float t) {
    renderer_flush();
    gc_mat4 ortho;
    gc_mat4_ortho(ortho, l, r, b, t, -1.0f, 1.0f);
    gc_mat4 tmp;
    for (int i = 0; i < 16; i++) tmp[i] = (*gc_current_matrix())[i];
    gc_mat4_multiply(*gc_current_matrix(), tmp, ortho);
}

/* ── Color / Texcoord / Normal ────────────────────────────────────── */

void renderer_set_color(float r, float g, float b, float a) {
    gc.cur_color[0] = r; gc.cur_color[1] = g;
    gc.cur_color[2] = b; gc.cur_color[3] = a;
}

void renderer_set_color3(float r, float g, float b) {
    renderer_set_color(r, g, b, 1.0f);
}

void renderer_set_color_v(const float* v) {
    renderer_set_color(v[0], v[1], v[2], v[3]);
}

void renderer_set_texcoord(float u, float v) {
    gc.cur_texcoord[0] = u;
    gc.cur_texcoord[1] = v;
}

void renderer_set_normal(float x, float y, float z) {
    (void)x; (void)y; (void)z;
}

void renderer_set_normal_v(const float* v) {
    (void)v;
}

/* ── Enable / Disable / BindTexture / BlendFunc ──────────────────── */

void renderer_enable(GLenum cap) {
    switch (cap) {
        case 0x0DE1: /* GL_TEXTURE_2D */
            gc.tex_enabled = 1;
            break;
        case GL_BLEND:
        case GL_DEPTH_TEST:
            renderer_flush();
            glEnable(cap);
            break;
        case GL_ALPHA_TEST:
        case GL_NORMALIZE:
        case GL_TEXTURE_GEN_S:
        case GL_TEXTURE_GEN_T:
        case GL_CULL_FACE:
            break;
        default:
            renderer_flush();
            glEnable(cap);
            break;
    }
}

void renderer_disable(GLenum cap) {
    switch (cap) {
        case 0x0DE1: /* GL_TEXTURE_2D */
            gc.tex_enabled = 0;
            break;
        case GL_BLEND:
        case GL_DEPTH_TEST:
            renderer_flush();
            glDisable(cap);
            break;
        case GL_ALPHA_TEST:
        case GL_NORMALIZE:
        case GL_TEXTURE_GEN_S:
        case GL_TEXTURE_GEN_T:
        case GL_CULL_FACE:
            break;
        default:
            renderer_flush();
            glDisable(cap);
            break;
    }
}

void renderer_bind_texture_raw(GLenum target, GLuint texture) {
    gc.cur_texture = texture;
    gc_check_batch_state();
    glBindTexture(target, texture);
}

void renderer_bind_texture(GLuint texture) {
    renderer_bind_texture_raw(GL_TEXTURE_2D, texture);
}

void renderer_set_blend_func(GLenum sfactor, GLenum dfactor) {
    gc.blend_src = sfactor;
    gc.blend_dst = dfactor;
    gc_check_batch_state();
}

/* ── Immediate mode ──────────────────────────────────────────────── */

void renderer_begin(GLenum mode) {
    gc_check_batch_state();
    gc.im_mode = mode;
    gc.im_vert_count = 0;
}

void renderer_vertex(float x, float y, float z) {
    float* v = &gc.im_verts[gc.im_vert_count * GC_VERT_FLOATS];
    v[0] = x; v[1] = y; v[2] = z;
    v[3] = gc.cur_texcoord[0]; v[4] = gc.cur_texcoord[1];
    v[5] = gc.cur_color[0]; v[6] = gc.cur_color[1];
    v[7] = gc.cur_color[2]; v[8] = gc.cur_color[3];
    gc.im_vert_count++;

    if (gc.im_mode == GL_TRIANGLE_STRIP) {
        if (gc.im_vert_count >= 3) {
            int i = gc.im_vert_count - 1;
            const float* v0;
            const float* v1;
            const float* v2;
            if ((gc.im_vert_count & 1) == 1) {
                v0 = &gc.im_verts[(i-2) * GC_VERT_FLOATS];
                v1 = &gc.im_verts[(i-1) * GC_VERT_FLOATS];
                v2 = &gc.im_verts[i * GC_VERT_FLOATS];
            } else {
                v0 = &gc.im_verts[(i-1) * GC_VERT_FLOATS];
                v1 = &gc.im_verts[(i-2) * GC_VERT_FLOATS];
                v2 = &gc.im_verts[i * GC_VERT_FLOATS];
            }
            gc_emit_triangle(v0, v1, v2);
        }
    } else if (gc.im_mode == GL_QUADS) {
        if (gc.im_vert_count == 4) {
            gc_emit_triangle(&gc.im_verts[0 * GC_VERT_FLOATS],
                             &gc.im_verts[1 * GC_VERT_FLOATS],
                             &gc.im_verts[2 * GC_VERT_FLOATS]);
            gc_emit_triangle(&gc.im_verts[0 * GC_VERT_FLOATS],
                             &gc.im_verts[2 * GC_VERT_FLOATS],
                             &gc.im_verts[3 * GC_VERT_FLOATS]);
            gc.im_vert_count = 0;
        }
    } else if (gc.im_mode == GL_TRIANGLES) {
        if (gc.im_vert_count == 3) {
            gc_emit_triangle(&gc.im_verts[0 * GC_VERT_FLOATS],
                             &gc.im_verts[1 * GC_VERT_FLOATS],
                             &gc.im_verts[2 * GC_VERT_FLOATS]);
            gc.im_vert_count = 0;
        }
    }
}

void renderer_vertex_v(const float* v) {
    renderer_vertex(v[0], v[1], v[2]);
}

void renderer_vertex2(float x, float y) {
    renderer_vertex(x, y, 0.0f);
}

void renderer_end(void) {
    gc.im_mode = 0;
    gc.im_vert_count = 0;
}

/* ── Stubs ────────────────────────────────────────────────────────── */

void renderer_set_alpha_func(GLenum func, float ref) {
    (void)func; (void)ref;
}

void renderer_tex_parameterfv(GLenum target, GLenum pname, const float* params) {
    (void)target; (void)pname; (void)params;
}

void renderer_build_mipmaps(GLenum target, GLint internalformat,
    GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data)
{
    GLint ifmt = internalformat;
    GLenum fmt = format;
    if (ifmt == 1 || ifmt == GL_LUMINANCE) { ifmt = GL_R8; fmt = GL_RED; }
    else if (ifmt == 2 || ifmt == GL_LUMINANCE_ALPHA) { ifmt = GL_RG8; fmt = GL_RG; }
    else if (ifmt == 3) { ifmt = GL_RGB; fmt = GL_RGB; }
    else if (ifmt == 4) { ifmt = GL_RGBA; fmt = GL_RGBA; }
    glTexImage2D(target, 0, ifmt, width, height, 0, fmt, type, data);
    glGenerateMipmap(target);
}

void renderer_set_point_size(float size) { (void)size; }
void renderer_set_line_width(float width) { (void)width; }

void renderer_tex_geni(GLenum coord, GLenum pname, GLenum param) {
    (void)coord; (void)pname; (void)param;
}

GLuint renderer_gen_lists(GLsizei range) { (void)range; return 1; }
void renderer_new_list(GLuint list, GLenum mode) { (void)list; (void)mode; }
void renderer_end_list(void) {}
void renderer_call_list(GLuint list) { (void)list; }
void renderer_delete_lists(GLuint list, GLsizei range) { (void)list; (void)range; }

void renderer_finish(void) {
    renderer_flush();
}

void renderer_pixel_storei(GLenum pname, GLint param) {
    (void)pname; (void)param;
}

void renderer_get_integerv(GLenum pname, GLint* params) {
    (void)pname;
    if (params) *params = 4;
}

/* ── Passthrough GL functions ──────────────────────────────────────── */
/* These are standard ES 3.0 calls that don't need batching/translation */

void renderer_viewport(int x, int y, int w, int h) { renderer_flush(); glViewport(x, y, w, h); }
void renderer_clear(unsigned int mask) { renderer_flush(); glClear(mask); }
void renderer_set_clear_color(float r, float g, float b, float a) { glClearColor(r, g, b, a); }
void renderer_scissor(int x, int y, int w, int h) { glScissor(x, y, w, h); }
void renderer_set_depth_func(unsigned int func) { glDepthFunc(func); }
void renderer_set_depth_mask(unsigned char flag) { glDepthMask(flag); }
void renderer_set_color_mask(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { glColorMask(r, g, b, a); }
void renderer_gen_textures(int n, unsigned int* tex) { glGenTextures(n, tex); }
void renderer_delete_textures(int n, const unsigned int* tex) { glDeleteTextures(n, tex); }
void renderer_tex_image_2d(unsigned int target, int level, int ifmt, int w, int h, int border, unsigned int fmt, unsigned int type, const void* data) { glTexImage2D(target, level, ifmt, w, h, border, fmt, type, data); }
void renderer_tex_sub_image_2d(unsigned int target, int level, int xoff, int yoff, int w, int h, unsigned int fmt, unsigned int type, const void* data) { glTexSubImage2D(target, level, xoff, yoff, w, h, fmt, type, data); }
void renderer_tex_parameteri(unsigned int target, unsigned int pname, int param) { glTexParameteri(target, pname, param); }
void renderer_tex_parameterf(unsigned int target, unsigned int pname, float param) { glTexParameterf(target, pname, param); }
void renderer_generate_mipmap(unsigned int target) { glGenerateMipmap(target); }
void renderer_active_texture(unsigned int unit) { glActiveTexture(unit); }
void renderer_end_frame(void) { renderer_flush(); }

} /* extern "C" */
