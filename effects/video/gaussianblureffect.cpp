#include "gaussianblureffect.h"

GaussianBlurEffect::GaussianBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_GAUSSIANBLUR_EFFECT), frag(QOpenGLShader::Fragment), vert(QOpenGLShader::Vertex) {
	enable_opengl = true;

    vert.compileSourceCode("attribute vec2 position;\nvoid main() { gl_Position = vec4(position, 1, 1); }");
    frag.compileSourceCode("uniform vec3 resolution;\nuniform sampler2D image;\nuniform bool flip;\nuniform vec2 direction;\n\nvoid main() {\n  vec2 uv = vec2(gl_FragCoord.xy / resolution.xy);\n  if (flip) {\n    uv.y = 1.0 - uv.y;\n  }\n\n  vec4 color = vec4(0.0);\n  vec2 off1 = vec2(1.3846153846) * direction;\n  vec2 off2 = vec2(3.2307692308) * direction;\n  color += texture2D(image, uv) * 0.2270270270;\n  color += texture2D(image, uv + (off1 / resolution.xy)) * 0.3162162162;\n  color += texture2D(image, uv - (off1 / resolution.xy)) * 0.3162162162;\n  color += texture2D(image, uv + (off2 / resolution.xy)) * 0.0702702703;\n  color += texture2D(image, uv - (off2 / resolution.xy)) * 0.0702702703;\n  gl_FragColor = color;\n}\n");
	program.addShader(&frag);
	program.addShader(&vert);
	program.link();
}

void GaussianBlurEffect::process_gl(double timecode, GLTextureCoords &coords) {
	program.bind();
    program.setUniformValue("resolution", 1920, 1080, 0);
    program.setUniformValue("flip", true);
    program.setUniformValue("direction", 0, 20);
}

void GaussianBlurEffect::clean_gl() {
	program.release();
}
