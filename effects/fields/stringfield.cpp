#include "stringfield.h"

#include <QtMath>
#include <QDebug>

#include "ui/texteditex.h"
#include "global/config.h"

StringField::StringField(EffectRow* parent, const QString& id, bool rich_text) :
  EffectField(parent, id, EFFECT_FIELD_STRING),
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
    text_edit->setTextHeight(qCeil(text_edit->fontMetrics().lineSpacing()*olive::CurrentConfig.effect_textbox_lines
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
  olive::UndoStack.push(kdc);
}
