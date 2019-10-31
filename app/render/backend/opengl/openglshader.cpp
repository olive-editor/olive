#include "openglshader.h"

OpenGLShader::OpenGLShader()
{

}

OpenGLShaderPtr OpenGLShader::CreateDefault(const QString &function_name, const QString &shader_code)
{
  OpenGLShaderPtr program = std::make_shared<OpenGLShader>();

  // Add shaders to program
  program->addShaderFromSourceCode(QOpenGLShader::Vertex, CodeDefaultVertex());
  program->addShaderFromSourceCode(QOpenGLShader::Fragment, CodeDefaultFragment(function_name, shader_code));
  program->link();

  return program;
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
                                     "uniform sampler2D texture;\n"
                                     "uniform bool color_only;\n"
                                     "uniform vec4 color_only_color;\n"
                                     "varying vec2 v_texcoord;\n"
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
                                    "    vec4 color = texture2D(texture, v_texcoord);\n"
                                    "    gl_FragColor = color;\n"
                                    "  }\n"
                                    "}\n"));

  } else {

    // If additional code was passed, add it and reference it in main().
    //
    // The function in the additional code is expected to be `vec4 function_name(vec4 color)`. The texture coordinate
    // can be acquired through `v_texcoord`.

    frag_code.append(shader_code);

    frag_code.append(QString(QStringLiteral("\n"
                                            "void main() {\n"
                                            "  vec4 color = %1(texture2D(texture, v_texcoord));\n"
                                            "  gl_FragColor = color;\n"
                                            "}\n")).arg(function_name));

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
                        "uniform mat4 mvp_matrix;\n"
                        "\n"
                        "attribute vec4 a_position;\n"
                        "attribute vec2 a_texcoord;\n"
                        "\n"
                        "varying vec2 v_texcoord;\n"
                        "\n"
                        "void main() {\n"
                        "  gl_Position = mvp_matrix * a_position;\n"
                        "  v_texcoord = a_texcoord;\n"
                        "}\n");
}

QString OpenGLShader::CodeAlphaDisassociate(const QString &function_name)
{
  return QString(QStringLiteral("vec4 %1(vec4 col) {\n"
                                "  if (col.a > 0.0) {\n"
                                "    return vec4(col.rgb / col.a, col.a);"
                                "  }\n"
                                "  return col;\n"
                                "}\n")).arg(function_name);
}

QString OpenGLShader::CodeAlphaReassociate(const QString &function_name)
{
  return QString(QStringLiteral("vec4 %1(vec4 col) {\n"
                                "  if (col.a > 0.0) {\n"
                                "    return vec4(col.rgb * col.a, col.a);"
                                "  }\n"
                                "  return col;\n"
                                "}\n")).arg(function_name);
}

QString OpenGLShader::CodeAlphaAssociate(const QString &function_name)
{
  return QString(QStringLiteral("vec4 %1(vec4 col) {\n"
                                "  return vec4(col.rgb * col.a, col.a);\n"
                                "}\n")).arg(function_name);
}
