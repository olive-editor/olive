#ifndef FOOTAGE_H
#define FOOTAGE_H

#include "project/item/item.h"
#include "project/item/footage/audiostream.h"
#include "project/item/footage/videostream.h"

#include <QList>

class Footage : public Item
{
public:
  Footage();

  const QString& filename();
  void set_filename(const QString& s);

  virtual Type type() const override;

private:
  QString filename_;

  QList<VideoStream> video_streams_;
  QList<AudioStream> audio_streams_;
};

#endif // FOOTAGE_H
