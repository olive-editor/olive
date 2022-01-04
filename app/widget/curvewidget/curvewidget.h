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

#ifndef CURVEWIDGET_H
#define CURVEWIDGET_H

#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>

#include "curveview.h"
#include "widget/nodeparamview/nodeparamviewkeyframecontrol.h"
#include "widget/nodeparamview/nodeparamviewwidgetbridge.h"
#include "widget/nodetreeview/nodetreeview.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

class CurveWidget : public TimeBasedWidget, public TimeTargetObject
{
  Q_OBJECT
public:
  CurveWidget(QWidget* parent = nullptr);

  const double& GetVerticalScale();
  void SetVerticalScale(const double& vscale);

  void DeleteSelected();

  void SelectAll()
  {
    view_->SelectAll();
  }

  void DeselectAll()
  {
    view_->DeselectAll();
  }

public slots:
  void SetNodes(const QVector<Node *> &nodes);

protected:
  virtual void TimeChangedEvent(const rational &) override;
  virtual void TimebaseChangedEvent(const rational &) override;
  virtual void ScaleChangedEvent(const double &) override;

  virtual void TimeTargetChangedEvent(Node* target) override;

  virtual void ConnectedNodeChangeEvent(ViewerOutput* n) override;

private:
  void SetKeyframeButtonEnabled(bool enable);

  void SetKeyframeButtonChecked(bool checked);

  void SetKeyframeButtonCheckedFromType(NodeKeyframe::Type type);

  void UpdateBridgeTime(const rational &time);

  void ConnectInput(Node *node, const QString &input, int element);

  void ConnectInputInternal(Node *node, const QString &input, int element);

  QHash<NodeKeyframeTrackReference, QColor> keyframe_colors_;

  NodeTreeView* tree_view_;

  QPushButton* linear_button_;

  QPushButton* bezier_button_;

  QPushButton* hold_button_;

  CurveView* view_;

  NodeParamViewKeyframeControl* key_control_;

  QVector<Node*> nodes_;

  QVector<NodeKeyframeTrackReference> selected_tracks_;

private slots:
  void SelectionChanged();

  void KeyframeTypeButtonTriggered(bool checked);

  void InputSelectionChanged(const NodeKeyframeTrackReference& ref);

  void KeyframeViewDragged(int x, int y);

  void CatchUpYScrollToPoint(int point);

};

}

#endif // CURVEWIDGET_H
