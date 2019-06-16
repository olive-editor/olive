#include "timing.h"

#include "timeline/sequence.h"
#include "timeline/clip.h"
#include "global/config.h"

double get_timecode(Clip* c, long playhead) {
  return double(playhead_to_clip_frame(c, playhead))/c->track()->sequence()->frame_rate();
}

long rescale_frame_number(long framenumber, double source_frame_rate, double target_frame_rate) {
  return qRound((double(framenumber)/source_frame_rate)*target_frame_rate);
}

long playhead_to_clip_frame(Clip* c, long playhead) {
  return (qMax(0L, playhead - c->timeline_in(true)) + c->clip_in(true));
}

double playhead_to_clip_seconds(Clip* c, long playhead) {
  // returns time in seconds
  long clip_frame = playhead_to_clip_frame(c, playhead);

  if (c->reversed()) {
    clip_frame = c->media_length() - clip_frame - 1;
  }

  double secs = (double(clip_frame)/c->track()->sequence()->frame_rate())*c->speed().value;
  if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
    secs *= c->media()->to_footage()->speed;
  }

  return secs;
}

int64_t seconds_to_timestamp(Clip *c, double seconds) {
  return qRound64(seconds * av_q2d(av_inv_q(c->time_base())));
}

int64_t playhead_to_timestamp(Clip* c, long playhead) {
  return seconds_to_timestamp(c, playhead_to_clip_seconds(c, playhead));
}

bool frame_rate_is_droppable(double rate) {
  return (qFuzzyCompare(rate, 23.976)
          || qFuzzyCompare(rate, 29.97)
          || qFuzzyCompare(rate, 59.94));
}

long SMPTEToTime(const QString& s, int view, double frame_rate) {
  QList<QString> list = s.split(QRegExp("[:;]"));

  if (view == olive::kTimecodeFrames || (list.size() == 1 && view != olive::kTimecodeMilliseconds)) {
    return s.toLong();
  }

  int frRound = qRound(frame_rate);
  int hours, minutes, seconds, frames;

  if (view == olive::kTimecodeMilliseconds) {
    long milliseconds = s.toLong();

    hours = milliseconds/3600000;
    milliseconds -= (hours*3600000);
    minutes = milliseconds/60000;
    milliseconds -= (minutes*60000);
    seconds = milliseconds/1000;
    milliseconds -= (seconds*1000);
    frames = qRound64((milliseconds*0.001)*frame_rate);

    seconds = qRound64(seconds * frame_rate);
    minutes = qRound64(minutes * frame_rate * 60);
    hours = qRound64(hours * frame_rate * 3600);
  } else {
    hours = ((list.size() > 0) ? list.at(0).toInt() : 0) * frRound * 3600;
    minutes = ((list.size() > 1) ? list.at(1).toInt() : 0) * frRound * 60;
    seconds = ((list.size() > 2) ? list.at(2).toInt() : 0) * frRound;
    frames = (list.size() > 3) ? list.at(3).toInt() : 0;
  }

  int f = (frames + seconds + minutes + hours);

  if ((view == olive::kTimecodeDrop || view == olive::kTimecodeMilliseconds) && frame_rate_is_droppable(frame_rate)) {
    // return drop
    int d;
    int m;

    int dropFrames = qRound(frame_rate * .066666); //Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
    int framesPer10Minutes = qRound(frame_rate * 60 * 10); //Number of frames per ten minutes
    int framesPerMinute = (qRound(frame_rate)*60)-  dropFrames; //Number of frames per minute is the round of the framerate * 60 minus the number of dropped frames

    d = f / framesPer10Minutes;
    f -= dropFrames*9*d;

    m = f % framesPer10Minutes;

    if (m > dropFrames) {
      f -= (dropFrames * ((m - dropFrames) / framesPerMinute));
    }
  }

  // return non-drop
  return f;
}

QString TimeToSMPTE(const rational& time, int view, double frame_rate) {
  long f = qFloor(time.ToDouble()*frame_rate); // FIXME: Not sure if this should be Floor or Round

  if (view == olive::kTimecodeFrames) {
    return QString::number(f);
  }

  // return timecode
  int hours = 0;
  int mins = 0;
  int secs = 0;
  int frames = 0;
  QString token = ":";

  if ((view == olive::kTimecodeDrop || view == olive::kTimecodeMilliseconds) && frame_rate_is_droppable(frame_rate)) {
    //CONVERT A FRAME NUMBER TO DROP FRAME TIMECODE
    //Code by David Heidelberger, adapted from Andrew Duncan, further adapted for Olive by Olive Team
    //Given an int called framenumber and a double called framerate
    //Framerate should be 29.97, 59.94, or 23.976, otherwise the calculations will be off.

    int d;
    int m;

    int dropFrames = qRound(frame_rate * .066666); //Number of frames to drop on the minute marks is the nearest integer to 6% of the framerate
    int framesPerHour = qRound(frame_rate*60*60); //Number of frqRound64ames in an hour
    int framesPer24Hours = framesPerHour*24; //Number of frames in a day - timecode rolls over after 24 hours
    int framesPer10Minutes = qRound(frame_rate * 60 * 10); //Number of frames per ten minutes
    int framesPerMinute = (qRound(frame_rate)*60)-  dropFrames; //Number of frames per minute is the round of the framerate * 60 minus the number of dropped frames

    //If framenumber is greater than 24 hrs, next operation will rollover clock
    f = f % framesPer24Hours; // % is the modulus operator, which returns a remainder. a % b = the remainder of a/b

    d = f / framesPer10Minutes; // \ means integer division, which is a/b without a remainder. Some languages you could use floor(a/b)
    m = f % framesPer10Minutes;

    //In the original post, the next line read m>1, which only worked for 29.97. Jean-Baptiste Mardelle correctly pointed out that m should be compared to dropFrames.
    if (m > dropFrames) {
      f = f + (dropFrames*9*d) + dropFrames * ((m - dropFrames) / framesPerMinute);
    } else {
      f = f + dropFrames*9*d;
    }

    int frRound = qRound(frame_rate);
    frames = f % frRound;
    secs = (f / frRound) % 60;
    mins = ((f / frRound) / 60) % 60;
    hours = (((f / frRound) / 60) / 60);

    token = ";";
  } else {
    // non-drop timecode

    int int_fps = qRound(frame_rate);
    hours = f / (3600 * int_fps);
    mins = f / (60*int_fps) % 60;
    secs = f/int_fps % 60;
    frames = f%int_fps;
  }
  if (view == olive::kTimecodeMilliseconds) {
    return QString::number((hours*3600000)+(mins*60000)+(secs*1000)+qCeil(frames*1000/frame_rate));
  }
  return QString(QString::number(hours).rightJustified(2, '0') +
                 ":" + QString::number(mins).rightJustified(2, '0') +
                 ":" + QString::number(secs).rightJustified(2, '0') +
                 token + QString::number(frames).rightJustified(2, '0')
                 );
}
