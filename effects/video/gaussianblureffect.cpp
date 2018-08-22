#include "gaussianblureffect.h"

GaussianBlurEffect::GaussianBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_GAUSSIANBLUR_EFFECT) {
	enable_opengl = true;
}

void GaussianBlurEffect::process_gl(double timecode, QOpenGLShaderProgram &shaders, GLTextureCoords &coords) {
	shaders.addShaderFromSourceCode(QOpenGLShader::Vertex, "attribute vec2 position;\nvoid main() {\ngl_Position = vec4(position, 1, 1);\n}");
	shaders.addShaderFromSourceCode(QOpenGLShader::Fragment, "uniform vec3 iResolution;\nuniform sampler2D iChannel0;\nuniform bool flip;\nuniform vec2 direction;\nvoid main() {\nvec2 uv = vec2(gl_FragCoord.xy / iResolution.xy);\nif (flip) uv.y = 1.0 - uv.y;\nvec4 color = vec4(0.0);\nvec2 off1 = vec2(1.3846153846) * direction;\nvec2 off2 = vec2(3.2307692308) * direction;\ncolor += texture2D(image, uv) * 0.2270270270;\ncolor += texture2D(image, uv + (off1 / resolution)) * 0.3162162162;\ncolor += texture2D(image, uv - (off1 / resolution)) * 0.3162162162;\ncolor += texture2D(image, uv + (off2 / resolution)) * 0.0702702703;\ncolor += texture2D(image, uv - (off2 / resolution)) * 0.0702702703;\ngl_FragColor = color;}");
}


