/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "qtutils.h"

#include <QDebug>

namespace olive {

int QtUtils::QFontMetricsWidth(QFontMetrics fm, const QString& s) {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
  return fm.width(s);
#else
  return fm.horizontalAdvance(s);
#endif
}

QFrame *QtUtils::CreateHorizontalLine()
{
  QFrame* horizontal_line = new QFrame();
  horizontal_line->setFrameShape(QFrame::HLine);
  horizontal_line->setFrameShadow(QFrame::Sunken);
  return horizontal_line;
}

QFrame *QtUtils::CreateVerticalLine()
{
  QFrame *l = CreateHorizontalLine();
  l->setFrameShape(QFrame::VLine);
  return l;
}

int QtUtils::MsgBox(QWidget *parent, QMessageBox::Icon icon, const QString &title, const QString &message, QMessageBox::StandardButtons buttons)
{
  QMessageBox b(parent);
  b.setIcon(icon);
  b.setWindowModality(Qt::WindowModal);
  b.setWindowTitle(title);
  b.setText(message);

  uint mask = QMessageBox::FirstButton;
  while (mask <= QMessageBox::LastButton) {
    uint sb = buttons & mask;
    if (sb) {
      b.addButton(static_cast<QMessageBox::StandardButton>(sb));
    }
    mask <<= 1;
  }

  return b.exec();
}

QDateTime QtUtils::GetCreationDate(const QFileInfo &info)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
  return info.created();
#else
  QDateTime t = info.birthTime();
  if (!t.isValid()) {
    t = info.metadataChangeTime();
  }
  return t;
#endif
}

QString QtUtils::GetFormattedDateTime(const QDateTime &dt)
{
  return dt.toString(Qt::TextDate);
}

QStringList QtUtils::WordWrapString(const QString &s, const QFontMetrics &fm, int bounding_width)
{
  QStringList list;

  QStringList lines = s.split('\n');

  // Iterate every line
  for (int i=0; i<lines.size(); i++) {
    QString this_line = lines.at(i);

    while (this_line.size() > 1 && QFontMetricsWidth(fm, this_line) >= bounding_width) {
      int old_size = this_line.size();
      int hard_break = -1;

      for (int j=this_line.size()-1; j>=0; j--) {
        const QChar &char_test = this_line.at(j);

        if (char_test.isSpace()
            || char_test == '-') {
          if (QFontMetricsWidth(fm, this_line.left(j)) < bounding_width) {
            if (!char_test.isSpace()) {
              j++;
            }

            QString chopped = this_line.left(j);

            list.append(chopped);

            while (j < this_line.size() && this_line.at(j).isSpace()) {
              j++;
            }
            this_line.remove(0, j);
            break;
          }
        } else if (hard_break == -1 && QFontMetricsWidth(fm, this_line.left(j)) < bounding_width) {
          // In case we can't find a better place to split, split at the earliest time the line
          // goes under the width limit
          hard_break = j;
        }
      }

      if (old_size == this_line.size()) {
        if (hard_break != -1) {
          list.append(this_line.left(hard_break));
          this_line.remove(0, hard_break);
        } else {
          qWarning() << "Failed to find anywhere to wrap. Returning full line.";
          break;
        }
      }
    }

    if (!this_line.isEmpty()) {
      list.append(this_line);
    }
  }

  return list;
}

Qt::KeyboardModifiers QtUtils::FlipControlAndShiftModifiers(Qt::KeyboardModifiers e)
{
  if (e & Qt::ControlModifier & Qt::ShiftModifier) {
    return e;
  }

  if (e & Qt::ShiftModifier) {
    e |= Qt::ControlModifier;
    e &= ~Qt::ShiftModifier;
  } else if (e & Qt::ControlModifier) {
    e |= Qt::ShiftModifier;
    e &= ~Qt::ControlModifier;
  }

  return e;
}

void QtUtils::SetComboBoxData(QComboBox *cb, int data)
{
  for (int i=0; i<cb->count(); i++) {
    if (cb->itemData(i).toInt() == data) {
      cb->setCurrentIndex(i);
      break;
    }
  }
}

void QtUtils::SetComboBoxData(QComboBox *cb, const QString &data)
{
  for (int i=0; i<cb->count(); i++) {
    if (cb->itemData(i).toString() == data) {
      cb->setCurrentIndex(i);
      break;
    }
  }
}

QColor QtUtils::toQColor(const core::Color &i)
{
  QColor c;

  // QColor only supports values from 0.0 to 1.0 and are only used for UI representations
  c.setRedF(std::clamp(i.red(), 0.0f, 1.0f));
  c.setGreenF(std::clamp(i.green(), 0.0f, 1.0f));
  c.setBlueF(std::clamp(i.blue(), 0.0f, 1.0f));
  c.setAlphaF(std::clamp(i.alpha(), 0.0f, 1.0f));

  return c;
}

namespace core {

uint qHash(const core::rational &r, uint seed)
{
  return ::qHash(r.toDouble(), seed);
}

uint qHash(const core::TimeRange &r, uint seed)
{
  return qHash(r.in(), seed) ^ qHash(r.out(), seed);
}


}

}
