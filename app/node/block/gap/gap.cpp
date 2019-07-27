#include "gap.h"

GapBlock::GapBlock()
{
}

rational GapBlock::length()
{
  return length_;
}

void GapBlock::set_length(const rational &length)
{
  length_ = length;

  RefreshSurrounds();
}
