#include "track.h"

#include "timeline/clip.h"
#include "timeline/tracklist.h"
#include "timeline/sequence.h"
#include "global/math.h"

int olive::timeline::kTrackDefaultHeight = 40;
int olive::timeline::kTrackMinHeight = 30;
int olive::timeline::kTrackHeightIncrement = 10;

Track::Track(TrackList* parent, Type type) :
  parent_(parent),
  type_(type),
  muted_(false),
  soloed_(false),
  locked_(false),
  height_(olive::timeline::kTrackDefaultHeight)
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

Sequence *Track::sequence()
{
  return parent_->GetParent();
}

TrackList *Track::track_list()
{
  return parent_;
}

void Track::Save(QXmlStreamWriter &stream)
{
  stream.writeStartElement("track");

  for (int j=0;j<clips_.size();j++) {
    Clip* c = clips_.at(j).get();

    c->Save(stream);

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
  height_ = qMax(h, olive::timeline::kTrackMinHeight);
}

QString Track::name()
{
  if (name_.isEmpty()) {
    int display_index = Index() + 1;

    switch (type_) {
    case kTypeVideo:
      return tr("Video %1").arg(display_index);
    case kTypeAudio:
      return tr("Audio %1").arg(display_index);
    case kTypeSubtitle:
      return tr("Subtitle %1").arg(display_index);
    default:
      return tr("Unknown %1").arg(display_index);
    }
  }

  return name_;
}

void Track::SetName(const QString &s)
{
  name_ = s;
}

void Track::AddClip(ClipPtr clip)
{
  if (clips_.contains(clip)) {
    return;
  }

  clips_.append(clip);
  if (clip->track() != nullptr && clip->track() != this) {
    clip->track()->RemoveClip(clip.get());
  }
  clip->set_track(this);
}

int Track::ClipCount()
{
  return clips_.size();
}

ClipPtr Track::GetClip(int i)
{
  return clips_.at(i);
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

QVector<Clip*> Track::GetAllClips()
{
  QVector<Clip*> clips;

  clips.resize(clips_.size());
  for (int i=0;i<clips_.size();i++) {
    clips[i] = clips_.at(i).get();
  }

  return clips;
}

QVector<Clip *> Track::GetSelectedClips(bool containing)
{
  QVector<Clip*> selected_clips;

  for (int i=0;i<clips_.size();i++) {

    Clip* c = clips_.at(i).get();

    if (IsClipSelected(c, containing)) {
      selected_clips.append(c);
    }
  }

  return selected_clips;
}

ClipPtr Track::GetClipObjectFromRawPtr(Clip *c)
{
  for (int i=0;i<clips_.size();i++) {
    if (clips_.at(i).get() == c) {
      return clips_.at(i);
    }
  }

  // Assert here since we shouldn't be calling this function ever
  Q_ASSERT(false);
}

Clip *Track::GetClipFromPoint(long point)
{
  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i).get();

    if (c->timeline_in() <= point && c->timeline_out() > point) {
      return c;
    }
  }

  return nullptr;
}

int Track::Index()
{
  return parent_->IndexOfTrack(this);
}

bool Track::IsClipSelected(int clip_index, bool containing)
{
  return IsClipSelected(clips_.at(clip_index).get(), containing);
}

bool Track::IsClipSelected(Clip *clip, bool containing)
{
  for (int i=0;i<selections_.size();i++) {
    const Selection& s = selections_.at(i);
    if (((clip->timeline_in() >= s.in() && clip->timeline_out() <= s.out())
         || (!containing && !(clip->timeline_in() < s.in() && clip->timeline_out() < s.in())
             && !(clip->timeline_in() > s.in() && clip->timeline_out() > s.in())))) {
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
    if (s.in() <= transition_in_point
        && s.out() >= transition_out_point) {
      return true;
    }
  }

  return false;
}

void Track::SelectArea(long in, long out)
{
  selections_.append(Selection(in, out, this));
}

void Track::SelectClip(Clip* c)
{
  selections_.append(Selection(c->timeline_in(), c->timeline_out(), this));
}

void Track::SelectAll()
{
  ClearSelections();

  // Select every clip
  for (int i=0;i<clips_.size();i++) {
    SelectClip(clips_.at(i).get());
  }
}

void Track::SelectAtPoint(long point)
{
  ClearSelections();

  // Select every clip that this point is within
  for (int i=0;i<clips_.size();i++) {

    Clip* c = clips_.at(i).get();

    if (c->timeline_in() <= point
        && c->timeline_out() > point) {
      SelectClip(c);
    }
  }
}

void Track::ClearSelections()
{
  selections_.clear();
}

void Track::DeselectArea(long in, long out)
{
  int selection_count = selections_.size();
    for (int i=0;i<selection_count;i++) {
      Selection& s = selections_[i];

      if (s.in() >= in && s.out() <= out) {
        // whole selection is in deselect area
        selections_.removeAt(i);
        i--;
        selection_count--;
      } else if (s.in() < in && s.out() > out) {
        // middle of selection is in deselect area
        Selection new_sel(out, s.out(), s.track());
        selections_.append(new_sel);

        s.set_out(in);
      } else if (s.in() < in && s.out() > in) {
        // only out point is in deselect area
        s.set_out(in);
      } else if (s.in() < out && s.out() > out) {
        // only in point is in deselect area
        s.set_in(out);
      }
  }
}

QVector<Selection> Track::Selections()
{
  return selections_;
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

bool Track::IsMuted()
{
  return muted_;
}

void Track::SetMuted(bool muted)
{
  muted_ = muted;
}

bool Track::IsSoloed()
{
  return soloed_;
}

void Track::SetSoloed(bool soloed)
{
  soloed_ = soloed;
}

bool Track::IsLocked()
{
  return locked_;
}

void Track::SetLocked(bool locked)
{
  locked_ = locked;
}
