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

#ifndef TRANSFORMDISTORTNODE_H
#define TRANSFORMDISTORTNODE_H

#include "node/generator/matrix/matrix.h"
#include "node/gizmo/point.h"
#include "node/gizmo/polygon.h"
#include "node/gizmo/screen.h"

namespace olive {

class TransformDistortNode : public MatrixGenerator
{
  Q_OBJECT
public:
  TransformDistortNode();

  NODE_DEFAULT_FUNCTIONS(TransformDistortNode)

  virtual QString Name() const override
  {
    return tr("Transform");
  }

  virtual QString ShortName() const override
  {
    // Override MatrixGenerator's short name "Ortho"
    return Name();
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.transform");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryDistort};
  }

  virtual QString Description() const override
  {
    return tr("Transform an image in 2D space. Equivalent to multiplying by an orthographic matrix.");
  }

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;

  enum AutoScaleType {
    kAutoScaleNone,
    kAutoScaleFit,
    kAutoScaleFill,
    kAutoScaleStretch
  };

  static QMatrix4x4 AdjustMatrixByResolutions(const QMatrix4x4& mat,
                                              const QVector2D& sequence_res,
                                              const QVector2D& texture_res,
                                              const QVector2D& offset,
                                              AutoScaleType autoscale_type = kAutoScaleNone);

  virtual void UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals) override;

  static const QString kTextureInput;
  static const QString kAutoscaleInput;
  static const QString kInterpolationInput;

protected:
  virtual void Hash(QCryptographicHash& hash, const NodeGlobals &globals, const VideoParams& video_params) const override;

protected slots:
  virtual void GizmoDragStart(const olive::NodeValueRow &row, double x, double y, const olive::rational &time) override;

  virtual void GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers) override;

private:
  static QPointF CreateScalePoint(double x, double y, const QPointF& half_res, const QMatrix4x4& mat);

  QMatrix4x4 GenerateAutoScaledMatrix(const QMatrix4x4 &generated_matrix, const NodeValueRow &db, const NodeGlobals &globals, const VideoParams &texture_params) const;

  bool IsAScaleGizmo(NodeGizmo *g) const;

  // Gizmo variables
  double gizmo_start_angle_;
  QTransform gizmo_inverted_transform_;
  QPointF gizmo_anchor_pt_;
  bool gizmo_scale_uniform_;
  double gizmo_last_angle_;
  double gizmo_last_alt_angle_;
  int gizmo_rotate_wrap_;

  enum RotationDirection {
    kDirectionNone,
    kDirectionPositive, // Clockwise
    kDirectionNegative // Counter-clockwise
  };

  static RotationDirection GetDirectionFromAngles(double last, double current);
  RotationDirection gizmo_rotate_last_dir_;
  RotationDirection gizmo_rotate_last_alt_dir_;

  enum GizmoScaleType {
    kGizmoScaleXOnly,
    kGizmoScaleYOnly,
    kGizmoScaleBoth
  };

  GizmoScaleType gizmo_scale_axes_;
  QVector2D gizmo_scale_anchor_;

  // Gizmo on screen object storage
  PointGizmo *point_gizmo_[kGizmoScaleCount];
  PointGizmo *anchor_gizmo_;
  PolygonGizmo *poly_gizmo_;
  ScreenGizmo *rotation_gizmo_;

};

}

#endif // TRANSFORMDISTORTNODE_H
