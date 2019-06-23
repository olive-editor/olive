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

#include <QString>
#include <QList>

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
   * @brief Item destructor, deletes all children
   */
  virtual ~Item();

  /**
   * @brief Deleted copy constructor
   */
  Item(const Item& other) = delete;

  /**
   * @brief Deleted move constructor
   */
  Item(Item&& other) = delete;

  /**
   * @brief Deleted copy assignment
   */
  Item& operator=(const Item& other) = delete;

  /**
   * @brief Deleted move assignment
   */
  Item& operator=(Item&& other) = delete;

  virtual Type type() const = 0;

  void add_child(Item *c);
  void remove_child(Item* c);
  int child_count();
  Item* child(int i);

  const QString& name() const;
  void set_name(const QString& n);

  Item *parent() const;
  void set_parent(Item *p);

private:
  QList<Item*> children_;

  Item* parent_;

  QString name_;
};

#endif // ITEM_H
