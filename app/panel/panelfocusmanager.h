#ifndef PANELFOCUSMANAGER_H
#define PANELFOCUSMANAGER_H

#include <QObject>
#include <QList>

#include "widget/panel/panel.h"

class PanelFocusManager : public QObject
{
  Q_OBJECT
public:
  PanelFocusManager(QObject* parent);

  PanelWidget* CurrentlyFocused();

public slots:
  void FocusChanged(QWidget* old, QWidget* now);

private:
  QList<PanelWidget*> focus_history_;
};

namespace olive {
extern PanelFocusManager* panel_focus_manager;
}

#endif // PANELFOCUSMANAGER_H
