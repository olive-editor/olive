#include "effectspanel.h"

#include "timeline.h"
#include "global/clipboard.h"
#include "panels.h"

EffectsPanel::EffectsPanel(QWidget *parent) :
  Panel(parent)
{

}

EffectsPanel::~EffectsPanel()
{
  Clear(true);
}

void EffectsPanel::Clear(bool clear_cache) {
  // clear existing clips
  deselect_all_effects(nullptr);

  for (int i=0;i<open_effects_.size();i++) {
    delete open_effects_.at(i);
  }
  open_effects_.clear();


  if (clear_cache) {
    selected_clips_.clear();
  }

  ClearEvent();
}

bool EffectsPanel::focused()
{
  for (int i=0;i<open_effects_.size();i++) {
    if (open_effects_.at(i)->IsFocused()) {
      return true;
    }
  }

  return Panel::focused();
}

void EffectsPanel::ClearEvent() {}

void EffectsPanel::LoadEvent() {}

void EffectsPanel::Reload() {
  Clear(false);
  Load();
}

void EffectsPanel::SetClips()
{
  Clear(true);

  Sequence* top_sequence = Timeline::GetTopSequence().get();

  if (top_sequence == nullptr) {
    selected_clips_.clear();
  } else {
    // replace clip vector
    selected_clips_ = top_sequence->SelectedClips(false);

    Load();
  }
}

void EffectsPanel::Load() {
  bool graph_editor_row_is_still_active = false;

  // load in new clips
  for (int i=0;i<selected_clips_.size();i++) {
    Clip* c = selected_clips_.at(i);

    // Create a list of the effects we'll open
    QVector<Effect*> effects_to_open;

    // Determine based on the current selections whether to load all effects or just the transitions
    bool whole_clip_is_selected = c->IsSelected();

    if (whole_clip_is_selected) {
      for (int j=0;j<c->effects.size();j++) {
        effects_to_open.append(c->effects.at(j).get());
      }
    }
    if (c->opening_transition != nullptr
        && (whole_clip_is_selected || c->track()->IsTransitionSelected(c->opening_transition.get()))) {
      effects_to_open.append(c->opening_transition.get());
    }
    if (c->closing_transition != nullptr
        && (whole_clip_is_selected || c->track()->IsTransitionSelected(c->closing_transition.get()))) {
      effects_to_open.append(c->closing_transition.get());
    }

    for (int j=0;j<effects_to_open.size();j++) {

      // Check if we've already opened an effect of this type before
      bool already_opened = false;
      for (int k=0;k<open_effects_.size();k++) {
        if (open_effects_.at(k)->GetEffect()->meta == effects_to_open.at(j)->meta
            && !open_effects_.at(k)->IsAttachedToClip(c)) {

          open_effects_.at(k)->AddAdditionalEffect(effects_to_open.at(j));

          already_opened = true;

          break;
        }
      }

      if (!already_opened) {
        open_effect(effects_to_open.at(j));
      }

      // Check if one of the open effects contains the row currently active in the graph editor. If not, we'll have
      // to clear the graph editor later.
      if (!graph_editor_row_is_still_active) {
        for (int k=0;k<effects_to_open.at(j)->row_count();k++) {
          EffectRow* row = effects_to_open.at(j)->row(k);
          if (row == panel_graph_editor->get_row()) {
            graph_editor_row_is_still_active = true;
            break;
          }
        }
      }
    }
  }

  // If the graph editor's currently active row is not part of the current effects, clear it
  if (!graph_editor_row_is_still_active) {
    panel_graph_editor->set_row(nullptr);
  }

  LoadEvent();
}

bool EffectsPanel::IsEffectSelected(Effect *e)
{
  for (int i=0;i<open_effects_.size();i++) {
    if (open_effects_.at(i)->GetEffect() == e && open_effects_.at(i)->IsSelected()) {
      return true;
    }
  }
  return false;
}

void EffectsPanel::deselect_all_effects(QWidget* sender) {

  for (int i=0;i<open_effects_.size();i++) {
    if (open_effects_.at(i) != sender) {
      open_effects_.at(i)->Deselect();
    }
  }

  if (panel_sequence_viewer != nullptr) {
    panel_sequence_viewer->viewer_widget()->update();
  }
}

void EffectsPanel::cut() {
  copy(true);
}

void EffectsPanel::copy(bool del) {
  bool cleared = false;

  ComboAction* ca = nullptr;
  if (del) {
    ca = new ComboAction();
  }

  for (int i=0;i<open_effects_.size();i++) {
    if (open_effects_.at(i)->IsSelected()) {
      Effect* e = open_effects_.at(i)->GetEffect();

      if (e->meta->type == EFFECT_TYPE_EFFECT) {

        if (!cleared) {
          olive::clipboard.Clear();
          cleared = true;
          olive::clipboard.SetType(Clipboard::CLIPBOARD_TYPE_EFFECT);
        }

        olive::clipboard.Append(e->copy(nullptr));

        if (del) {

          DeleteEffect(ca, e);

        }

      }
    }
  }

  if (del) {
    if (ca->hasActions()) {
      olive::undo_stack.push(ca);
    } else {
      delete ca;
    }
  }
}

void EffectsPanel::DeleteEffect(ComboAction* ca, Effect* effect_ref) {
  if (effect_ref->meta->type == EFFECT_TYPE_EFFECT) {

    ca->append(new EffectDeleteCommand(effect_ref));

  } else if (effect_ref->meta->type == EFFECT_TYPE_TRANSITION) {

    // Retrieve shared ptr for this transition

    Clip* attached_clip = effect_ref->parent_clip;

    TransitionPtr t = nullptr;

    if (attached_clip->opening_transition.get() == effect_ref) {

      t = attached_clip->opening_transition;

    } else if (attached_clip->closing_transition.get() == effect_ref) {

      t = attached_clip->closing_transition;

    }

    if (t == nullptr) {

      qWarning() << "Failed to delete transition, couldn't find clip link.";

    } else {

      ca->append(new DeleteTransitionCommand(t));

    }

  }
}

void EffectsPanel::DeleteSelectedEffects() {
  ComboAction* ca = new ComboAction();

  for (int i=0;i<open_effects_.size();i++) {
    if (open_effects_.at(i)->IsSelected()) {
      DeleteEffect(ca, open_effects_.at(i)->GetEffect());
    }
  }

  if (ca->hasActions()) {
    olive::undo_stack.push(ca);
    update_ui(true);
  } else {
    delete ca;
  }
}

void EffectsPanel::open_effect(Effect* e) {
  EffectUI* container = new EffectUI(e);

  connect(container, SIGNAL(CutRequested()), this, SLOT(cut()));
  connect(container, SIGNAL(CopyRequested()), this, SLOT(copy()));
  connect(container, SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));

  open_effects_.append(container);
}
