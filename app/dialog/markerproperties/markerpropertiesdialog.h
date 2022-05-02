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

#ifndef MARKERPROPERTIESDIALOG_H
#define MARKERPROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "timeline/timelinemarker.h"
#include "widget/colorlabelmenu/colorcodingcombobox.h"
#include "widget/slider/rationalslider.h"

namespace olive {

class LineEditWithFocusSignal : public QLineEdit
{
  Q_OBJECT
public:
  LineEditWithFocusSignal(QWidget *parent = nullptr) :
    QLineEdit(parent)
  {
  }

protected:
  virtual void focusInEvent(QFocusEvent *e) override
  {
    QLineEdit::focusInEvent(e);
    emit Focused();
  }

signals:
  void Focused();

};

class MarkerPropertiesDialog : public QDialog
{
  Q_OBJECT
public:
  MarkerPropertiesDialog(const std::vector<TimelineMarker*> &markers, const rational &timebase, QWidget *parent = nullptr);

public slots:
  virtual void accept() override;

private:
  std::vector<TimelineMarker*> markers_;

  LineEditWithFocusSignal *label_edit_;

  ColorCodingComboBox *color_menu_;

  RationalSlider *in_slider_;

  RationalSlider *out_slider_;

};

}

#endif // MARKERPROPERTIESDIALOG_H
