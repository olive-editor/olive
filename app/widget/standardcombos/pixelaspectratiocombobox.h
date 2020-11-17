/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef PIXELASPECTRATIOCOMBOBOX_H
#define PIXELASPECTRATIOCOMBOBOX_H

#include <QComboBox>

#include "common/ratiodialog.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class PixelAspectRatioComboBox : public QComboBox
{
  Q_OBJECT
public:
  PixelAspectRatioComboBox(QWidget* parent = nullptr) :
    QComboBox(parent),
    dont_prompt_custom_par_(false)
  {
    QStringList par_names = VideoParams::GetStandardPixelAspectRatioNames();
    for (int i=0; i<VideoParams::kStandardPixelAspects.size(); i++) {
      const rational& ratio = VideoParams::kStandardPixelAspects.at(i);

      this->addItem(par_names.at(i),
                                   QVariant::fromValue(ratio));
    }

    // Always add custom item last, much of the logic relies on this. Set this to the current AR so
    // that if none of the above are ==, it will eventually select this item
    this->addItem(QString());
    UpdateCustomItem(rational());

    // Pick up index signal to query for custom aspect ratio if requested
    connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PixelAspectRatioComboBox::IndexChanged);
  }

  rational GetPixelAspectRatio() const
  {
    return this->currentData().value<rational>();
  }

  void SetPixelAspectRatio(const rational& r)
  {
    // Determine which index to select on startup
    for (int i=0; i<this->count(); i++) {
      if (this->itemData(i).value<rational>() == r) {
        this->setCurrentIndex(i);
        return;
      }
    }

    // Must not have found the ratio, so it must be custom
    UpdateCustomItem(r);
    dont_prompt_custom_par_ = true;
    this->setCurrentIndex(this->count() - 1);
    dont_prompt_custom_par_ = false;
  }

private slots:
  void IndexChanged(int index)
  {
    if (dont_prompt_custom_par_) {
      return;
    }

    // Detect if custom was selected, in which case query what the new AR should be
    if (index == this->count() - 1) {
      // Query for custom pixel aspect ratio
      bool ok;

      double custom_ratio = GetFloatRatioFromUser(this,
                                                  tr("Set Custom Pixel Aspect Ratio"),
                                                  &ok);

      if (ok) {
        UpdateCustomItem(rational::fromDouble(custom_ratio));
      }
    }
  }

private:
  void UpdateCustomItem(const rational &ratio)
  {
    const int custom_index = this->count() - 1;

    if (ratio.isNull()) {
      this->setItemText(custom_index,
                        tr("Custom..."));

      // Use 1:1 to prevent any real chance of the PAR being set to 0
      this->setItemData(custom_index,
                        QVariant::fromValue(rational(1)));
    } else {
      this->setItemText(custom_index,
                        VideoParams::FormatPixelAspectRatioString(tr("Custom (%1)"), ratio));
      this->setItemData(custom_index,
                        QVariant::fromValue(ratio));
    }
  }

  bool dont_prompt_custom_par_;

};

OLIVE_NAMESPACE_EXIT

#endif // PIXELASPECTRATIOCOMBOBOX_H
