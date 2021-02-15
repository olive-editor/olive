/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef ITEM_H
#define ITEM_H

#include <memory>
#include <QIcon>
#include <QList>
#include <QMutex>
#include <QString>
#include <QXmlStreamWriter>

#include "common/threadedobject.h"
#include "common/xmlutils.h"
#include "node/node.h"

namespace olive {

class Folder;
class Project;

/**
 * @brief A base-class representing any element in a Project
 *
 * Project objects implement a parent-child hierarchy of Items that can be used throughout the Project. The Item class
 * itself is abstract and will need to be subclassed to be used in a Project.
 */
class Item : public Node
{
  Q_OBJECT
public:
  /**
   * @brief Item constructor
   */
  Item(bool create_folder_input = true, bool create_default_output = true);

  const QString& tooltip() const;
  void set_tooltip(const QString& t);

  virtual QString duration();

  virtual QString rate();

  Folder *item_parent() const;

  static const QString kParentInput;

  virtual void Retranslate() override;

private:
  QString tooltip_;

};

}

#endif // ITEM_H
