#ifndef _OLIVE_CLIP_H_
#define _OLIVE_CLIP_H_

#include <QObject>

namespace olive {

class Clip : public QObject {
  Q_OBJECT

private:
  int id_;
  int clip_in_;
  int in_;
  int out_;

signals:
  void onDidChangeTime(int oldIn, int oldOut);

public:
  Clip(int in, int out);

  void setTime(int in, int out);

  int in() const;
  int out() const;
  int id() const;

  bool operator<(const Clip& rhs) const;

};

}

#endif