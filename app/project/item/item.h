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
