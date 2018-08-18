#include "inverteffect.h"

#include "ui/labelslider.h"

#include <QLabel>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

InvertEffect::InvertEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_INVERT_EFFECT), vert_shader(QOpenGLShader::Vertex), frag_shader(QOpenGLShader::Fragment) {
	enable_opengl = true;

	EffectRow* amount_row = add_row("Amount:");
	amount_val = amount_row->add_field(EFFECT_FIELD_DOUBLE);
	amount_val->set_double_minimum_value(0);
	amount_val->set_double_maximum_value(100);

	// set defaults	
	amount_val->set_double_default_value(100);

	connect(amount_val, SIGNAL(changed()), this, SLOT(compile()));

	compile();
}

void InvertEffect::compile() {
	double value = amount_val->get_double_value()*0.01;
	vert_shader.compileSourceCode("varying vec2 vTexCoord; void main() { vTexCoord = gl_MultiTexCoord0; gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; }");
	frag_shader.compileSourceCode("uniform sampler2D myTexture; varying vec2 vTexCoord; void main(void) { vec4 textureColor = texture2D(myTexture, vTexCoord); gl_FragColor = vec4(textureColor.r+((1.0-textureColor.r-textureColor.r)*" + QString::number(value) + "), textureColor.g+((1.0-textureColor.g-textureColor.g)*" + QString::number(value) + "), textureColor.b+((1.0-textureColor.b-textureColor.b)*" + QString::number(value) + "), 1); }");
	field_changed();
}

Effect* InvertEffect::copy(Clip* c) {
	InvertEffect* i = new InvertEffect(c);
	i->amount_val->set_double_value(amount_val->get_double_value());
	return i;
}

void InvertEffect::process_gl(QOpenGLShaderProgram& shader_prog, int* anchor_x, int* anchor_y) {
	shader_prog.addShader(&vert_shader);
	shader_prog.addShader(&frag_shader);
}
