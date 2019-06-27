
/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef FOOTAGE_H
#define FOOTAGE_H

#include <QList>
#include <QDateTime>

#include "project/item/item.h"
#include "project/item/footage/audiostream.h"
#include "project/item/footage/videostream.h"

class Footage : public Item
{
public:
  Footage();

  virtual ~Footage() override;

  /**
   * @brief Deleted copy constructor
   */
  Footage(const Footage& other) = delete;

  /**
   * @brief Deleted move constructor
   */
  Footage(Footage&& other) = delete;

  /**
   * @brief Deleted copy assignment
   */
  Footage& operator=(const Footage& other) = delete;

  /**
   * @brief Deleted move assignment
   */
  Footage& operator=(Footage&& other) = delete;

  void Clear();

  const QString& filename();
  void set_filename(const QString& s);

  const QDateTime& timestamp();
  void set_timestamp(const QDateTime& t);

  void add_stream(Stream* s);
  const Stream* stream(int index);

  virtual Type type() const override;

private:
  void ClearStreams();

  QString filename_;

  QDateTime timestamp_;

  QList<Stream*> streams_;
};

#endif // FOOTAGE_H
