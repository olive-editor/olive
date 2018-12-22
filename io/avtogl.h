#ifndef AVTOGL_H
#define AVTOGL_H

#include <QOpenGLTexture>

enum QOpenGLTexture::PixelFormat get_gl_pix_fmt_from_av(int format);
enum QOpenGLTexture::TextureFormat get_gl_tex_fmt_from_av(int format);

#endif // AVTOGL_H
