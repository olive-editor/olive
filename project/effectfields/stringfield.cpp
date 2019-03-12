#include "stringfield.h"

#include <QtMath>

#include "ui/texteditex.h"
#include "io/config.h"

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

  text_edit->setUndoRedoEnabled(true);

  // the "2" looks like a magic number, but it's just one pixel on the top and the bottom
  text_edit->setFixedHeight(qCeil(text_edit->fontMetrics().lineSpacing()*olive::CurrentConfig.effect_textbox_lines
                                  + text_edit->document()->documentMargin()
                                  + text_edit->document()->documentMargin() + 2));

  connect(text_edit, SIGNAL(textModified(const QString&)), this, SLOT(UpdateFromWidget(const QString&)));

  return text_edit;
}

void StringField::UpdateFromWidget(const QString &s)
{
  SetValueAt(Now(), s);
}
