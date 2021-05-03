/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef FRAMERATECOMBOBOX_H
#define FRAMERATECOMBOBOX_H

#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

#include "common/rational.h"
#include "render/videoparams.h"

namespace olive {

class FrameRateComboBox : public QWidget
{
  Q_OBJECT
public:
  FrameRateComboBox(QWidget* parent = nullptr) :
    QWidget(parent)
  {
    inner_ = new QComboBox();

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(inner_);

    RepopulateList();

    old_index_ = 0;

    connect(inner_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FrameRateComboBox::IndexChanged);
  }

  rational GetFrameRate() const
  {
    if (inner_->currentIndex() == inner_->count() - 1) {
      return custom_rate_;
    } else {
      return inner_->currentData().value<rational>();
    }
  }

  void SetFrameRate(const rational& r)
  {
    int standard_rates = inner_->count() - 1;
    for (int i=0; i<standard_rates; i++) {
      if (inner_->itemData(i).value<rational>() == r) {
        // Set standard frame rate
        old_index_ = i;
        SetInnerIndexWithoutSignal(i);
        return;
      }
    }

    // If we're here, set a custom rate
    custom_rate_ = r;
    old_index_ = inner_->count()-1;
    SetInnerIndexWithoutSignal(old_index_);
    RepopulateList();
  }

signals:
  void FrameRateChanged(const rational& frame_rate);

protected:
  virtual void changeEvent(QEvent* event) override
  {
    QWidget::changeEvent(event);

    if (event->type() == QEvent::LanguageChange) {
      RepopulateList();
    }
  }

private slots:
  void IndexChanged(int index)
  {
    if (index == inner_->count() - 1) {
      // Custom
      QString s;
      bool ok;

      if (!custom_rate_.isNull()) {
        s = QString::number(custom_rate_.toDouble());
      }

      while (true) {
        s = QInputDialog::getText(this, tr("Custom Frame Rate"), tr("Enter custom frame rate:"),
                                  QLineEdit::Normal, s, &ok);

        if (ok) {
          rational r;

          // Try converting to double, assuming most users will input frame rates this way
          double d = s.toDouble(&ok);

          if (ok) {
            // Try converting from double
            r = rational::fromDouble(d, &ok);
          } else {
            // Try converting to rational in case someone formatted that way
            r = rational::fromString(s, &ok);
          }

          if (ok) {

            custom_rate_ = r;
            emit FrameRateChanged(r);
            old_index_ = index;
            RepopulateList();
            break;

          } else {
            // Show message and continue loop
            QMessageBox::critical(this, tr("Invalid Input"), tr("Failed to convert \"%1\" to a frame rate.").arg(s));
          }

        } else {

          // User cancelled, revert to original value
          SetInnerIndexWithoutSignal(old_index_);
          break;

        }
      }
    } else {
      old_index_ = index;
      emit FrameRateChanged(GetFrameRate());
    }
  }

private:
  void RepopulateList()
  {
    int temp_index = inner_->currentIndex();

    inner_->blockSignals(true);

    inner_->clear();

    foreach (const rational& fr, VideoParams::kSupportedFrameRates) {
      inner_->addItem(VideoParams::FrameRateToString(fr), QVariant::fromValue(fr));
    }

    if (custom_rate_.isNull()) {
      inner_->addItem(tr("Custom..."));
    } else {
      inner_->addItem(tr("Custom (%1)").arg(VideoParams::FrameRateToString(custom_rate_)));
    }

    inner_->setCurrentIndex(temp_index);

    inner_->blockSignals(false);
  }

  void SetInnerIndexWithoutSignal(int index)
  {
    inner_->blockSignals(true);
    inner_->setCurrentIndex(index);
    inner_->blockSignals(false);
  }

  QComboBox* inner_;

  rational custom_rate_;

  int old_index_;

};

}

#endif // FRAMERATECOMBOBOX_H
