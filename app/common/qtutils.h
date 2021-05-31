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

#ifndef QTVERSIONABSTRACTION_H
#define QTVERSIONABSTRACTION_H

/**
 *
 * A fairly simple header for reducing the amount of Qt version checks necessary throughout the code
 *
 */

#include <QFontMetrics>
#include <QFrame>
#include <QMessageBox>

#include "common/define.h"

#ifdef MessageBox
#undef MessageBox
#endif

namespace olive {

class QtUtils {
public:
  /**
   * @brief Retrieves the width of a string according to certain QFontMetrics
   *
   * QFontMetrics::width() has been deprecatd in favor of QFontMetrics::horizontalAdvance(), but the
   * latter was only introduced in 5.11+. This function wraps the latter for 5.11+ and the former for
   * earlier.
   */
  static int QFontMetricsWidth(QFontMetrics fm, const QString& s);

  static QFrame* CreateHorizontalLine();

  static int MessageBox(QWidget *parent, QMessageBox::Icon icon, const QString& title, const QString& message, QMessageBox::StandardButtons buttons = QMessageBox::Ok);

};

}

#endif // QTVERSIONABSTRACTION_H
