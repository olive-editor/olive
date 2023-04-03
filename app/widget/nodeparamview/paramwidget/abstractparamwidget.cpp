/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "abstractparamwidget.h"

#include <QWidget>

namespace olive {

AbstractParamWidget::AbstractParamWidget(QObject *parent) :
  QObject{parent}
{

}

AbstractParamWidget::~AbstractParamWidget()
{
  qDeleteAll(widgets_);
}

void AbstractParamWidget::SetProperty(const QString &key, const value_t &value)
{
  if (key == QStringLiteral("tooltip")) {
    for (size_t i = 0; i < widgets_.size(); i++) {
      widgets_.at(i)->setToolTip(value.toString());
    }
  } else {
    bool key_is_disable = key.startsWith(QStringLiteral("disable"));
    if (key_is_disable || key.startsWith(QStringLiteral("enabled"))) {

      bool e = value.toBool();
      if (key_is_disable) {
        e = !e;
      }

      if (key.size() == 7) { // just the word "disable" or "enabled"
        for (size_t i=0; i<widgets_.size(); i++) {
          widgets_.at(i)->setEnabled(e);
        }
      } else { // set specific track/widget
        bool ok;
        size_t element = key.midRef(7).toInt(&ok);

        if (ok && element < widgets_.size()) {
          widgets_.at(element)->setEnabled(e);
        }
      }

    }
  }
}

void AbstractParamWidget::ArbitrateSliders()
{
  NumericSliderBase *w = static_cast<NumericSliderBase *>(sender());

  for (size_t i = 0; i < GetWidgets().size(); i++) {
    if (GetWidgets().at(i) == w) {
      //emit ChannelValueChanged(i, w->GetValue());
      emit SliderDragged(w, i);
      break;
    }
  }
}

}
