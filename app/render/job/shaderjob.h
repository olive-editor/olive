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

#ifndef SHADERJOB_H
#define SHADERJOB_H

#include <QMatrix4x4>
#include <QVector>

#include "acceleratedjob.h"
#include "common/filefunctions.h"
#include "render/texture.h"

namespace olive {

class ShaderCode {
public:
  ShaderCode(const QString& frag_code = QString(), const QString& vert_code = QString()) :
    frag_code_(frag_code),
    vert_code_(vert_code)
  {
  }

  const QString& frag_code() const { return frag_code_; }
  void set_frag_code(const QString &f) { frag_code_ = f; }

  const QString& vert_code() const { return vert_code_; }
  void set_vert_code(const QString &v) { vert_code_ = v; }

private:
  QString frag_code_;

  QString vert_code_;

};

class ShaderJob : public AcceleratedJob
{
public:
  typedef ShaderCode (*GetShaderCodeFunction_t)(const QString &id);

  ShaderJob()
  {
    iterations_ = 1;
    iterative_input_ = nullptr;
    function_ = nullptr;
  }

  ShaderJob(const NodeValueRow &row) :
    ShaderJob()
  {
    Insert(row);
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

  Texture::Interpolation GetInterpolation(const QString& id) const
  {
    return interpolation_.value(id, Texture::kDefaultInterpolation);
  }

  const QHash<QString, Texture::Interpolation> &GetInterpolationMap() const
  {
    return interpolation_;
  }

  void SetInterpolation(const QString& id, Texture::Interpolation interp)
  {
    interpolation_.insert(id, interp);
  }

  void SetVertexCoordinates(const QVector<float> &vertex_coords)
  {
    vertex_overrides_ = vertex_coords;
  }

  const QVector<float>& GetVertexCoordinates()
  {
    return vertex_overrides_;
  }

  GetShaderCodeFunction_t function() const { return function_; }
  void set_function(GetShaderCodeFunction_t f) { function_ = f; }
  ShaderCode do_function() const
  {
    return function_ ? function_(GetShaderID()) : ShaderCode();
  }

private:
  int iterations_;

  QString iterative_input_;

  QHash<QString, Texture::Interpolation> interpolation_;

  QVector<float> vertex_overrides_;

  GetShaderCodeFunction_t function_;

};

}

#endif // SHADERJOB_H
