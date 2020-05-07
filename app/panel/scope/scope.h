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

#ifndef SCOPE_PANEL_H
#define SCOPE_PANEL_H

#include <QComboBox>
#include <QStackedWidget>
#include <QCheckBox>
#include <QVector>

#include "widget/panel/panel.h"
#include "widget/scope/histogram/histogram.h"
#include "widget/scope/waveform/waveform.h"

OLIVE_NAMESPACE_ENTER

class ViewerPanel;

class ScopePanel : public PanelWidget
{
  Q_OBJECT
public:
  enum Type {
    kTypeWaveform,
    kTypeHistogram,

    kTypeCount
  };

  ScopePanel(QWidget* parent = nullptr);

  void SetType(Type t);

  static QString TypeToName(Type t);

  void SetSwizzleVisibility();

  void SortSwizzleData(int checkbox);

public slots:
  void SetReferenceBuffer(Frame* frame);

  void SetColorManager(ColorManager* manager);

signals:

  void SendSwizzleData(QVector<bool> swizzle);

protected:
  virtual void Retranslate() override;

private:
  Type type_;

  QStackedWidget* stack_;

  QComboBox* scope_type_combobox_;

  WaveformScope* waveform_view_;

  HistogramScope* histogram_;

  QCheckBox* luma_select_;

  QCheckBox* red_select_;

  QCheckBox* green_select_;

  QCheckBox* blue_select_;

  QVector<bool> swizzle_;

};

OLIVE_NAMESPACE_EXIT

#endif // SCOPE_PANEL_H
