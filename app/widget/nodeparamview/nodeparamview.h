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

#ifndef NODEPARAMVIEW_H
#define NODEPARAMVIEW_H

#include <QVBoxLayout>
#include <QWidget>

#include "node/node.h"
#include "nodeparamviewitem.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/timeruler/timeruler.h"

class NodeParamView : public QWidget
{
  Q_OBJECT
public:
  NodeParamView(QWidget* parent);

  void SetNodes(QList<Node*> nodes);
  const QList<Node*>& nodes();

  const double& GetScale() const;
  void SetScale(const double &scale);

public slots:
  void SetTime(const int64_t& timestamp);

signals:
  void TimeChanged(const int64_t& timestamp);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

private:
  void SetTimebase(const rational& timebase);

  void UpdateItemTime(const int64_t &timestamp);

  QVBoxLayout* param_layout_;

  KeyframeView* keyframe_view_;

  TimeRuler* ruler_;

  QList<Node*> nodes_;

  QList<NodeParamViewItem*> items_;

  QScrollBar* vertical_scrollbar_;

  QGraphicsRectItem* bottom_item_;

  int last_scroll_val_;

private slots:
  void RulerTimeChanged(const int64_t& timestamp);

  void ItemRequestedTimeChanged(const rational& time);

  void ForceKeyframeViewToScroll(int min, int max);

};

#endif // NODEPARAMVIEW_H
