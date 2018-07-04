#ifndef TRANSITION_H
#define TRANSITION_H

#include <QString>

class Transition
{
public:
    Transition();
    int length;
    QString name;
    virtual void process_transition(float);
    virtual Transition* copy();
};

class CrossDissolveTransition : public Transition {
public:
    CrossDissolveTransition();
    void process_transition(float);
    Transition* copy();
};

#endif // TRANSITION_H
