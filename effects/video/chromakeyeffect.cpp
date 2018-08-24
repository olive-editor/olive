#include "chromakeyeffect.h"

ChromaKeyEffect::ChromaKeyEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_CHROMAKEY_EFFECT), vert(QOpenGLShader::Vertex), frag(QOpenGLShader::Fragment) {
	enable_opengl = true;

	color_field = add_row("Color:")->add_field(EFFECT_FIELD_COLOR);
	tolerance_field = add_row("Tolerance:")->add_field(EFFECT_FIELD_DOUBLE);

	color_field->set_color_value(Qt::green);
	tolerance_field->set_double_default_value(10);

	tolerance_field->set_double_minimum_value(0);
	tolerance_field->set_double_maximum_value(100);

	vert.compileSourceCode("varying vec2 vTexCoord;\nvoid main() {\n\tvTexCoord = gl_MultiTexCoord0.xy;\n\tgl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n}");
	frag.compileSourceCode("uniform vec4 keyColor;\nuniform float threshold;\nuniform sampler2D myTexture;\nvarying vec2 vTexCoord;\nvoid main(void) {\n\tvec4 textureColor = texture2D(myTexture, vTexCoord);\nfloat diff = length(keyColor - textureColor);\nif (diff < threshold) {gl_FragColor = vec4(0, 0, 0, 0);} else {gl_FragColor = textureColor;}\n}");
	program.addShader(&vert);
	program.addShader(&frag);
	program.link();

	connect(color_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(tolerance_field, SIGNAL(changed()), this, SLOT(field_changed()));
}

void ChromaKeyEffect::process_gl(double timecode, GLTextureCoords&) {
	program.bind();
	program.setUniformValue("keyColor", color_field->get_color_value(timecode));
	program.setUniformValue("threshold", (GLfloat) (tolerance_field->get_double_value(timecode)*0.01));
}

void ChromaKeyEffect::clean_gl() {
	program.release();
}
