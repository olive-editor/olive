/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "textparamwidget.h"

#include "widget/nodeparamview/nodeparamviewtextedit.h"

namespace olive {

#define super AbstractParamWidget

TextParamWidget::TextParamWidget(QObject *parent)
  : super{parent}
{

}

void TextParamWidget::Initialize(QWidget *parent, size_t channels)
{
  for (size_t i = 0; i < channels; i++) {
    NodeParamViewTextEdit* line_edit = new NodeParamViewTextEdit(parent);
    AddWidget(line_edit);
    connect(line_edit, &NodeParamViewTextEdit::textEdited, this, &TextParamWidget::Arbitrate);
    connect(line_edit, &NodeParamViewTextEdit::RequestEditInViewer, this, &TextParamWidget::RequestEditInViewer);
  }
}

void TextParamWidget::SetValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    NodeParamViewTextEdit* e = static_cast<NodeParamViewTextEdit*>(GetWidgets().at(i));
    e->setTextPreservingCursor(val.value<QString>(i));
  }
}

void TextParamWidget::SetProperty(const QString &key, const value_t &val)
{
  if (key == QStringLiteral("vieweronly")) {
    for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
      static_cast<NodeParamViewTextEdit *>(GetWidgets().at(i))->SetEditInViewerOnlyMode(val.value<int64_t>(i));
    }
  } else {
    super::SetProperty(key, val);
  }
}

void TextParamWidget::Arbitrate()
{
  NodeParamViewTextEdit* ff = static_cast<NodeParamViewTextEdit*>(sender());

  for (size_t i = 0; i < GetWidgets().size(); i++) {
    if (GetWidgets().at(i) == ff) {
      emit ChannelValueChanged(i, ff->text());
      break;
    }
  }
}

}
