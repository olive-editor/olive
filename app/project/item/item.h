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
#include <QString>

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

  void add_child(ItemPtr c);
  void remove_child(Item* c);
  int child_count();
  Item* child(int i);

  ItemPtr shared_ptr_from_raw(Item* item);

  const QString& name() const;
  void set_name(const QString& n);

  const QString& tooltip() const;
  void set_tooltip(const QString& t);

  const QIcon& icon();
  void set_icon(const QIcon& icon);

  Item *parent() const;

  virtual bool CanHaveChildren() const;

private:
  QList<ItemPtr> children_;

  Item* parent_;

  QString name_;

  QIcon icon_;

  QString tooltip_;

};

#endif // ITEM_H
