/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef EFFECTLOADERS_H
#define EFFECTLOADERS_H

#include <QList>
#include <QThread>
#include <QMutex>

namespace olive {
  extern QMutex effects_loaded;
}

/**
 * @brief The EffectInit class
 *
 * A separate thread for loading effects in the background while the rest of the program's initiation takes place.
 * The program can even run before the effects have finished loading, but any point the software needs to access
 * effects, it will have to wait for this thread to finish before it can. Fortunately this thread is usually very
 * quick and is over before the MainWindow shows.
 */
class EffectInit : public QThread {
public:
  EffectInit();

  /**
   * @brief A static convenience function to set up the EffectInit thread, start it, and free itself when complete.
   */
  static void StartLoading();
protected:
  /**
   * @brief Function that runs in the other thread.
   */
  void run();
};

#endif // EFFECTLOADERS_H
