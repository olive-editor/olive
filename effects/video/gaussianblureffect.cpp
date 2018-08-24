#include "gaussianblureffect.h"

GaussianBlurEffect::GaussianBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_GAUSSIANBLUR_EFFECT), frag(QOpenGLShader::Fragment), vert(QOpenGLShader::Vertex) {
	enable_opengl = true;

	vert.compileSourceCode("varying vec2 vTexCoord;\nvoid main() {\n\tvTexCoord = gl_MultiTexCoord0.xy;\n\tgl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n}");
	frag.compileSourceCode("uniform mediump float amount_val;\nuniform sampler2D myTexture;\nvarying vec2 vTexCoord;\nvoid main(void) {\n\tvec4 textureColor = texture2D(myTexture, vTexCoord);\n\tgl_FragColor = vec4(textureColor.r+((1.0-textureColor.r-textureColor.r)*amount_val), textureColor.g+((1.0-textureColor.g-textureColor.g)*amount_val), textureColor.b+((1.0-textureColor.b-textureColor.b)*amount_val), textureColor.a);\n}");
	program.addShader(&frag);
	program.addShader(&vert);
	program.link();
}

void GaussianBlurEffect::process_gl(double timecode, GLTextureCoords &coords) {
	program.bind();
}

void GaussianBlurEffect::clean_gl() {
	program.release();
}


