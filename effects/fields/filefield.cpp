#include "filefield.h"

#include "ui/embeddedfilechooser.h"

FileField::FileField(EffectRow* parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_FILE)
{

}

QString FileField::GetFileAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QWidget *FileField::CreateWidget()
{
  EmbeddedFileChooser* efc = new EmbeddedFileChooser();

  connect(efc, SIGNAL(changed(const QString&)), this, SLOT(UpdateFromWidget(const QString&)));
  connect(this, SIGNAL(EnabledChanged(bool)), efc, SLOT(setEnabled(bool)));

  return efc;
}

void FileField::UpdateFromWidget(const QString &s)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), s);

  kdc->SetNewKeyframes();
  olive::UndoStack.push(kdc);
}
