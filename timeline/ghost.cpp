#include "ghost.h"

Ghost::Ghost() :
  transition(nullptr),
  track_movement(0),
  clip(nullptr)
{
}

Selection Ghost::ToSelection() const
{
  return Selection(in, out, track->Sibling(track_movement));
}
