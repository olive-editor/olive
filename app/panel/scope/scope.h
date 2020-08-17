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

public slots:
  void SetReferenceBuffer(Frame* frame);

  void SetColorManager(ColorManager* manager);

protected:
  virtual void Retranslate() override;

private:
  Type type_;

  QStackedWidget* stack_;

  QStackedWidget* stack_ui_;

  QComboBox* scope_type_combobox_;

  WaveformScope* waveform_view_;

  HistogramScope* histogram_;

};

OLIVE_NAMESPACE_EXIT

#endif // SCOPE_PANEL_H
