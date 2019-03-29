#include "track.h"

#include "timeline/clip.h"

int olive::timeline::kTrackDefaultHeight = 40;
int olive::timeline::kTrackMinHeight = 30;
int olive::timeline::kTrackHeightIncrement = 10;

Track::Track(TrackList* parent, Type type) :
  parent_(parent),
  type_(type)
{
}

Track *Track::copy(TrackList *parent)
{
  Track* t = new Track(parent, type_);

  t->ResizeClipArray(ClipCount());
  for (int i=0;i<clips_.size();i++) {
    ClipPtr c = clips_.at(i);
    ClipPtr copy = c->copy(t);
    copy->linked = c->linked;
    t->clips_[i] = copy;
  }

  return t;
}

void Track::Save(QXmlStreamWriter &stream)
{
  stream.writeStartElement("track");

  for (int j=0;j<clips_.size();j++) {
    Clip* c = clips_.at(j).get();
    if (c != nullptr) {
      stream.writeStartElement("clip");
      stream.writeAttribute("id", QString::number(j));

      c->Save(stream);

      stream.writeEndElement(); // clip
    }
  }

  stream.writeEndElement(); // track
}

Track::Type Track::type()
{
  return type_;
}

int Track::height()
{
  return height_;
}

void Track::set_height(int h)
{
  height_ = h;
}

void Track::AddClip(ClipPtr clip)
{
  clips_.append(clip);
  if (clip->track() != nullptr) {
    clip->track()->RemoveClip(clip.get());
  }
  clip->set_track(this);
}

void Track::RemoveClip(int i)
{
  clips_.removeAt(i);
}

void Track::RemoveClip(Clip *c)
{
  for (int i=0;i<clips_.size();i++) {
    if (clips_.at(i).get() == c) {
      clips_.removeAt(i);
      return;
    }
  }
}

void Track::ResizeClipArray(int new_size)
{
  clips_.resize(new_size);
}

QVector<ClipPtr> Track::GetAllClips()
{
  return clips_;
}

bool Track::IsClipSelected(int clip_index, bool containing)
{
  return IsClipSelected(clips_.at(clip_index).get(), containing);
}

bool Track::IsClipSelected(Clip *clip, bool containing)
{
  for (int i=0;i<selections_.size();i++) {
    const Selection& s = selections_.at(i);
    if (((clip->timeline_in() >= s.in && clip->timeline_out() <= s.out)
         || (!containing && !(clip->timeline_in() < s.in && clip->timeline_out() < s.in)
             && !(clip->timeline_in() > s.in && clip->timeline_out() > s.in)))) {
      return true;
    }
  }
  return false;
}

bool Track::IsTransitionSelected(Transition *t)
{
  if (t == nullptr) {
    return false;
  }

  Clip* c = t->parent_clip;

  long transition_in_point;
  long transition_out_point;

  // Get positions of the transition on the timeline

  if (t == c->opening_transition.get()) {
    transition_in_point = c->timeline_in();
    transition_out_point = c->timeline_in() + t->get_true_length();

    if (t->secondary_clip != nullptr) {
      transition_in_point -= t->get_true_length();
    }
  } else {
    transition_in_point = c->timeline_out() - t->get_true_length();
    transition_out_point = c->timeline_out();

    if (t->secondary_clip != nullptr) {
      transition_out_point += t->get_true_length();
    }
  }

  // See if there's a selection matching this
  for (int i=0;i<selections_.size();i++) {
    const Selection& s = selections_.at(i);
    if (s.in <= transition_in_point
        && s.out >= transition_out_point) {
      return true;
    }
  }

  return false;
}

void Track::ClearSelections()
{
  selections_.clear();
}

long Track::GetEndFrame()
{
  long end_frame = 0;

  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i).get();
    if (c != nullptr) {
      end_frame = qMax(end_frame, c->timeline_out(true));
    }
  }

  return end_frame;
}
