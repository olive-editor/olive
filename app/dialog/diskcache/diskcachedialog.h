/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef DISKCACHEDIALOG_H
#define DISKCACHEDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QPushButton>

#include "render/diskmanager.h"
#include "widget/slider/floatslider.h"

OLIVE_NAMESPACE_ENTER

class DiskCacheDialog : public QDialog
{
  Q_OBJECT
public:
  DiskCacheDialog(DiskCacheFolder* folder, QWidget* parent = nullptr);

public slots:
  virtual void accept() override;

private:
  DiskCacheFolder* folder_;

  FloatSlider* maximum_cache_slider_;

  QCheckBox* clear_disk_cache_;

  QPushButton* clear_cache_btn_;

private slots:
  void ClearDiskCache();

};

OLIVE_NAMESPACE_EXIT

#endif // DISKCACHEDIALOG_H
