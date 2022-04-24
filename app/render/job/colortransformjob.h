/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef COLORTRANSFORMJOB_H
#define COLORTRANSFORMJOB_H

#include <QString>

#include "render/colorprocessor.h"
#include "render/texture.h"

namespace olive {

class ColorTransformJob {
public:
  ColorTransformJob()
  {
    processor_ = nullptr;
    input_texture_ = nullptr;
  }

  TexturePtr GetInputTexture() const { return input_texture_; }
  void SetInputTexture(TexturePtr tex) { input_texture_ = tex; }

  ColorProcessorPtr GetColorProcessor() const { return processor_; }
  void SetColorProcessor(ColorProcessorPtr p) { processor_ = p; }

  QString GetShaderPath() const { return shader_path_; }
  void SetShaderPath(const QString shader_path) { shader_path_ = shader_path; }

private:
  ColorProcessorPtr processor_;

  TexturePtr input_texture_;

  QString shader_path_;

};

}

Q_DECLARE_METATYPE(olive::ColorTransformJob)

#endif // COLORTRANSFORMJOB_H
