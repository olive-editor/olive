/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include <QMatrix4x4>
#include <QString>

#include "acceleratedjob.h"
#include "render/alphaassoc.h"
#include "render/colorprocessor.h"
#include "render/texture.h"
#include "shaderjob.h"

namespace olive {

class Node;

class ColorTransformJob : public AcceleratedJob
{
public:
  ColorTransformJob()
  {
    processor_ = nullptr;
    custom_shader_ = nullptr;
    input_alpha_association_ = kAlphaNone;
    clear_destination_ = true;
  }

  ColorTransformJob(const NodeValueRow &row) :
    ColorTransformJob()
  {
    Insert(row);
  }

  QString id() const
  {
    if (id_.isEmpty()) {
      return processor_->id();
    } else {
      return id_;
    }
  }

  void SetOverrideID(const QString &id) { id_ = id; }

  const value_t &GetInputTexture() const { return input_texture_; }
  void SetInputTexture(const value_t &tex) { input_texture_ = tex; }
  void SetInputTexture(TexturePtr tex)
  {
    Q_ASSERT(!tex->IsDummy());
    input_texture_ = tex;
  }

  ColorProcessorPtr GetColorProcessor() const { return processor_; }
  void SetColorProcessor(ColorProcessorPtr p) { processor_ = p; }

  const AlphaAssociated &GetInputAlphaAssociation() const { return input_alpha_association_; }
  void SetInputAlphaAssociation(const AlphaAssociated &e) { input_alpha_association_ = e; }

  ShaderJob::GetShaderCodeFunction_t GetCustomShaderFunction() const { return custom_shader_; }
  void SetCustomShaderFunction(ShaderJob::GetShaderCodeFunction_t shader) { custom_shader_ = shader; }

  bool IsClearDestinationEnabled() const { return clear_destination_; }
  void SetClearDestinationEnabled(bool e) { clear_destination_ = e; }

  const QMatrix4x4 &GetTransformMatrix() const { return matrix_; }
  void SetTransformMatrix(const QMatrix4x4 &m) { matrix_ = m; }

  const QMatrix4x4 &GetCropMatrix() const { return crop_matrix_; }
  void SetCropMatrix(const QMatrix4x4 &m) { crop_matrix_ = m; }

  const QString &GetFunctionName() const { return function_name_; }
  void SetFunctionName(const QString &function_name = QString()) { function_name_ = function_name; };

private:
  ColorProcessorPtr processor_;
  QString id_;

  value_t input_texture_;

  ShaderJob::GetShaderCodeFunction_t custom_shader_;

  AlphaAssociated input_alpha_association_;

  bool clear_destination_;

  QMatrix4x4 matrix_;

  QMatrix4x4 crop_matrix_;

  QString function_name_;

};

}

#endif // COLORTRANSFORMJOB_H
