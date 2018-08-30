#include "gaussianblureffect.h"

#include "project/clip.h"

#include <QImage>

GaussianBlurEffect::GaussianBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_GAUSSIANBLUR_EFFECT), frag(QOpenGLShader::Fragment), vert(QOpenGLShader::Vertex), bound(false) {
	enable_opengl = true;
	enable_image = false;

	amount_val = add_row("Amount:")->add_field(EFFECT_FIELD_DOUBLE);
	horiz_val = add_row("Horizontal:")->add_field(EFFECT_FIELD_BOOL);
	vert_val = add_row("Vertical:")->add_field(EFFECT_FIELD_BOOL);

	amount_val->set_double_minimum_value(0);
	amount_val->set_double_default_value(20);
	horiz_val->set_bool_value(true);
	vert_val->set_bool_value(true);

    vert.compileSourceCode("void main() {\ngl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n}");
//    vert.compileSourceCode("#version 330 core\nin vec2 VertexPosition;\nvoid main(void) {\ngl_Position = vec4(VertexPosition, 0.0, 1.0);\n}");
//	frag.compileSourceCode("#version 330 core\n\nuniform sampler2D image;\n\nout vec4 FragmentColor;\n\nuniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );\nuniform float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );\n\nvoid main(void)\n{\n	FragmentColor = texture2D( image, vec2(gl_FragCoord)/vec2(640.0, 360.0) ) * weight[0];\n	for (int i=1; i<3; i++) {\n		FragmentColor += texture2D( image, ( vec2(gl_FragCoord)+vec2(offset[i], 0.0) )/vec2(640.0, 360.0) ) * weight[i];\n		FragmentColor += texture2D( image, ( vec2(gl_FragCoord)-vec2(offset[i], 0.0) )/vec2(640.0, 360.0) ) * weight[i];\n	}\n}");
	frag.compileSourceCode("#version 330 core\n\nuniform sampler2D image;\n\nout vec4 FragmentColor;\n\nuniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );\nuniform float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );\nuniform float halfweight[3] = float[]( 0.1135135135, 0.1581081081, 0.03513513515 );\n\nuniform vec2 resolution;\nuniform bool horiz_blur;\nuniform bool vert_blur;\n\nvoid main(void) {\n	FragmentColor = texture2D( image, vec2(gl_FragCoord)/resolution ) * weight[0];\n	for (int i=1; i<3; i++) {\n		float weight = (horiz_blur && vert_blur) ? halfweight[i] : weight[i];\n\n		if (horiz_blur) {\n			FragmentColor += texture2D( image, ( vec2(gl_FragCoord)+vec2(offset[i], 0.0) )/resolution ) * weight;\n			FragmentColor += texture2D( image, ( vec2(gl_FragCoord)-vec2(offset[i], 0.0) )/resolution ) * weight;\n		}\n		\n		if (vert_blur) {\n			FragmentColor += texture2D( image, ( vec2(gl_FragCoord)+vec2(0.0, offset[i]) )/resolution ) * weight;\n			FragmentColor += texture2D( image, ( vec2(gl_FragCoord)-vec2(0.0, offset[i]) )/resolution ) * weight;\n		}		\n	}\n}");
	program.addShader(&vert);
	program.addShader(&frag);
	program.link();

	connect(amount_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(horiz_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(vert_val, SIGNAL(changed()), this, SLOT(field_changed()));

	refresh();
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
	bound = false;
	bool horiz = horiz_val->get_bool_value(timecode);
	bool vert = vert_val->get_bool_value(timecode);
	int iterationCount = amount_val->get_double_value(timecode);
	if (iterationCount > 0 && (horiz || vert)) {
		bound = program.bind();
		setIterations(iterationCount);
		program.setUniformValue("resolution", parent_clip->getWidth(), parent_clip->getHeight());
		program.setUniformValue("horiz_blur", horiz);
		program.setUniformValue("vert_blur", vert);
	}
}

void GaussianBlurEffect::clean_gl() {	
	if (bound) program.release();
}
