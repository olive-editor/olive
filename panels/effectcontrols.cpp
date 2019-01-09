#include "effectcontrols.h"

#include <QMenu>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QScrollArea>
#include <QPushButton>
#include <QSpacerItem>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>

#include "panels/panels.h"
#include "project/effect.h"
#include "project/clip.h"
#include "project/transition.h"
#include "ui/collapsiblewidget.h"
#include "project/sequence.h"
#include "project/undo.h"
#include "panels/project.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/grapheditor.h"
#include "ui/viewerwidget.h"
#include "io/clipboard.h"
#include "ui/timelineheader.h"
#include "ui/keyframeview.h"
#include "ui/resizablescrollbar.h"
#include "debug.h"

EffectControls::EffectControls(QWidget *parent) :
	QDockWidget(parent),
	multiple(false),
	zoom(1),
	panel_name("Effects: "),
	mode(TA_NO_TRANSITION)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	setup_ui();

	clear_effects(false);
	headers->viewer = panel_sequence_viewer;
	headers->snapping = false;

	effects_area->parent_widget = scrollArea;
	effects_area->keyframe_area = keyframeView;
	effects_area->header = headers;
	keyframeView->header = headers;

	lblMultipleClipsSelected->setVisible(false);

	connect(horizontalScrollBar, SIGNAL(valueChanged(int)), headers, SLOT(set_scroll(int)));
	connect(horizontalScrollBar, SIGNAL(resize_move(double)), keyframeView, SLOT(resize_move(double)));
	connect(horizontalScrollBar, SIGNAL(valueChanged(int)), keyframeView, SLOT(set_x_scroll(int)));
	connect(verticalScrollBar, SIGNAL(valueChanged(int)), keyframeView, SLOT(set_y_scroll(int)));
	connect(verticalScrollBar, SIGNAL(valueChanged(int)), scrollArea->verticalScrollBar(), SLOT(setValue(int)));
	connect(scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)), verticalScrollBar, SLOT(setValue(int)));
}

EffectControls::~EffectControls() {}

bool EffectControls::keyframe_focus() {
	return headers->hasFocus() || keyframeView->hasFocus();
}

void EffectControls::set_zoom(bool in) {
	zoom *= (in) ? 2 : 0.5;
	update_keyframes();
}

void EffectControls::menu_select(QAction* q) {
	ComboAction* ca = new ComboAction();
	for (int i=0;i<selected_clips.size();i++) {
		Clip* c = sequence->clips.at(selected_clips.at(i));
		if ((c->track < 0) == (effect_menu_subtype == EFFECT_TYPE_VIDEO)) {
			const EffectMeta* meta = reinterpret_cast<const EffectMeta*>(q->data().value<quintptr>());
			if (effect_menu_type == EFFECT_TYPE_TRANSITION) {
				if (c->get_opening_transition() == NULL) {
					ca->append(new AddTransitionCommand(c, NULL, NULL, meta, TA_OPENING_TRANSITION, 30));
				}
				if (c->get_closing_transition() == NULL) {
					ca->append(new AddTransitionCommand(c, NULL, NULL, meta, TA_CLOSING_TRANSITION, 30));
				}
			} else {
				ca->append(new AddEffectCommand(c, NULL, meta));
			}
		}
	}
	undo_stack.push(ca);
	if (effect_menu_type == EFFECT_TYPE_TRANSITION) {
		update_ui(true);
	} else {
		reload_clips();
		panel_sequence_viewer->viewer_widget->update();
	}
}

void EffectControls::update_keyframes() {
	headers->update_zoom(zoom);
	keyframeView->update();
}

void EffectControls::delete_selected_keyframes() {
	keyframeView->delete_selected_keyframes();
}

