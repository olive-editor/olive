#include "inverteffect.h"

#include "ui/labelslider.h"

#include <QLabel>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

InvertEffect::InvertEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_INVERT_EFFECT) {
	enable_opengl = true;

	EffectRow* amount_row = add_row("Amount:");
	amount_val = amount_row->add_field(EFFECT_FIELD_DOUBLE);
	amount_val->set_double_minimum_value(0);
	amount_val->set_double_maximum_value(100);

	// set defaults	
	amount_val->set_double_default_value(100);

	connect(amount_val, SIGNAL(changed()), this, SLOT(field_changed()));
}

void InvertEffect::process_gl(long frame, QOpenGLShaderProgram& shaders, GLTextureCoords&) {
	double value = amount_val->get_double_value(frame)*0.01;
	shaders.addShaderFromSourceCode(QOpenGLShader::Vertex, "varying vec2 vTexCoord;\nvoid main() {\n\tvTexCoord = gl_MultiTexCoord0.xy;\n\tgl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n}");
	shaders.addShaderFromSourceCode(QOpenGLShader::Fragment, "uniform sampler2D myTexture;\nvarying vec2 vTexCoord;\nvoid main(void) {\n\tvec4 textureColor = texture2D(myTexture, vTexCoord);\n\tgl_FragColor = vec4(textureColor.r+((1.0-textureColor.r-textureColor.r)*" + QString::number(value, 'f', 2) + "), textureColor.g+((1.0-textureColor.g-textureColor.g)*" + QString::number(value, 'f', 2) + "), textureColor.b+((1.0-textureColor.b-textureColor.b)*" + QString::number(value, 'f', 2) + "), 1);\n}");
}
