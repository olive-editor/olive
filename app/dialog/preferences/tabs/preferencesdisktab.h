#ifndef PREFERENCESDISKTAB_H
#define PREFERENCESDISKTAB_H

#include <QLineEdit>

#include "preferencestab.h"
#include "widget/slider/floatslider.h"

class PreferencesDiskTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesDiskTab();

  virtual void Accept() override;

private:
  QLineEdit* disk_cache_location_;

  FloatSlider* maximum_cache_slider_;

private slots:
  void DiskCacheLineEditChanged();

  void BrowseDiskCachePath();

  void ClearDiskCache();

};

#endif // PREFERENCESDISKTAB_H
