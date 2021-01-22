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
#include "project/item/footage/stream.h"

namespace olive {

class Project;

/**
 * @brief A base-class representing any element in a Project
 *
 * Project objects implement a parent-child hierarchy of Items that can be used throughout the Project. The Item class
 * itself is abstract and will need to be subclassed to be used in a Project.
 */
class Item : public QObject
{
  Q_OBJECT
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
  virtual ~Item() override;

  virtual void Load(QXmlStreamReader* reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt *cancelled) = 0;

  virtual void Save(QXmlStreamWriter* writer) const = 0;

  virtual Type type() const = 0;

  int item_child_count() const
  {
    return item_children_.size();
  }

  Item* item_child(int i) const
  {
    return item_children_.at(i);
  }

  const QVector<Item*>& children() const
  {
    return item_children_;
  }

  const QString& name() const;
  void set_name(const QString& n);

  const QString& tooltip() const;
  void set_tooltip(const QString& t);

  virtual QIcon icon() = 0;

  virtual QString duration();

  virtual QString rate();

  Item *item_parent() const
  {
    return item_parent_;
  }

  Project* project() const;

  void set_project(Project* project);

  QVector<Item*> get_children_of_type(Type type, bool recursive) const;

  virtual bool CanHaveChildren() const;

  bool ChildExistsWithName(const QString& name);

protected:
  virtual void NameChangedEvent(const QString& name);

  virtual void childEvent(QChildEvent *event) override;

private:
  static bool ChildExistsWithNameInternal(const QString& name, Item* folder);

  QVector<Item*> item_children_;

  Item* item_parent_;

  Project* project_;

  QString name_;

  QString tooltip_;

};

}

#endif // ITEM_H
