/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "stringfield.h"

#include <QtMath>
#include <QDebug>

#include "ui/texteditex.h"
#include "global/config.h"

StringField::StringField(EffectRow* parent, const QString& id, bool rich_text) :
  EffectField(parent, id, olive::nodes::kString),
  rich_text_(rich_text)
{
  // Set default value to an empty string
  SetValueAt(0, "");
}

QString StringField::GetStringAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QWidget *StringField::CreateWidget(QWidget *existing)
{
  TextEditEx* text_edit;

  if (existing == nullptr) {

    text_edit = new TextEditEx(nullptr, rich_text_);

    text_edit->setEnabled(IsEnabled());
    text_edit->setUndoRedoEnabled(true);

    // the "2" is because the height needs one extra pixel of padding on the top and the bottom
    text_edit->setTextHeight(qCeil(text_edit->fontMetrics().lineSpacing()*olive::config.effect_textbox_lines
                                   + text_edit->document()->documentMargin()
                                   + text_edit->document()->documentMargin() + 2));

  } else {

    text_edit = static_cast<TextEditEx*>(existing);

  }

  connect(text_edit, SIGNAL(textModified(const QString&)), this, SLOT(UpdateFromWidget(const QString&)));
  connect(this, SIGNAL(EnabledChanged(bool)), text_edit, SLOT(setEnabled(bool)));

  return text_edit;
}

void StringField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  TextEditEx* text = static_cast<TextEditEx*>(widget);

  text->blockSignals(true);

  int pos = text->textCursor().position();

  if (rich_text_) {
    text->setHtml(GetValueAt(timecode).toString());
  } else {
    text->setPlainText(GetValueAt(timecode).toString());
  }

  QTextCursor new_cursor(text->document());
  new_cursor.setPosition(pos);
  text->setTextCursor(new_cursor);

  text->blockSignals(false);
}

void StringField::UpdateFromWidget(const QString &s)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), s);

  kdc->SetNewKeyframes();
  olive::undo_stack.push(kdc);
}
