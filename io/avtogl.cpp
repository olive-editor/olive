#include "avtogl.h"

extern "C" {
	#include <libavutil/avutil.h>
}

enum QOpenGLTexture::PixelFormat get_gl_pix_fmt_from_av(int format) {
	switch (format) {
	case AV_PIX_FMT_RGB24: return QOpenGLTexture::RGB;
	}
	return QOpenGLTexture::RGBA;
}

enum QOpenGLTexture::TextureFormat get_gl_tex_fmt_from_av(int format) {
	switch (format) {
	case AV_PIX_FMT_RGB24: return QOpenGLTexture::RGB8_UNorm;
	}
	return QOpenGLTexture::RGBA8_UNorm;
}
