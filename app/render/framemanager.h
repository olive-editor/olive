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

#ifndef FRAMEMANAGER_H
#define FRAMEMANAGER_H

#include <QMutex>
#include <QObject>
#include <QTimer>

namespace olive {

class FrameManager : public QObject
{
  Q_OBJECT
public:
  static void CreateInstance();

  static void DestroyInstance();

  static FrameManager* instance();

  static char* Allocate(int size);

  static void Deallocate(int size, char* buffer);

private:
  FrameManager();

  virtual ~FrameManager() override;

  /**
   * @brief Allocate buffer
   *
   * Caller takes ownership of buffer and can delete it if they want. It can also be returned to
   * the manager with Deallocate and potentially be re-used later.
   *
   * Thread-safe.
   */
  char* AllocateFromPool(int size);

  /**
   * @brief Deallocate buffer
   *
   * Manager will take ownership and buffer will stay allocated for some time in case it can be
   * re-used.
   *
   * Thread-safe.
   */
  void DeallocateToPool(int size, char* buffer);

  static FrameManager* instance_;

  static const int kFrameLifetime;

  struct Buffer
  {
    qint64 time;
    char* data;
  };

  std::map< int, std::list<Buffer> > pool_;

  QMutex mutex_;

  QTimer clear_timer_;

private slots:
  void GarbageCollection();

};

}

#endif // FRAMEMANAGER_H
