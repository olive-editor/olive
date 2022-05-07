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

#include <QMatrix4x4>
#include <QString>

#include "render/job/generatejob.h"
#include "render/alphaassoc.h"
#include "render/colorprocessor.h"
#include "render/texture.h"

namespace olive {

class Node;

class ColorTransformJob : public GenerateJob
{
public:
  ColorTransformJob()
  {
    processor_ = nullptr;
    input_texture_ = nullptr;
    custom_shader_src_ = nullptr;
    input_alpha_association_ = kAlphaNone;
    clear_destination_ = true;
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

  TexturePtr GetInputTexture() const { return input_texture_; }
  void SetInputTexture(TexturePtr tex) { input_texture_ = tex; }

  ColorProcessorPtr GetColorProcessor() const { return processor_; }
  void SetColorProcessor(ColorProcessorPtr p) { processor_ = p; }

  const AlphaAssociated &GetInputAlphaAssociation() const { return input_alpha_association_; }
  void SetInputAlphaAssociation(const AlphaAssociated &e) { input_alpha_association_ = e; }

  const Node *CustomShaderSource() const { return custom_shader_src_; }
  const QString &CustomShaderID() const { return custom_shader_id_; }
  void SetNeedsCustomShader(const Node *node, const QString &id = QString())
  {
    custom_shader_src_ = node;
    custom_shader_id_ = id;
  }

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

  TexturePtr input_texture_;

  const Node *custom_shader_src_;
  QString custom_shader_id_;

  AlphaAssociated input_alpha_association_;

  bool clear_destination_;

  QMatrix4x4 matrix_;

  QMatrix4x4 crop_matrix_;

  QString function_name_;

};

}

Q_DECLARE_METATYPE(olive::ColorTransformJob)

#endif // COLORTRANSFORMJOB_H