void EffectControls::copy(bool del) {
	if (mode == TA_NO_TRANSITION) {
		bool cleared = false;

		ComboAction* ca = new ComboAction();
		EffectDeleteCommand* del_com = (del) ? new EffectDeleteCommand() : NULL;
		for (int i=0;i<selected_clips.size();i++) {
			Clip* c = sequence->clips.at(selected_clips.at(i));
			for (int j=0;j<c->effects.size();j++) {
				Effect* effect = c->effects.at(j);
				if (effect->container->selected) {
					if (!cleared) {
						clear_clipboard();
						cleared = true;
						clipboard_type = CLIPBOARD_TYPE_EFFECT;
					}

					clipboard.append(effect->copy(NULL));

					if (del_com != NULL) {
						del_com->clips.append(c);
						del_com->fx.append(j);
					}
				}
			}
		}
		if (del_com != NULL) {
			if (del_com->clips.size() > 0) {
				ca->append(del_com);
			} else {
				delete del_com;
			}
		}
		undo_stack.push(ca);
	}
}

void EffectControls::scroll_to_frame(long frame) {
	scroll_to_frame_internal(horizontalScrollBar, frame, zoom, keyframeView->width());
}

void EffectControls::show_effect_menu(int type, int subtype) {
	effect_menu_type = type;
	effect_menu_subtype = subtype;

	effects_loaded.lock();

	QMenu effects_menu(this);
	for (int i=0;i<effects.size();i++) {
		const EffectMeta& em = effects.at(i);

		if (em.type == type && em.subtype == subtype) {
			QAction* action = new QAction(&effects_menu);
			action->setText(em.name);
			action->setData(reinterpret_cast<quintptr>(&em));

			QMenu* parent = &effects_menu;
			if (!em.category.isEmpty()) {
				bool found = false;
				for (int j=0;j<effects_menu.actions().size();j++) {
					QAction* action = effects_menu.actions().at(j);
					if (action->menu() != NULL) {
						if (action->menu()->title() == em.category) {
							parent = action->menu();
							found = true;
							break;
						}
					}
				}
				if (!found) {
					parent = new QMenu(&effects_menu);
					parent->setTitle(em.category);

					bool found = false;
					for (int i=0;i<effects_menu.actions().size();i++) {
						QAction* comp_action = effects_menu.actions().at(i);
						if (comp_action->text() > em.category) {
							effects_menu.insertMenu(comp_action, parent);
							found = true;
							break;
						}
					}
					if (!found) effects_menu.addMenu(parent);
				}
			}

			bool found = false;
			for (int i=0;i<parent->actions().size();i++) {
				QAction* comp_action = parent->actions().at(i);
				if (comp_action->text() > action->text()) {
					parent->insertAction(comp_action, action);
					found = true;
					break;
				}
			}
			if (!found) parent->addAction(action);
		}
	}

	effects_loaded.unlock();

	connect(&effects_menu, SIGNAL(triggered(QAction*)), this, SLOT(menu_select(QAction*)));
	effects_menu.exec(QCursor::pos());
}

void EffectControls::clear_effects(bool clear_cache) {
	// clear existing clips
	deselect_all_effects(NULL);

	// clear graph editor
	if (panel_graph_editor != NULL) panel_graph_editor->set_row(NULL);

	QVBoxLayout* video_layout = static_cast<QVBoxLayout*>(video_effect_area->layout());
	QVBoxLayout* audio_layout = static_cast<QVBoxLayout*>(audio_effect_area->layout());
	QLayoutItem* item;
	while ((item = video_layout->takeAt(0))) {
		item->widget()->setParent(NULL);
		disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
	}
	while ((item = audio_layout->takeAt(0))) {
		item->widget()->setParent(NULL);
		disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
	}
	lblMultipleClipsSelected->setVisible(false);
	vcontainer->setVisible(false);
	acontainer->setVisible(false);
	headers->setVisible(false);
	keyframeView->setEnabled(false);
	if (clear_cache) selected_clips.clear();
	setWindowTitle(panel_name + "(none)");
}

void EffectControls::deselect_all_effects(QWidget* sender) {
	for (int i=0;i<selected_clips.size();i++) {
		Clip* c = sequence->clips.at(selected_clips.at(i));
		for (int j=0;j<c->effects.size();j++) {
			if (c->effects.at(j)->container != sender) {
				c->effects.at(j)->container->header_click(false, false);
			}
		}
	}
	panel_sequence_viewer->viewer_widget->update();
}

