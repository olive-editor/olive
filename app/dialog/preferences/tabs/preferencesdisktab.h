#ifndef PREFERENCESDISKTAB_H
#define PREFERENCESDISKTAB_H

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

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

  FloatSlider* cache_ahead_slider_;

  FloatSlider* cache_behind_slider_;

  QCheckBox* clear_disk_cache_;

  QPushButton* clear_cache_btn_;

private slots:
  void DiskCacheLineEditChanged();

  void BrowseDiskCachePath();

  void ClearDiskCache();

};

#endif // PREFERENCESDISKTAB_H
