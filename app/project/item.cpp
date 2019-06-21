#include "item.h"

Item::Item() :
  parent_(nullptr)
{
}

Item::~Item()
{
}

void Item::add_child(Item* c)
{
  if (c->parent_ == this) {
    return;
  }

  children_.append(c);
  c->parent_ = this;
}

void Item::remove_child(Item *c)
{
  if (c->parent_ != this) {
    return;
  }

  children_.removeAll(c);
  c->parent_ = nullptr;
}

int Item::child_count()
{
  return children_.size();
}

Item *Item::child(int i)
{
  return children_.at(i);
}

const QString &Item::name() const
{
  return name_;
}

void Item::set_name(const QString &n)
{
  name_ = n;
}

Item *Item::parent() const
{
  return parent_;
}

void Item::set_parent(Item *p)
{
  if (parent_ != nullptr) {
    parent_->remove_child(this);
  }

  p->add_child(this);
}
