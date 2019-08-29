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

#ifndef SHADERGENERATORS_H
#define SHADERGENERATORS_H

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "shaderptr.h"

/**
 *
 * Olive standardizes on OpenGL 3.2 Core which has no fixed pipeline. Instead, the pipeline is provided by the
 * programmer in the form of a shader. This is a collection of OpenGL shader pipeline generators for use throughout
 * Olive.
 *
 */

namespace olive {

class ShaderGenerator {
public:
  static ShaderPtr DefaultPipeline(const QString &function_name = QString(), const QString &shader_code = QString());

  static ShaderPtr OCIOPipeline(QOpenGLContext *ctx,
                                GLuint &lut_texture,
                                OCIO::ConstProcessorRcPtr processor,
                                bool alpha_is_associated);

  static QString AlphaDisassociateFunction(const QString& function_name);
  static QString AlphaReassociateFunction(const QString& function_name);
  static QString AlphaAssociateFunction(const QString& function_name);
};

}

#endif // SHADERGENERATORS_H
