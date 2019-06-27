#ifndef STREAM_H
#define STREAM_H

#include "rational.h"

class Footage;

class Stream
{
public:
  enum Type {
    kUnknown,
    kVideo,
    kAudio,
    kData,
    kSubtitle,
    kAttachment
  };

  /**
   * @brief Stream constructor
   */
  Stream();

  /**
   * @brief Required virtual destructor, serves no purpose
   */
  virtual ~Stream();

  const Type& type();
  void set_type(const Type& type);

  Footage* footage();
  void set_footage(Footage* f);

  const rational& timebase();
  void set_timebase(const rational& timebase);

  const int& index();
  void set_index(const int& index);

private:
  Footage* footage_;

  rational timebase_;

  int index_;

  Type type_;

};

#endif // STREAM_H
