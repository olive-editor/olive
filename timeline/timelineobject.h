#ifndef TIMELINEOBJECT_H
#define TIMELINEOBJECT_H

#include "nodes/nodegraph.h"

/**
 * @brief The TimelineObject class
 *
 * A base class for Clip, Sequence, and Track to allow compatibility between each of them and any effects nodes.
 */
class TimelineObject : public QObject
{
public:
  TimelineObject();
  virtual ~TimelineObject();

  virtual int MediaWidth();
  virtual int MediaHeight();
  virtual double MediaFrameRate();

  virtual int SequenceWidth() = 0;
  virtual int SequenceHeight() = 0;
  virtual double SequenceFrameRate() = 0;

  virtual long SequencePlayhead() = 0;

  NodeGraph* pipeline();
private:
  NodeGraph pipeline_;
};

#endif // TIMELINEOBJECT_H
