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

#ifndef OPENGLCOLORPROCESSOR_H
#define OPENGLCOLORPROCESSOR_H

#include "openglshader.h"
#include "render/colorprocessor.h"

OLIVE_NAMESPACE_ENTER

class OpenGLColorProcessor;
using OpenGLColorProcessorPtr = std::shared_ptr<OpenGLColorProcessor>;

class OpenGLColorProcessor : public QObject, public ColorProcessor
{
  Q_OBJECT
public:
  OpenGLColorProcessor(ColorManager *config, const QString &source_space, const QString &dest_space);

  OpenGLColorProcessor(ColorManager *config,
                       const QString& source_space,
                       QString display,
                       QString view,
                       const QString& look,
                       Direction dir);

  ~OpenGLColorProcessor();

  static OpenGLColorProcessorPtr Create(ColorManager* config, const QString& source_space, const QString& dest_space);

  static OpenGLColorProcessorPtr Create(ColorManager* config,
                                        const QString& source_space,
                                        const QString& display,
                                        const QString& view,
                                        const QString& look,
                                        Direction dir = kNormal);

  void Enable(QOpenGLContext* context, bool alpha_is_associated);
  bool IsEnabled() const;

  OpenGLShaderPtr pipeline() const;

  void ProcessOpenGL(bool flipped = false, const QMatrix4x4& matrix = QMatrix4x4());

private:
  QOpenGLContext* context_;

  GLuint ocio_lut_;

  OpenGLShaderPtr pipeline_;

private slots:
  void ClearTexture();

};

OLIVE_NAMESPACE_EXIT

#endif // OPENGLCOLORPROCESSOR_H
