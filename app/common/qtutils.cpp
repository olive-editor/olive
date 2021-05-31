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

#include "qtutils.h"

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

int QtUtils::MessageBox(QWidget *parent, QMessageBox::Icon icon, const QString &title, const QString &message, QMessageBox::StandardButtons buttons)
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

}
