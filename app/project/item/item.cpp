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

#include "item.h"

namespace olive {

Item::Item() :
  item_parent_(nullptr),
  project_(nullptr)
{
}

Item::~Item()
{
}

const QString &Item::name() const
{
  return name_;
}

void Item::set_name(const QString &n)
{
  name_ = n;

  NameChangedEvent(n);
}

const QString &Item::tooltip() const
{
  return tooltip_;
}

void Item::set_tooltip(const QString &t)
{
  tooltip_ = t;
}

QString Item::duration()
{
  return QString();
}

QString Item::rate()
{
  return QString();
}

const Item *Item::root() const
{
  const Item* item = this;

  while (item->item_parent()) {
    item = item->item_parent();
  }

  return item;
}

Project *Item::project() const
{
  const Item* root_item = root();

  return root_item->project_;
}

void Item::set_project(Project *project)
{
  project_ = project;
}

QVector<Item *> Item::get_children_of_type(Type type, bool recursive) const
{
  QVector<Item *> list;

  foreach (Item* item, item_children_) {
    if (item->type() == type) {
      list.append(item);
    }

    if (recursive && item->CanHaveChildren()) {
      list.append(item->get_children_of_type(type, recursive));
    }
  }

  return list;
}

bool Item::CanHaveChildren() const
{
  return false;
}

bool Item::ChildExistsWithName(const QString &name)
{
  return ChildExistsWithNameInternal(name, this);
}

void Item::NameChangedEvent(const QString &)
{
}

void Item::childEvent(QChildEvent *event)
{
  QObject::childEvent(event);

  Item* cast_test = dynamic_cast<Item*>(event->child());

  if (cast_test) {
    if (event->type() == QEvent::ChildAdded) {

      item_children_.append(cast_test);
      cast_test->item_parent_ = this;

    } else if (event->type() == QEvent::ChildRemoved) {

      item_children_.removeOne(cast_test);
      cast_test->item_parent_ = nullptr;

    }
  }
}

bool Item::ChildExistsWithNameInternal(const QString &name, Item *folder)
{
  // Loop through all children
  foreach (Item* child, folder->item_children_) {
    // If this child has the same name, return true
    if (child->name() == name) {
      return true;
    } else if (child->CanHaveChildren()) {
      // If the child has children, run function recursively on this item
      if (ChildExistsWithNameInternal(name, child)) {
        // If it returns true, we've found a child so we can return now
        return true;
      }
    }
  }

  return false;
}

}