void EffectControls::open_effect(QVBoxLayout* layout, Effect* e) {
	CollapsibleWidget* container = e->container;
	layout->addWidget(container);
	connect(container, SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
}

void EffectControls::setup_ui() {
	QWidget* contents = new QWidget();

	QHBoxLayout* layout = new QHBoxLayout(contents);
	layout->setSpacing(0);
	layout->setMargin(0);

	QSplitter* splitter = new QSplitter(contents);
	splitter->setOrientation(Qt::Horizontal);
	splitter->setChildrenCollapsible(false);

	scrollArea = new QScrollArea(splitter);
	scrollArea->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setFrameShadow(QFrame::Plain);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	scrollArea->setWidgetResizable(true);

	QWidget* scrollAreaWidgetContents = new QWidget();

	QHBoxLayout* scrollAreaLayout = new QHBoxLayout(scrollAreaWidgetContents);
	scrollAreaLayout->setSpacing(0);
	scrollAreaLayout->setMargin(0);

	effects_area = new EffectsArea(scrollAreaWidgetContents);

	QVBoxLayout* effects_area_layout = new QVBoxLayout(effects_area);
	effects_area_layout->setSpacing(0);
	effects_area_layout->setMargin(0);

	vcontainer = new QWidget(effects_area);
	QVBoxLayout* vcontainerLayout = new QVBoxLayout(vcontainer);
	vcontainerLayout->setSpacing(0);
	vcontainerLayout->setMargin(0);

	QWidget* veHeader = new QWidget(vcontainer);
	veHeader->setObjectName(QStringLiteral("veHeader"));
	veHeader->setStyleSheet(QLatin1String("#veHeader { background: rgba(0, 0, 0, 0.25); }"));

	QHBoxLayout* veHeaderLayout = new QHBoxLayout(veHeader);
	veHeaderLayout->setSpacing(0);
	veHeaderLayout->setMargin(0);

	QPushButton* btnAddVideoEffect = new QPushButton(veHeader);
	btnAddVideoEffect->setIcon(QIcon(":/icons/add-effect.png"));
	btnAddVideoEffect->setToolTip("Add Video Effect");
	veHeaderLayout->addWidget(btnAddVideoEffect);
	connect(btnAddVideoEffect, SIGNAL(clicked(bool)), this, SLOT(video_effect_click()));

	veHeaderLayout->addStretch();

	QLabel* lblVideoEffects = new QLabel(veHeader);
	QFont font;
	font.setPointSize(9);
	lblVideoEffects->setFont(font);
	lblVideoEffects->setAlignment(Qt::AlignCenter);
	lblVideoEffects->setText("VIDEO EFFECTS");
	veHeaderLayout->addWidget(lblVideoEffects);

	veHeaderLayout->addStretch();

	QPushButton* btnAddVideoTransition = new QPushButton(veHeader);
	btnAddVideoTransition->setIcon(QIcon(":/icons/add-transition.png"));
	btnAddVideoTransition->setToolTip("Add Video Transition");
	connect(btnAddVideoTransition, SIGNAL(clicked(bool)), this, SLOT(video_transition_click()));

	veHeaderLayout->addWidget(btnAddVideoTransition);

	vcontainerLayout->addWidget(veHeader);

	video_effect_area = new QWidget(vcontainer);
	QVBoxLayout* veAreaLayout = new QVBoxLayout(video_effect_area);
	veAreaLayout->setSpacing(0);
	veAreaLayout->setMargin(0);

	vcontainerLayout->addWidget(video_effect_area);

	effects_area_layout->addWidget(vcontainer);

	acontainer = new QWidget(effects_area);
	QVBoxLayout* acontainerLayout = new QVBoxLayout(acontainer);
	acontainerLayout->setSpacing(0);
	acontainerLayout->setMargin(0);
	QWidget* aeHeader = new QWidget(acontainer);
	aeHeader->setObjectName(QStringLiteral("aeHeader"));
	aeHeader->setStyleSheet(QLatin1String("#aeHeader { background: rgba(0, 0, 0, 0.25); }"));

	QHBoxLayout* aeHeaderLayout = new QHBoxLayout(aeHeader);
	aeHeaderLayout->setSpacing(0);
	aeHeaderLayout->setMargin(0);

	QPushButton* btnAddAudioEffect = new QPushButton(aeHeader);
	btnAddAudioEffect->setIcon(QIcon(":/icons/add-effect.png"));
	btnAddAudioEffect->setToolTip("Add Audio Effect");
	connect(btnAddAudioEffect, SIGNAL(clicked(bool)), this, SLOT(audio_effect_click()));
	aeHeaderLayout->addWidget(btnAddAudioEffect);

	aeHeaderLayout->addStretch();

	QLabel* lblAudioEffects = new QLabel(aeHeader);
	lblAudioEffects->setFont(font);
	lblAudioEffects->setAlignment(Qt::AlignCenter);
	lblAudioEffects->setText("AUDIO EFFECTS");
	aeHeaderLayout->addWidget(lblAudioEffects);

	aeHeaderLayout->addStretch();

	QPushButton* btnAddAudioTransition = new QPushButton(aeHeader);
	btnAddAudioTransition->setIcon(QIcon(":/icons/add-transition.png"));
	btnAddAudioTransition->setToolTip("Add Audio Transition");
	connect(btnAddAudioTransition, SIGNAL(clicked(bool)), this, SLOT(audio_transition_click()));
	aeHeaderLayout->addWidget(btnAddAudioTransition);

	acontainerLayout->addWidget(aeHeader);

	audio_effect_area = new QWidget(acontainer);
	QVBoxLayout* aeAreaLayout = new QVBoxLayout(audio_effect_area);
	aeAreaLayout->setSpacing(0);
	aeAreaLayout->setMargin(0);

	acontainerLayout->addWidget(audio_effect_area);

	effects_area_layout->addWidget(acontainer);

	lblMultipleClipsSelected = new QLabel(effects_area);
	lblMultipleClipsSelected->setAlignment(Qt::AlignCenter);
	lblMultipleClipsSelected->setText("(Multiple clips selected)");
	effects_area_layout->addWidget(lblMultipleClipsSelected);

	effects_area_layout->addStretch();

	scrollAreaLayout->addWidget(effects_area);

	scrollArea->setWidget(scrollAreaWidgetContents);
	splitter->addWidget(scrollArea);
	QWidget* keyframeArea = new QWidget(splitter);
	QSizePolicy keyframe_sp;
	keyframe_sp.setHorizontalPolicy(QSizePolicy::Minimum);
	keyframe_sp.setVerticalPolicy(QSizePolicy::Preferred);
	keyframe_sp.setHorizontalStretch(1);
	keyframeArea->setSizePolicy(keyframe_sp);
	QVBoxLayout* keyframeAreaLayout = new QVBoxLayout(keyframeArea);
	keyframeAreaLayout->setSpacing(0);
	keyframeAreaLayout->setMargin(0);
	headers = new TimelineHeader(keyframeArea);

	keyframeAreaLayout->addWidget(headers);

	QWidget* keyframeCenterWidget = new QWidget(keyframeArea);

	QHBoxLayout* keyframeCenterLayout = new QHBoxLayout(keyframeCenterWidget);
	keyframeCenterLayout->setSpacing(0);
	keyframeCenterLayout->setMargin(0);

	keyframeView = new KeyframeView(keyframeCenterWidget);

	keyframeCenterLayout->addWidget(keyframeView);

	verticalScrollBar = new QScrollBar(keyframeCenterWidget);
	verticalScrollBar->setOrientation(Qt::Vertical);

	keyframeCenterLayout->addWidget(verticalScrollBar);


	keyframeAreaLayout->addWidget(keyframeCenterWidget);

	horizontalScrollBar = new ResizableScrollBar(keyframeArea);
	horizontalScrollBar->setOrientation(Qt::Horizontal);

	keyframeAreaLayout->addWidget(horizontalScrollBar);

	splitter->addWidget(keyframeArea);

	layout->addWidget(splitter);

	setWidget(contents);
}

void EffectControls::load_effects() {
	lblMultipleClipsSelected->setVisible(multiple);

	if (!multiple) {
		// load in new clips
		for (int i=0;i<selected_clips.size();i++) {
			Clip* c = sequence->clips.at(selected_clips.at(i));
			QVBoxLayout* layout;
			if (c->track < 0) {
				vcontainer->setVisible(true);
				layout = static_cast<QVBoxLayout*>(video_effect_area->layout());
			} else {
				acontainer->setVisible(true);
				layout = static_cast<QVBoxLayout*>(audio_effect_area->layout());
			}
			if (mode == TA_NO_TRANSITION) {
				for (int j=0;j<c->effects.size();j++) {
					open_effect(layout, c->effects.at(j));
				}
			} else if (mode == TA_OPENING_TRANSITION && c->get_opening_transition() != NULL) {
				open_effect(layout, c->get_opening_transition());
			} else if (mode == TA_CLOSING_TRANSITION && c->get_closing_transition() != NULL) {
				open_effect(layout, c->get_closing_transition());
			}
		}
		if (selected_clips.size() > 0) {
			setWindowTitle(panel_name + sequence->clips.at(selected_clips.at(0))->name);
			verticalScrollBar->setMaximum(qMax(0, effects_area->sizeHint().height() - headers->height() + scrollArea->horizontalScrollBar()->height()/* - keyframeView->height() - headers->height()*/));
			keyframeView->setEnabled(true);
			headers->setVisible(true);
			keyframeView->update();
		}
	}
}

void EffectControls::delete_effects() {
	// load in new clips
	if (mode == TA_NO_TRANSITION) {
		EffectDeleteCommand* command = new EffectDeleteCommand();
		for (int i=0;i<selected_clips.size();i++) {
			Clip* c = sequence->clips.at(selected_clips.at(i));
			for (int j=0;j<c->effects.size();j++) {
				Effect* effect = c->effects.at(j);
				if (effect->container->selected) {
					command->clips.append(c);
					command->fx.append(j);
				}
			}
		}
		if (command->clips.size() > 0) {
			undo_stack.push(command);
			panel_sequence_viewer->viewer_widget->update();
		} else {
			delete command;
		}
	}
}

void EffectControls::reload_clips() {
	clear_effects(false);
	load_effects();
}

void EffectControls::set_clips(QVector<int>& clips, int m) {
	clear_effects(true);

	// replace clip vector
	selected_clips = clips;
	mode = m;

	load_effects();
}

void EffectControls::video_effect_click() {
	show_effect_menu(EFFECT_TYPE_EFFECT, EFFECT_TYPE_VIDEO);
}

void EffectControls::audio_effect_click() {
	show_effect_menu(EFFECT_TYPE_EFFECT, EFFECT_TYPE_AUDIO);
}

void EffectControls::video_transition_click() {
	show_effect_menu(EFFECT_TYPE_TRANSITION, EFFECT_TYPE_VIDEO);
}

void EffectControls::audio_transition_click() {
	show_effect_menu(EFFECT_TYPE_TRANSITION, EFFECT_TYPE_AUDIO);
}

void EffectControls::resizeEvent(QResizeEvent*) {
	verticalScrollBar->setMaximum(qMax(0, effects_area->height() - keyframeView->height() - headers->height()));
}

bool EffectControls::is_focused() {
	if (this->hasFocus()) return true;
	for (int i=0;i<selected_clips.size();i++) {
		Clip* c = sequence->clips.at(selected_clips.at(i));
		if (c != NULL) {
			for (int j=0;j<c->effects.size();j++) {
				if (c->effects.at(j)->container->is_focused()) {
					return true;
				}
			}
		} else {
			dout << "[WARNING] Tried to check focus of a NULL clip";
		}
	}
	return false;
}

EffectsArea::EffectsArea(QWidget* parent) : QWidget(parent) {}

void EffectsArea::resizeEvent(QResizeEvent*) {
	parent_widget->setMinimumWidth(sizeHint().width());
}
