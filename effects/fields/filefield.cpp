#include "filefield.h"

#include <QDebug>

#include "ui/embeddedfilechooser.h"

FileField::FileField(EffectRow* parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_FILE)
{
  // Set default value to an empty string
  SetValueAt(0, "");
}

QString FileField::GetFileAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QWidget *FileField::CreateWidget(QWidget *existing)
{
  EmbeddedFileChooser* efc = (existing != nullptr) ? static_cast<EmbeddedFileChooser*>(existing) : new EmbeddedFileChooser();

  connect(efc, SIGNAL(changed(const QString&)), this, SLOT(UpdateFromWidget(const QString&)));
  connect(this, SIGNAL(EnabledChanged(bool)), efc, SLOT(setEnabled(bool)));

  return efc;
}

void FileField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  EmbeddedFileChooser* efc = static_cast<EmbeddedFileChooser*>(widget);

  efc->blockSignals(true);
  efc->setFilename(GetFileAt(timecode));
  efc->blockSignals(false);
}

void FileField::UpdateFromWidget(const QString &s)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), s);

  kdc->SetNewKeyframes();
  olive::UndoStack.push(kdc);
}
