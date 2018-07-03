#ifndef TRANSITION_H
#define TRANSITION_H

class Transition
{
public:
    Transition();
    int length;
    virtual void process_transition(float);
};

class CrossDissolveTransition : public Transition {
public:
    void process_transition(float);
};

#endif // TRANSITION_H
