#include "grapheditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "ui/graphview.h"

GraphEditor::GraphEditor(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle("Graph Editor");

    QWidget* central_widget = new QWidget();
    setWidget(central_widget);

    QVBoxLayout* layout = new QVBoxLayout();
    central_widget->setLayout(layout);

    QHBoxLayout* tools = new QHBoxLayout();
    tools->addWidget(new QPushButton("HECK!"));
    layout->addLayout(tools);

    view = new GraphView();
    layout->addWidget(view);

    QHBoxLayout* values = new QHBoxLayout();
    values->addStretch();
    values->addWidget(new QLabel("1920.0"));
    values->addWidget(new QLabel("1080.0"));
    values->addStretch();
    layout->addLayout(values);
}
