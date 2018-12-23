#include "effectcontrols.h"
#include "ui_effectcontrols.h"

#include <QMenu>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>

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
#include "ui/viewerwidget.h"
#include "io/clipboard.h"
#include "debug.h"

EffectControls::EffectControls(QWidget *parent) :
	QDockWidget(parent),
	multiple(false),
    zoom(1),
    ui(new Ui::EffectControls),
    panel_name("Effects: "),
    mode(TA_NO_TRANSITION)
{
    ui->setupUi(this);

	init_effects();
    clear_effects(false);
	ui->headers->viewer = panel_sequence_viewer;
	ui->headers->snapping = false;

	ui->effects_area->parent_widget = ui->scrollArea;
	ui->effects_area->keyframe_area = ui->keyframeView;
	ui->effects_area->header = ui->headers;
	ui->keyframeView->header = ui->headers;

	ui->label_2->setVisible(false);

	connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), ui->headers, SLOT(set_scroll(int)));
	connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), ui->keyframeView, SLOT(set_x_scroll(int)));
	connect(ui->verticalScrollBar, SIGNAL(valueChanged(int)), ui->keyframeView, SLOT(set_y_scroll(int)));
	connect(ui->verticalScrollBar, SIGNAL(valueChanged(int)), ui->scrollArea->verticalScrollBar(), SLOT(setValue(int)));
	connect(ui->scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->verticalScrollBar, SLOT(setValue(int)));
}

EffectControls::~EffectControls() {
	delete ui;
}

bool EffectControls::keyframe_focus() {
	return ui->headers->hasFocus() || ui->keyframeView->hasFocus();
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
	ui->headers->update_zoom(zoom);
	ui->keyframeView->update();
}

void EffectControls::delete_selected_keyframes() {
    ui->keyframeView->delete_selected_keyframes();
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
                        clipboard_type = CLIPBOARD_TYPE_EFFECT;
                        clear_clipboard();
                        cleared = true;
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

    QVBoxLayout* video_layout = static_cast<QVBoxLayout*>(ui->video_effect_area->layout());
    QVBoxLayout* audio_layout = static_cast<QVBoxLayout*>(ui->audio_effect_area->layout());
    QLayoutItem* item;
    while ((item = video_layout->takeAt(0))) {
        item->widget()->setParent(NULL);
		disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
    }
    while ((item = audio_layout->takeAt(0))) {
        item->widget()->setParent(NULL);
        disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
    }
    ui->label_2->setVisible(false);
    ui->vcontainer->setVisible(false);
    ui->acontainer->setVisible(false);
	ui->headers->setVisible(false);
	ui->keyframeView->setEnabled(false);
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

void EffectControls::load_effects() {
	ui->label_2->setVisible(multiple);

	if (!multiple) {        
		// load in new clips
		for (int i=0;i<selected_clips.size();i++) {
            Clip* c = sequence->clips.at(selected_clips.at(i));
			QVBoxLayout* layout;
			if (c->track < 0) {
				ui->vcontainer->setVisible(true);
				layout = static_cast<QVBoxLayout*>(ui->video_effect_area->layout());
			} else {
				ui->acontainer->setVisible(true);
				layout = static_cast<QVBoxLayout*>(ui->audio_effect_area->layout());
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
			ui->verticalScrollBar->setMaximum(qMax(0, ui->effects_area->sizeHint().height() - ui->headers->height() + ui->scrollArea->horizontalScrollBar()->height()/* - ui->keyframeView->height() - ui->headers->height()*/));
			ui->keyframeView->setEnabled(true);
			ui->headers->setVisible(true);
			ui->keyframeView->update();
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

void EffectControls::on_add_video_effect_button_clicked() {
    show_effect_menu(EFFECT_TYPE_EFFECT, EFFECT_TYPE_VIDEO);
}

void EffectControls::on_add_audio_effect_button_clicked() {
    show_effect_menu(EFFECT_TYPE_EFFECT, EFFECT_TYPE_AUDIO);
}

void EffectControls::on_add_video_transition_button_clicked() {
    show_effect_menu(EFFECT_TYPE_TRANSITION, EFFECT_TYPE_VIDEO);
}

void EffectControls::on_add_audio_transition_button_clicked() {
    show_effect_menu(EFFECT_TYPE_TRANSITION, EFFECT_TYPE_AUDIO);
}

void EffectControls::resizeEvent(QResizeEvent*) {
	ui->verticalScrollBar->setMaximum(qMax(0, ui->effects_area->height() - ui->keyframeView->height() - ui->headers->height()));
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
