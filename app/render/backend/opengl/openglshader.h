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

#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include <memory>
#include <QOpenGLShaderProgram>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class OpenGLShader;
using OpenGLShaderPtr = std::shared_ptr<OpenGLShader>;

/**
 * @brief A simple QOpenGLShaderProgram derivative with static functions for creating
 */
class OpenGLShader : public QOpenGLShaderProgram {
public:
  OpenGLShader() = default;

  static OpenGLShaderPtr Create();

  static OpenGLShaderPtr CreateDefault(const QString &function_name = QString(),
                                       const QString &shader_code = QString());

  static OpenGLShaderPtr CreateOCIO(QOpenGLContext* ctx,
                                    GLuint& lut_texture,
                                    OCIO::ConstProcessorRcPtr processor,
                                    bool alpha_is_associated);

  static QString CodeDefaultFragment(const QString &function_name = QString(),
                                     const QString &shader_code = QString());
  static QString CodeDefaultVertex();
  static QString CodeAlphaDisassociate(const QString& function_name);
  static QString CodeAlphaReassociate(const QString& function_name);
  static QString CodeAlphaAssociate(const QString& function_name);

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLSHADER_H
