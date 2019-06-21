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

  Item();

  // Required virtual destructor (it's empty)
  virtual ~Item();

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
