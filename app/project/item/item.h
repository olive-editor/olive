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

#ifndef ITEM_H
#define ITEM_H

#include <memory>
#include <QIcon>
#include <QList>
#include <QMutex>
#include <QString>
#include <QXmlStreamWriter>

#include "common/constructors.h"
#include "common/threadedobject.h"
#include "common/xmlutils.h"
#include "node/param.h"
#include "project/item/footage/stream.h"

OLIVE_NAMESPACE_ENTER

class Project;

class Item;
using ItemPtr = std::shared_ptr<Item>;

/**
 * @brief A base-class representing any element in a Project
 *
 * Project objects implement a parent-child hierarchy of Items that can be used throughout the Project. The Item class
 * itself is abstract and will need to be subclassed to be used in a Project.
 */
class Item
{
public:
  enum Type {
    kFolder,
    kFootage,
    kSequence
  };

  /**
   * @brief Item constructor
   */
  Item();

  /**
   * @brief Required virtual Item destructor
   */
  virtual ~Item();

  DISABLE_COPY_MOVE(Item)

  virtual void Load(QXmlStreamReader* reader, XMLNodeData &xml_node_data, const QAtomicInt *cancelled) = 0;

  virtual void Save(QXmlStreamWriter* writer) const = 0;

  virtual Type type() const = 0;

  void add_child(ItemPtr c);
  void remove_child(Item* c);
  int child_count() const;
  Item* child(int i) const;
  const QList<ItemPtr>& children() const;

  ItemPtr get_shared_ptr() const;

  const QString& name() const;
  void set_name(const QString& n);

  const QString& tooltip() const;
  void set_tooltip(const QString& t);

  virtual QIcon icon() = 0;

  virtual QString duration();

  virtual QString rate();

  Item *parent() const;
  const Item* root() const;

  Project* project() const;
  void set_project(Project* project);

  QList<ItemPtr> get_children_of_type(Type type, bool recursive) const;

  virtual bool CanHaveChildren() const;

  bool ChildExistsWithName(const QString& name);

protected:
  virtual void NameChangedEvent(const QString& name);

private:
  bool ChildExistsWithNameInternal(const QString& name, Item* folder);

  QList<ItemPtr> children_;

  Item* parent_;

  Project* project_;

  QString name_;

  QString tooltip_;

};

OLIVE_NAMESPACE_EXIT

#endif // ITEM_H
