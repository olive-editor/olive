#include "selection.h"

#include "effects/transition.h"
#include "timeline/clip.h"

Selection::Selection()
{
}

Selection::Selection(long in, long out, Track *track) :
  in_(in),
  out_(out),
  track_(track)
{
}

long Selection::in() const
{
  return in_;
}

long Selection::out() const
{
  return out_;
}

Track *Selection::track() const
{
  return track_;
}

void Selection::set_in(long in)
{
  in_ = in;
}

void Selection::set_out(long out)
{
  out_ = out;
}

bool Selection::ContainsTransition(Clip* c, int type) const
{
  if (type == kTransitionOpening) {
      return c->opening_transition != nullptr
          && out_ == c->timeline_in() + c->opening_transition->get_true_length()
          && ((c->opening_transition->secondary_clip == nullptr && in_ == c->timeline_in())
              || (c->opening_transition->secondary_clip != nullptr && in_ == c->timeline_in() - c->opening_transition->get_true_length()));
    } else {
      return c->closing_transition != nullptr
          && in_ == c->timeline_out() - c->closing_transition->get_true_length()
          && ((c->closing_transition->secondary_clip == nullptr && out_ == c->timeline_out())
              || (c->closing_transition->secondary_clip != nullptr && out_ == c->timeline_out() + c->closing_transition->get_true_length()));
  }
}

void Selection::Tidy(QVector<Selection>& selections)
{
  for (int i=0;i<selections.size();i++) {
      Selection& s = selections[i];
      for (int j=0;j<selections.size();j++) {
        if (i != j) {
          Selection& ss = selections[j];
          if (s.track() == ss.track()) {
            bool remove = false;
            if (s.in() < ss.in() && s.out() > ss.out()) {
              // do nothing
            } else if (s.in() >= ss.in() && s.out() <= ss.out()) {
              remove = true;
            } else if (s.in() <= ss.out() && s.out() > ss.out()) {
              ss.set_out(s.out());
              remove = true;
            } else if (s.out() >= ss.in() && s.in() < ss.in()) {
              ss.set_in(s.in());
              remove = true;
            }
            if (remove) {
              selections.removeAt(i);
              i--;
              break;
            }
          }
        }
      }
  }
}
