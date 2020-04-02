#include "openglshader.h"

#include <QOpenGLExtraFunctions>

OpenGLShader::OpenGLShader()
{

}

OpenGLShaderPtr OpenGLShader::Create()
{
  return std::make_shared<OpenGLShader>();
}

OpenGLShaderPtr OpenGLShader::CreateDefault(const QString &function_name, const QString &shader_code)
{
  OpenGLShaderPtr program = Create();

  // Add shaders to program
  program->addShaderFromSourceCode(QOpenGLShader::Vertex, CodeDefaultVertex());
  program->addShaderFromSourceCode(QOpenGLShader::Fragment, CodeDefaultFragment(function_name, shader_code));
  program->link();

  return program;
}

// copied from source code to OCIODisplay
const int OCIO_LUT3D_EDGE_SIZE = 32;

// copied from source code to OCIODisplay, expanded from 3*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE
const int OCIO_NUM_3D_ENTRIES = 98304;

OpenGLShaderPtr OpenGLShader::CreateOCIO(QOpenGLContext* ctx,
                                         GLuint& lut_texture,
                                         OCIO::ConstProcessorRcPtr processor,
                                         bool alpha_is_associated)
{
  QOpenGLExtraFunctions* xf = ctx->extraFunctions();

  // Create LUT texture
  xf->glGenTextures(1, &lut_texture);

  // Bind LUT
  xf->glBindTexture(GL_TEXTURE_3D, lut_texture);

  // Set texture parameters
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // Allocate storage for texture
  xf->glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB,
                   OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE,
                   0, GL_RGB,GL_FLOAT, nullptr);

  //
  // SET UP GLSL SHADER
  //

  OCIO::GpuShaderDesc shaderDesc;
  const char* ocio_func_name = "OCIODisplay";
  shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
  shaderDesc.setFunctionName(ocio_func_name);
  shaderDesc.setLut3DEdgeLen(OCIO_LUT3D_EDGE_SIZE);

  //
  // COMPUTE 3D LUT
  //

  GLfloat* ocio_lut_data = new GLfloat[OCIO_NUM_3D_ENTRIES];
  processor->getGpuLut3D(ocio_lut_data, shaderDesc);

  // Upload LUT data to texture
  xf->glTexSubImage3D(GL_TEXTURE_3D, 0,
                      0, 0, 0,
                      OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE,
                      GL_RGB, GL_FLOAT, ocio_lut_data);

  delete [] ocio_lut_data;

  // Create OCIO shader code
  QString shader_text(processor->getGpuShaderText(shaderDesc));

  QString shader_call;

  // Enforce alpha association
  if (alpha_is_associated) {

    // If alpha is already associated, we'll need to disassociate and reassociate
    shader_text.append("\n");

    QString disassociate_func_name = "disassoc";
    shader_text.append(CodeAlphaDisassociate(disassociate_func_name));

    QString reassociate_func_name = "reassoc";
    shader_text.append(CodeAlphaReassociate(reassociate_func_name));

    // Make OCIO call pass through disassociate and reassociate function
    shader_call = QStringLiteral("%3(%1(%2(col), ove_ociolut));").arg(ocio_func_name,
                                                               disassociate_func_name,
                                                               reassociate_func_name);

  } else {

    // If alpha is not already associated, we can just associate after OCIO

    // Add associate function
    QString associate_func_name = "assoc";
    shader_text.append(CodeAlphaAssociate(associate_func_name));

    // Make OCIO call pass through associate function
    shader_call = QStringLiteral("%2(%1(col, ove_ociolut));").arg(ocio_func_name, associate_func_name);

  }

  // Add process() function, which GetPipeline() will call if specified
  QString process_function_name = "process";
  shader_text.append(QStringLiteral("\n"
                                    "uniform sampler3D ove_ociolut;\n"
                                    "\n"
                                    "vec4 %2(vec4 col) {\n"
                                    "  return %1\n"
                                    "}\n").arg(shader_call, process_function_name));


  // Get pipeline-based shader to inject OCIO shader into
  OpenGLShaderPtr shader = OpenGLShader::CreateDefault(process_function_name, shader_text);

  // Release LUT
  xf->glBindTexture(GL_TEXTURE_3D, 0);

  return shader;
}

QString OpenGLShader::CodeDefaultFragment(const QString &function_name, const QString &shader_code)
{
  QString frag_code = QStringLiteral("#version 110\n"
                                     "\n"
                                     "#ifdef GL_ES\n"
                                     "precision highp int;\n"
                                     "precision highp float;\n"
                                     "#endif\n"
                                     "\n"
                                     "uniform sampler2D ove_maintex;\n"
                                     "uniform bool color_only;\n"
                                     "uniform vec4 color_only_color;\n"
                                     "varying vec2 ove_texcoord;\n"
                                     "\n");

  // Finish the function with the main function

  // Check if additional code was passed to this function, add it here
  if (shader_code.isEmpty()) {

    // If not, just add a pure main() function

    frag_code.append(QStringLiteral("\n"
                                    "void main() {\n"
                                    "  if (color_only) {\n"
                                    "    gl_FragColor = color_only_color;"
                                    "  } else {\n"
                                    "    vec4 color = texture2D(ove_maintex, ove_texcoord);\n"
                                    "    gl_FragColor = color;\n"
                                    "  }\n"
                                    "}\n"));

  } else {

    // If additional code was passed, add it and reference it in main().
    //
    // The function in the additional code is expected to be `vec4 function_name(vec4 color)`. The texture coordinate
    // can be acquired through `ove_texcoord`.

    frag_code.append(shader_code);

    frag_code.append(QStringLiteral("\n"
                                    "void main() {\n"
                                    "  vec4 color = %1(texture2D(ove_maintex, ove_texcoord));\n"
                                    "  gl_FragColor = color;\n"
                                    "}\n").arg(function_name));

  }

  return frag_code;
}

QString OpenGLShader::CodeDefaultVertex()
{
  // Generate vertex shader
  return QStringLiteral("#version 110\n"
                        "\n"
                        "#ifdef GL_ES\n"
                        "precision highp int;\n"
                        "precision highp float;\n"
                        "#endif\n"
                        "\n"
                        "uniform mat4 ove_mvpmat;\n"
                        "\n"
                        "attribute vec4 a_position;\n"
                        "attribute vec2 a_texcoord;\n"
                        "\n"
                        "varying vec2 ove_texcoord;\n"
                        "\n"
                        "void main() {\n"
                        "  gl_Position = ove_mvpmat * a_position;\n"
                        "  ove_texcoord = a_texcoord;\n"
                        "}\n");
}

QString OpenGLShader::CodeAlphaDisassociate(const QString &function_name)
{
  return QStringLiteral("vec4 %1(vec4 col) {\n"
                        "  if (col.a > 0.0) {\n"
                        "    return vec4(col.rgb / col.a, col.a);"
                        "  }\n"
                        "  return col;\n"
                        "}\n").arg(function_name);
}

QString OpenGLShader::CodeAlphaReassociate(const QString &function_name)
{
  return QStringLiteral("vec4 %1(vec4 col) {\n"
                        "  if (col.a > 0.0) {\n"
                        "    return vec4(col.rgb * col.a, col.a);"
                        "  }\n"
                        "  return col;\n"
                        "}\n").arg(function_name);
}

QString OpenGLShader::CodeAlphaAssociate(const QString &function_name)
{
  return QStringLiteral("vec4 %1(vec4 col) {\n"
                        "  return vec4(col.rgb * col.a, col.a);\n"
                        "}\n").arg(function_name);
}
