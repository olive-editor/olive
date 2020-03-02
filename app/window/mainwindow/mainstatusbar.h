#ifndef MAINSTATUSBAR_H
#define MAINSTATUSBAR_H

#include <QProgressBar>
#include <QStatusBar>

#include "task/taskmanager.h"

/**
 * @brief Shows abbreviated information from a TaskManager object
 */
class MainStatusBar : public QStatusBar
{
public:
  MainStatusBar(QWidget* parent = nullptr);

  void ConnectTaskManager(TaskManager* manager);

private slots:
  void UpdateStatus();

private:
  TaskManager* manager_;

  QProgressBar* bar_;

};

#endif // MAINSTATUSBAR_H
