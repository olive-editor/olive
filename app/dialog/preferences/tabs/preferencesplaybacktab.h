#ifndef PREFERENCESPLAYBACKTAB_H
#define PREFERENCESPLAYBACKTAB_H

#include <QComboBox>
#include <QDoubleSpinBox>

#include "preferencestab.h"

class PreferencesPlaybackTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesPlaybackTab();

  virtual void Accept() override;

private:
};

#endif // PREFERENCESPLAYBACKTAB_H
