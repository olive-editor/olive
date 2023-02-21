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

#ifndef QTVERSIONABSTRACTION_H
#define QTVERSIONABSTRACTION_H

#include <olive/core/core.h>
#include <QComboBox>
#include <QDateTime>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QMessageBox>

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

  static QFrame* CreateVerticalLine();

  static int MsgBox(QWidget *parent, QMessageBox::Icon icon, const QString& title, const QString& message, QMessageBox::StandardButtons buttons = QMessageBox::Ok);

  static QDateTime GetCreationDate(const QFileInfo &info);

  static QString GetFormattedDateTime(const QDateTime &dt);

  static QStringList WordWrapString(const QString &s, const QFontMetrics &fm, int bounding_width);

  static Qt::KeyboardModifiers FlipControlAndShiftModifiers(Qt::KeyboardModifiers e);

  static void SetComboBoxData(QComboBox *cb, int data);

  template <typename T>
  static T *GetParentOfType(const QObject *child)
  {
    QObject *t = child->parent();

    while (t) {
      if (T *p = dynamic_cast<T*>(t)) {
        return p;
      }
      t = t->parent();
    }

    return nullptr;
  }

  static QColor toQColor(const core::Color &c);

  /**
   * @brief Convert a pointer to a value that can be sent between NodeParams
   */
  static QVariant PtrToValue(void* ptr)
  {
    return reinterpret_cast<quintptr>(ptr);
  }

  /**
   * @brief Convert a NodeParam value to a pointer of any kind
   */
  template<class T>
  static T* ValueToPtr(const QVariant &ptr)
  {
    return reinterpret_cast<T*>(ptr.value<quintptr>());
  }

};

namespace core {

uint qHash(const core::rational& r, uint seed = 0);
uint qHash(const core::TimeRange& r, uint seed = 0);

}

}

Q_DECLARE_METATYPE(olive::core::rational);
Q_DECLARE_METATYPE(olive::core::Color);
Q_DECLARE_METATYPE(olive::core::TimeRange);
Q_DECLARE_METATYPE(olive::core::Bezier);
Q_DECLARE_METATYPE(olive::core::AudioParams);
Q_DECLARE_METATYPE(olive::core::SampleBuffer);

#endif // QTVERSIONABSTRACTION_H
