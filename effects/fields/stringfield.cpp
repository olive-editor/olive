#include "stringfield.h"

#include <QtMath>
#include <QDebug>

#include "ui/texteditex.h"
#include "global/config.h"

StringField::StringField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_STRING)
{

}

QString StringField::GetStringAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QWidget *StringField::CreateWidget()
{
  TextEditEx* text_edit = new TextEditEx();

  text_edit->setEnabled(IsEnabled());
  text_edit->setUndoRedoEnabled(true);

  // the "2" looks like a magic number, but it's just one pixel on the top and the bottom
  text_edit->setFixedHeight(qCeil(text_edit->fontMetrics().lineSpacing()*olive::CurrentConfig.effect_textbox_lines
                                  + text_edit->document()->documentMargin()
                                  + text_edit->document()->documentMargin() + 2));

  connect(text_edit, SIGNAL(textModified(const QString&)), this, SLOT(UpdateFromWidget(const QString&)));
  connect(this, SIGNAL(EnabledChanged(bool)), text_edit, SLOT(setEnabled(bool)));

  return text_edit;
}

void StringField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  TextEditEx* text = static_cast<TextEditEx*>(widget);

  text->blockSignals(true);

  QTextCursor cursor = text->textCursor();
  text->setHtml(GetValueAt(timecode).toString());
  text->setTextCursor(cursor);

  text->blockSignals(false);
}

void StringField::UpdateFromWidget(const QString &s)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), s);

  kdc->SetNewKeyframes();
  olive::UndoStack.push(kdc);
}
