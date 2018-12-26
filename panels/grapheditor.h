#ifndef GRAPHEDITOR_H
#define GRAPHEDITOR_H

#include <QDockWidget>

class GraphView;

class GraphEditor : public QDockWidget {
    Q_OBJECT
public:
    GraphEditor(QWidget* parent = 0);
private:
    GraphView* view;
};

#endif // GRAPHEDITOR_H
