#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace olive {

/**
 * @brief The MainWindow class
 *
 * Olive's main window responsible for docking widgets.
 */
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(QWidget* parent = nullptr);
private:
};

}

#endif
