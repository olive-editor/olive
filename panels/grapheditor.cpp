#include "grapheditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "ui/graphview.h"
#include "ui/keyframenavigator.h"

GraphEditor::GraphEditor(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle("Graph Editor");
    resize(720, 480);

    QWidget* central_widget = new QWidget();
    setWidget(central_widget);

    QVBoxLayout* layout = new QVBoxLayout();
    central_widget->setLayout(layout);

    QWidget* tool_widget = new QWidget();
    tool_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QHBoxLayout* tools = new QHBoxLayout();
    tool_widget->setLayout(tools);

    KeyframeNavigator* keyframe_nav = new KeyframeNavigator();
    tools->addWidget(keyframe_nav);
    tools->addStretch();

    tools->addWidget(new QPushButton("HECK!"));
    tools->addStretch();

    tools->addWidget(new QPushButton("Hand"));

    layout->addWidget(tool_widget);

    view = new GraphView();
    layout->addWidget(view);

    QWidget* value_widget = new QWidget();
    value_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QHBoxLayout* values = new QHBoxLayout();
    value_widget->setLayout(values);
    values->addStretch();
    values->addWidget(new QLabel("1920.0"));
    values->addWidget(new QLabel("1080.0"));
    values->addStretch();
    layout->addWidget(value_widget);
}
