/*
 * ImageWC.cpp — Image loader for wasmcart (replaces Image.cpp)
 *
 * Uses stb_image to load PNGs from wasmcart assets and upload to GL textures.
 * Implements the same Image::load() interface as the original.
 */

#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "wasmcart.h"

/* Max asset file size (largest PNG is about 100KB) */
#define MAX_ASSET_SIZE (512 * 1024)
static unsigned char asset_buf[MAX_ASSET_SIZE];

/* Image::load — load a texture from an asset file */
GLuint Image::load(const char *filename, ImageMipMap mipmap, ImageBlend trans,
                   GLint wrapst, GLint minfilter, GLint magfilter)
{
    GLuint texture = 0;

    /* Load asset data from .wasc archive */
    int path_len = 0;
    const char* p = filename;
    while (*p) { path_len++; p++; }

    int asset_len = wc_asset_size(filename, path_len);
    if (asset_len <= 0 || asset_len > MAX_ASSET_SIZE) {
        wc_log("Image::load FAILED asset_size: ", 31);
        wc_log(filename, path_len);
        return 0;
    }
    int loaded = wc_load_asset(filename, path_len, asset_buf, MAX_ASSET_SIZE);
    if (loaded <= 0) {
        wc_log("Image::load FAILED load_asset: ", 31);
        wc_log(filename, path_len);
        return 0;
    }

    /* Decode image with stb_image */
    int w, h, channels;
    unsigned char *pixels = stbi_load_from_memory(asset_buf, loaded, &w, &h, &channels, 4);
    if (!pixels) {
        wc_log("Image::load FAILED stbi_decode: ", 32);
        wc_log(filename, path_len);
        return 0;
    }

    /* Apply blend mode alpha transforms (matching original Image.cpp logic) */
    if (trans != IMG_SOLID && trans != IMG_ALPHA) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                unsigned char *px = pixels + (y * w + x) * 4;
                unsigned int a;
                switch (trans) {
                    case IMG_BLEND1:
                        /* a = r+g+b (clipped to 0xFF) */
                        a = px[0] + px[1] + px[2];
                        px[3] = (a > 0xFF) ? 0xFF : (unsigned char)a;
                        break;
                    case IMG_BLEND2:
                        /* a = (r+g+b)/2 (clipped to 0xFF) */
                        a = px[0] + px[1] + px[2];
                        px[3] = (a > 0x1FE) ? 0xFF : (unsigned char)(a / 2);
                        break;
                    case IMG_BLEND3:
                        /* a = (r+g+b)/3 */
                        a = px[0] + px[1] + px[2];
                        px[3] = (unsigned char)(a / 3);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    /* GL_CLAMP → GL_CLAMP_TO_EDGE (already handled by Renderer.h) */

    /* Create GL texture */
    glGenTextures(1, &texture);
    renderer_bind_texture_raw(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapst);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapst);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);

    /* Upload to GPU */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    if (mipmap == IMG_SIMPLEMIPMAPS || mipmap == IMG_BUILDMIPMAPS) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(pixels);

    return texture;
}
