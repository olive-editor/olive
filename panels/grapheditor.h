#ifndef GRAPHEDITOR_H
#define GRAPHEDITOR_H

#include <QDockWidget>

class GraphView;
class TimelineHeader;
class QPushButton;
class EffectRow;
class QHBoxLayout;
class LabelSlider;
class QLabel;
class KeyframeNavigator;

class GraphEditor : public QDockWidget {
    Q_OBJECT
public:
    GraphEditor(QWidget* parent = 0);
	void update_panel();
	void set_row(EffectRow* r);
private:
	GraphView* view;
	TimelineHeader* header;
	QHBoxLayout* value_layout;
	QVector<LabelSlider*> slider_proxies;
	QVector<LabelSlider*> slider_proxy_sources;
	QLabel* current_row_desc;
	EffectRow* row;
	KeyframeNavigator* keyframe_nav;
private slots:
	void passthrough_slider_value();
};

#endif // GRAPHEDITOR_H
