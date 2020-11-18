/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef SHADERJOB_H
#define SHADERJOB_H

#include <QMatrix4x4>

#include "generatejob.h"
#include "render/texture.h"

namespace olive {

class ShaderJob : public GenerateJob {
public:
  ShaderJob()
  {
    iterations_ = 1;
    iterative_input_ = nullptr;
  }

  const QString& GetShaderID() const
  {
    return shader_id_;
  }

  void SetShaderID(const QString& id)
  {
    shader_id_ = id;
  }

  void SetIterations(int iterations, NodeInput* iterative_input)
  {
    SetIterations(iterations, iterative_input->id());
  }

  void SetIterations(int iterations, const QString& iterative_input)
  {
    iterations_ = iterations;
    iterative_input_ = iterative_input;
  }

  int GetIterationCount() const
  {
    return iterations_;
  }

  const QString& GetIterativeInput() const
  {
    return iterative_input_;
  }

  Texture::Interpolation GetInterpolation(const QString& id)
  {
    return interpolation_.value(id, Texture::kDefaultInterpolation);
  }

  void SetInterpolation(NodeInput* input, Texture::Interpolation interp)
  {
    interpolation_.insert(input->id(), interp);
  }

  void SetInterpolation(const QString& id, Texture::Interpolation interp)
  {
    interpolation_.insert(id, interp);
  }

private:
  QString shader_id_;

  int iterations_;

  QString iterative_input_;

  QHash<QString, Texture::Interpolation> interpolation_;

};

}

Q_DECLARE_METATYPE(olive::ShaderJob)

#endif // SHADERJOB_H
