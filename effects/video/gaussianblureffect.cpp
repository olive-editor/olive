#include "gaussianblureffect.h"

#include <QImage>

GaussianBlurEffect::GaussianBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_GAUSSIANBLUR_EFFECT), frag(QOpenGLShader::Fragment), vert(QOpenGLShader::Vertex) {
	enable_opengl = true;
	enable_image = false;

    vert.compileSourceCode("attribute vec2 position;\nvoid main() { gl_Position = vec4(position, 1, 1); }");
    frag.compileSourceCode("uniform vec3 resolution;\nuniform sampler2D image;\nuniform bool flip;\nuniform vec2 direction;\n\nvoid main() {\n  vec2 uv = vec2(gl_FragCoord.xy / resolution.xy);\n  if (flip) {\n    uv.y = 1.0 - uv.y;\n  }\n\n  vec4 color = vec4(0.0);\n  vec2 off1 = vec2(1.3846153846) * direction;\n  vec2 off2 = vec2(3.2307692308) * direction;\n  color += texture2D(image, uv) * 0.2270270270;\n  color += texture2D(image, uv + (off1 / resolution.xy)) * 0.3162162162;\n  color += texture2D(image, uv - (off1 / resolution.xy)) * 0.3162162162;\n  color += texture2D(image, uv + (off2 / resolution.xy)) * 0.0702702703;\n  color += texture2D(image, uv - (off2 / resolution.xy)) * 0.0702702703;\n  gl_FragColor = color;\n}\n");
	program.addShader(&frag);
	program.addShader(&vert);
	program.link();
}

void GaussianBlurEffect::process_image(double timecode, uint8_t *data, int width, int height) {
	QImage result(data, width, height, QImage::Format_RGBA8888); // create QImage wrapper
	int radius = 5;

	int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
	int alpha = (radius < 1)  ? 16 : (radius > 17) ? 1 : tab[radius-1];

	int r1 = result.rect().top();
	int r2 = result.rect().bottom();
	int c1 = result.rect().left();
	int c2 = result.rect().right();

	int bpl = result.bytesPerLine();
	int rgba[4];
	unsigned char* p;

	int i1 = 0;
	int i2 = 3;

	for (int col = c1; col <= c2; col++) {
		p = result.scanLine(r1) + col * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p += bpl;
		for (int j = r1; j < r2; j++, p += bpl)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}

	for (int row = r1; row <= r2; row++) {
		p = result.scanLine(row) + c1 * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p += 4;
		for (int j = c1; j < c2; j++, p += 4)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}

	for (int col = c1; col <= c2; col++) {
		p = result.scanLine(r2) + col * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p -= bpl;
		for (int j = r1; j < r2; j++, p -= bpl)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}

	for (int row = r1; row <= r2; row++) {
		p = result.scanLine(row) + c2 * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p -= 4;
		for (int j = c1; j < c2; j++, p -= 4)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}
}

void GaussianBlurEffect::process_gl(double timecode, GLTextureCoords &coords) {
	program.bind();
    program.setUniformValue("resolution", 1920, 1080, 0);
	program.setUniformValue("flip", false);
    program.setUniformValue("direction", 0, 20);
}

void GaussianBlurEffect::clean_gl() {
	program.release();
}
