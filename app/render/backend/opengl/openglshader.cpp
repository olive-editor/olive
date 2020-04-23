/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "openglshader.h"

#include <QOpenGLExtraFunctions>
OLIVE_NAMESPACE_ENTER

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
const int OCIO_NUM_3D_ENTRIES = 3*OCIO_LUT3D_EDGE_SIZE*OCIO_LUT3D_EDGE_SIZE*OCIO_LUT3D_EDGE_SIZE;

OpenGLShaderPtr OpenGLShader::CreateOCIO(QOpenGLContext* ctx,
                                         GLuint& lut_texture,
                                         OCIO::ConstProcessorRcPtr processor,
                                         bool alpha_is_associated)
{
  QOpenGLExtraFunctions* xf = ctx->extraFunctions();

  // Set up shader description
  OCIO::GpuShaderDesc shaderDesc;
  const char* ocio_func_name = "OCIODisplay";
  shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
  shaderDesc.setFunctionName(ocio_func_name);
  shaderDesc.setLut3DEdgeLen(OCIO_LUT3D_EDGE_SIZE);

  // Compute LUT
  GLfloat* ocio_lut_data = new GLfloat[OCIO_NUM_3D_ENTRIES];
  processor->getGpuLut3D(ocio_lut_data, shaderDesc);

  // Create LUT texture
  xf->glGenTextures(1, &lut_texture);

  // Bind LUT
  xf->glActiveTexture(GL_TEXTURE1);
  xf->glBindTexture(GL_TEXTURE_3D, lut_texture);

  // Set texture parameters
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  xf->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // Allocate storage for texture
  xf->glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F,
                   OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE, OCIO_LUT3D_EDGE_SIZE,
                   0, GL_RGB, GL_FLOAT, ocio_lut_data);

  // Delete local copy
  delete [] ocio_lut_data;

  // Create OCIO shader code
  QString shader_text;

  // Workaround since OCIO doesn't support the GLSL version we use
  shader_text.append(QStringLiteral("#define texture2D texture\n"
                                    "#define texture3D texture\n"));

  // Append OCIO shader code
  shader_text.append(processor->getGpuShaderText(shaderDesc));

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

  xf->glActiveTexture(GL_TEXTURE0);

  return shader;
}

QString OpenGLShader::CodeDefaultFragment(const QString &function_name, const QString &shader_code)
{
  QString frag_code = QStringLiteral("#version 150\n"
                                     "\n"
                                     "#ifdef GL_ES\n"
                                     "precision highp int;\n"
                                     "precision highp float;\n"
                                     "#endif\n"
                                     "\n"
                                     "uniform sampler2D ove_maintex;\n"
                                     "uniform bool color_only;\n"
                                     "uniform vec4 color_only_color;\n"
                                     "\n"
                                     "in vec2 ove_texcoord;\n"
                                     "\n"
                                     "out vec4 fragColor;\n"
                                     "\n");

  // Finish the function with the main function

  // Check if additional code was passed to this function, add it here
  if (shader_code.isEmpty()) {

    // If not, just add a pure main() function

    frag_code.append(QStringLiteral("\n"
                                    "void main() {\n"
                                    "    if (color_only) {\n"
                                    "        fragColor = color_only_color;"
                                    "    } else {\n"
                                    "        vec4 color = texture(ove_maintex, ove_texcoord);\n"
                                    "        fragColor = color;\n"
                                    "    }\n"
                                    "}\n"));

  } else {

    // If additional code was passed, add it and reference it in main().
    //
    // The function in the additional code is expected to be `vec4 function_name(vec4 color)`. The texture coordinate
    // can be acquired through `ove_texcoord`.

    frag_code.append(shader_code);

    frag_code.append(QStringLiteral("\n"
                                    "void main() {\n"
                                    "    vec4 color = %1(texture(ove_maintex, ove_texcoord));\n"
                                    "    fragColor = color;\n"
                                    "}\n").arg(function_name));

  }

  return frag_code;
}

QString OpenGLShader::CodeDefaultVertex()
{
  // Generate vertex shader
  return QStringLiteral("#version 150\n"
                        "\n"
                        "#ifdef GL_ES\n"
                        "precision highp int;\n"
                        "precision highp float;\n"
                        "#endif\n"
                        "\n"
                        "uniform mat4 ove_mvpmat;\n"
                        "\n"
                        "in vec4 a_position;\n"
                        "in vec2 a_texcoord;\n"
                        "\n"
                        "out vec2 ove_texcoord;\n"
                        "\n"
                        "void main() {\n"
                        "    gl_Position = ove_mvpmat * a_position;\n"
                        "    ove_texcoord = a_texcoord;\n"
                        "}\n");
}

QString OpenGLShader::CodeAlphaDisassociate(const QString &function_name)
{
  return QStringLiteral("vec4 %1(vec4 col) {\n"
                        "    if (col.a > 0.0) {\n"
                        "        return vec4(col.rgb / col.a, col.a);"
                        "    }\n"
                        "    return col;\n"
                        "}\n").arg(function_name);
}

QString OpenGLShader::CodeAlphaReassociate(const QString &function_name)
{
  return QStringLiteral("vec4 %1(vec4 col) {\n"
                        "    if (col.a > 0.0) {\n"
                        "        return vec4(col.rgb * col.a, col.a);"
                        "    }\n"
                        "    return col;\n"
                        "}\n").arg(function_name);
}

QString OpenGLShader::CodeAlphaAssociate(const QString &function_name)
{
  return QStringLiteral("vec4 %1(vec4 col) {\n"
                        "    return vec4(col.rgb * col.a, col.a);\n"
                        "}\n").arg(function_name);
}

OLIVE_NAMESPACE_EXIT
