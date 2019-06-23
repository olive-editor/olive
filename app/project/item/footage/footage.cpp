#include "footage.h"

Footage::Footage()
{

}

const QString &Footage::filename()
{
  return filename_;
}

void Footage::set_filename(const QString &s)
{
  filename_ = s;
}

Item::Type Footage::type() const
{
  return kFootage;
}
