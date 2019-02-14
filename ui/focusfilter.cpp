#include "focusfilter.h"

#include "panels/panels.h"

FocusFilter Olive::FocusFilter;

FocusFilter::FocusFilter()
{

}

void FocusFilter::go_to_in() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->go_to_in();
    } else {
        panel_sequence_viewer->go_to_in();
    }
}

void FocusFilter::go_to_out() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->go_to_out();
    } else {
        panel_sequence_viewer->go_to_out();
    }
}

void FocusFilter::go_to_start() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->go_to_start();
    } else {
        panel_sequence_viewer->go_to_start();
    }
}

void FocusFilter::prev_frame() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->previous_frame();
    } else {
        panel_sequence_viewer->previous_frame();
    }
}

void FocusFilter::play_in_to_out() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->play(true);
    } else {
        panel_sequence_viewer->play(true);
    }
}

void FocusFilter::next_frame() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->next_frame();
    } else {
        panel_sequence_viewer->next_frame();
    }
}

void FocusFilter::go_to_end() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->go_to_end();
    } else {
        panel_sequence_viewer->go_to_end();
    }
}

void FocusFilter::playpause() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->toggle_play();
    } else {
        panel_sequence_viewer->toggle_play();
    }
}

void FocusFilter::pause() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->pause();
    } else {
        panel_sequence_viewer->pause();
    }
}

void FocusFilter::increase_speed() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->increase_speed();
    } else {
        panel_sequence_viewer->increase_speed();
    }
}

void FocusFilter::decrease_speed() {
    QDockWidget* focused_panel = get_focused_panel();
    if (focused_panel == panel_footage_viewer) {
        panel_footage_viewer->decrease_speed();
    } else {
        panel_sequence_viewer->decrease_speed();
    }
}
