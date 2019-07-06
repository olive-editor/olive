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

#include "item.h"

Item::Item() :
  parent_(nullptr)
{
}

Item::~Item()
{
}

void Item::add_child(ItemPtr c)
{
  if (c->parent_ == this) {
    return;
  }

  if (c->parent_ != nullptr) {
    c->parent_->remove_child(c.get());
  }

  children_.append(c);
  c->parent_ = this;
}

void Item::remove_child(Item *c)
{
  if (c->parent_ != this) {
    return;
  }

  // Remove all instances of this child in the list
  for (int i=0;i<children_.size();i++) {
    if (children_.at(i).get() == c) {
      children_.removeAt(i);
      i--;
    }
  }

  c->parent_ = nullptr;
}

int Item::child_count()
{
  return children_.size();
}

Item *Item::child(int i)
{
  return children_.at(i).get();
}

ItemPtr Item::shared_ptr_from_raw(Item *item)
{
  for (int i=0;i<children_.size();i++) {
    if (children_.at(i).get() == item) {
      return children_.at(i);
    }
  }

  return nullptr;
}

const QString &Item::name() const
{
  return name_;
}

void Item::set_name(const QString &n)
{
  name_ = n;
}

const QString &Item::tooltip() const
{
  return tooltip_;
}

void Item::set_tooltip(const QString &t)
{
  tooltip_ = t;
}

const QIcon &Item::icon()
{
  return icon_;
}

void Item::set_icon(const QIcon &icon)
{
  icon_ = icon;
}

Item *Item::parent() const
{
  return parent_;
}
