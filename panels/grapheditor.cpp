#include "grapheditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVariant>

#include "ui/keyframenavigator.h"
#include "ui/timelineheader.h"
#include "ui/timelinetools.h"
#include "ui/labelslider.h"
#include "ui/graphview.h"
#include "project/effect.h"
#include "project/effectfield.h"
#include "project/effectrow.h"
#include "project/clip.h"
#include "panels.h"
#include "debug.h"

GraphEditor::GraphEditor(QWidget* parent) : QDockWidget(parent), row(NULL) {
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	setWindowTitle("Graph Editor");
	resize(720, 480);

	QWidget* main_widget = new QWidget();
	setWidget(main_widget);
	QVBoxLayout* layout = new QVBoxLayout();
	main_widget->setLayout(layout);

	QWidget* tool_widget = new QWidget();
	tool_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	QHBoxLayout* tools = new QHBoxLayout();
	tool_widget->setLayout(tools);

	QWidget* left_tool_widget = new QWidget();
	QHBoxLayout* left_tool_layout = new QHBoxLayout();
	left_tool_layout->setSpacing(0);
	left_tool_layout->setMargin(0);
	left_tool_widget->setLayout(left_tool_layout);
	tools->addWidget(left_tool_widget);
	QWidget* center_tool_widget = new QWidget();
	QHBoxLayout* center_tool_layout = new QHBoxLayout();
	center_tool_layout->setSpacing(0);
	center_tool_layout->setMargin(0);
	center_tool_widget->setLayout(center_tool_layout);
	tools->addWidget(center_tool_widget);
	QWidget* right_tool_widget = new QWidget();
	QHBoxLayout* right_tool_layout = new QHBoxLayout();
	right_tool_layout->setSpacing(0);
	right_tool_layout->setMargin(0);
	right_tool_widget->setLayout(right_tool_layout);
	tools->addWidget(right_tool_widget);

	keyframe_nav = new KeyframeNavigator();
	keyframe_nav->enable_keyframes(true);
	keyframe_nav->enable_keyframe_toggle(false);
	left_tool_layout->addWidget(keyframe_nav);
	left_tool_layout->addStretch();

	linear_button = new QPushButton("Linear");
	linear_button->setCheckable(true);
	bezier_button = new QPushButton("Bezier");
	bezier_button->setCheckable(true);
	hold_button = new QPushButton("Hold");
	hold_button->setCheckable(true);

	center_tool_layout->addStretch();
	center_tool_layout->addWidget(linear_button);
	center_tool_layout->addWidget(bezier_button);
	center_tool_layout->addWidget(hold_button);

	layout->addWidget(tool_widget);

	QWidget* central_widget = new QWidget();
	QVBoxLayout* central_layout = new QVBoxLayout();
	central_widget->setLayout(central_layout);
	central_layout->setSpacing(0);
	central_layout->setMargin(0);
	header = new TimelineHeader();
	header->viewer = panel_sequence_viewer;
	central_layout->addWidget(header);
	view = new GraphView();
	central_layout->addWidget(view);

	layout->addWidget(central_widget);

	QWidget* value_widget = new QWidget();
	value_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	QHBoxLayout* values = new QHBoxLayout();
	value_widget->setLayout(values);
	values->addStretch();

	current_row_desc = new QLabel();
	values->addWidget(current_row_desc);

	QWidget* central_value_widget = new QWidget();
	value_layout = new QHBoxLayout();
	value_layout->setMargin(0);
	central_value_widget->setLayout(value_layout);
	values->addWidget(central_value_widget);

	values->addStretch();
	layout->addWidget(value_widget);

	connect(view, SIGNAL(zoom_changed(double)), header, SLOT(update_zoom(double)));
	connect(view, SIGNAL(x_scroll_changed(int)), header, SLOT(set_scroll(int)));
	connect(view, SIGNAL(selection_changed(bool)), this, SLOT(set_key_button_enabled(bool)));
}

void GraphEditor::update_panel() {
	if (isVisible()) {
		if (row != NULL) {
			int slider_index = 0;
			for (int i=0;i<row->fieldCount();i++) {
				EffectField* field = row->field(i);
				if (field->type == EFFECT_FIELD_DOUBLE) {
					slider_proxies.at(slider_index)->set_value(row->field(i)->get_current_data().toDouble(), false);
					slider_index++;
				}
			}
		}

		header->update();
		view->update();
	}
}

void GraphEditor::set_row(EffectRow *r) {
	for (int i=0;i<slider_proxies.size();i++) {
		delete slider_proxies.at(i);
	}
	slider_proxies.clear();
	slider_proxy_sources.clear();

	if (row != NULL) {
		// clear old row connections
		disconnect(keyframe_nav, SIGNAL(goto_previous_key()), row, SLOT(goto_previous_key()));
		disconnect(keyframe_nav, SIGNAL(toggle_key()), row, SLOT(toggle_key()));
		disconnect(keyframe_nav, SIGNAL(goto_next_key()), row, SLOT(goto_next_key()));
	}

	bool found_vals = false;

	if (r != NULL && r->isKeyframing()) {
		for (int i=0;i<r->fieldCount();i++) {
			EffectField* field = r->field(i);
			if (field->type == EFFECT_FIELD_DOUBLE) {
				LabelSlider* slider = new LabelSlider();
				slider->set_color(get_curve_color(i, r->fieldCount()).name());
				connect(slider, SIGNAL(valueChanged()), this, SLOT(passthrough_slider_value()));

				slider_proxies.append(slider);
				value_layout->addWidget(slider);

				slider_proxy_sources.append(static_cast<LabelSlider*>(field->ui_element));

				found_vals = true;
			}
		}
	}

	if (found_vals) {
		row = r;
		current_row_desc->setText(row->parent_effect->parent_clip->name + " :: " + row->parent_effect->meta->name + " :: " + row->get_name());

		connect(keyframe_nav, SIGNAL(goto_previous_key()), row, SLOT(goto_previous_key()));
		connect(keyframe_nav, SIGNAL(toggle_key()), row, SLOT(toggle_key()));
		connect(keyframe_nav, SIGNAL(goto_next_key()), row, SLOT(goto_next_key()));
	} else {
		row = NULL;
		current_row_desc->setText(0);
	}
	view->set_row(row);
	update_panel();
}

void GraphEditor::set_key_button_enabled(bool e) {
	linear_button->setEnabled(e);
	bezier_button->setEnabled(e);
	hold_button->setEnabled(e);
}

void GraphEditor::passthrough_slider_value() {
	for (int i=0;i<slider_proxies.size();i++) {
		if (slider_proxies.at(i) == sender()) {
			slider_proxy_sources.at(i)->set_value(slider_proxies.at(i)->value(), true);
		}
	}
}
