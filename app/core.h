#ifndef CORE_H
#define CORE_H

#include "window/mainwindow/mainwindow.h"

class Core : public QObject
{
  Q_OBJECT
public:
  Core();

  void Start();
  void StartGUI();
  void Stop();

  olive::MainWindow* main_window();
private:
  olive::MainWindow* main_window_;

  QString startup_project_;
};

namespace olive {
extern Core core;
}

#endif // CORE_H
