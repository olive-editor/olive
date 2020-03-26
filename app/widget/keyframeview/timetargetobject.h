#ifndef TIMETARGETOBJECT_H
#define TIMETARGETOBJECT_H

#include "node/node.h"

class TimeTargetObject
{
public:
  TimeTargetObject();

  Node* GetTimeTarget() const;
  void SetTimeTarget(Node* target);

  void SetPathIndex(int index);

  rational GetAdjustedTime(Node* from, Node* to, const rational& r, NodeParam::Type direction) const;
  TimeRange GetAdjustedTime(Node* from, Node* to, const TimeRange& r, NodeParam::Type direction) const;

  //int GetNumberOfPathAdjustments(Node* from, NodeParam::Type direction) const;

protected:
  virtual void TimeTargetChangedEvent(Node* ){}

private:
  Node* time_target_;

  int path_index_;

};

#endif // TIMETARGETOBJECT_H
