#ifndef PREFERENCESTAB_H
#define PREFERENCESTAB_H

#include <QWidget>

#include "config/config.h"

class PreferencesTab : public QWidget
{
public:
  PreferencesTab();

  virtual void Accept() = 0;
};

#endif // PREFERENCESTAB_H
