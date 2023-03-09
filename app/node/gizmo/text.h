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

#ifndef TEXTGIZMO_H
#define TEXTGIZMO_H

#include "gizmo.h"
#include "node/param.h"

namespace olive {

class TextGizmo : public NodeGizmo
{
  Q_OBJECT
public:
  explicit TextGizmo(QObject *parent = nullptr);

  const QRectF &GetRect() const { return rect_; }
  void SetRect(const QRectF &r);

  const QString &GetHtml() const { return text_; }
  void SetHtml(const QString &t) { text_ = t; }

  void SetInput(const NodeKeyframeTrackReference &input) { input_ = input; }

  void UpdateInputHtml(const QString &s, const rational &time);

  Qt::Alignment GetVerticalAlignment() const { return valign_; }
  void SetVerticalAlignment(Qt::Alignment va);

signals:
  void Activated();
  void Deactivated();
  void VerticalAlignmentChanged(Qt::Alignment va);
  void RectChanged(const QRectF &r);

private:
  QRectF rect_;

  QString text_;

  NodeKeyframeTrackReference input_;

  Qt::Alignment valign_;

};

}

#endif // TEXTGIZMO_H
