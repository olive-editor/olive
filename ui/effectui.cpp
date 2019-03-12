#include "effectui.h"

#include <QPoint>
#include <QMenu>

#include "project/clip.h"
#include "ui/menuhelper.h"

EffectUI::EffectUI(Effect* e) :
  effect_(e)
{
  Q_ASSERT(e != nullptr);

  SetText(e->name);

  QWidget* ui = new QWidget(this);
  SetContents(ui);

  layout_ = new QGridLayout(ui);
  layout_->setSpacing(4);

  connect(title_bar,
          SIGNAL(customContextMenuRequested(const QPoint&)),
          this,
          SIGNAL(TitleBarContextMenuRequested(const QPoint&)));

  for (int i=0;i<e->row_count();i++) {
    EffectRow* row = e->row(i);

    QLabel* row_label = new QLabel(row->name());

    labels_.append(row_label);

    layout_->addWidget(row_label, i, 0);

    for (int j=0;j<row->FieldCount();j++) {
      EffectField* field = row->Field(j);

      QWidget* widget = field->CreateWidget();

      layout_->addWidget(widget, i, j + 1);
    }
  }

  connect(enabled_check, SIGNAL(clicked(bool)), this, SLOT(field_changed()));
}

Effect *EffectUI::GetEffect()
{
  return effect_;
}

int EffectUI::GetRowPos(int row)
{
  QLabel* row_label = labels_.at(row);
  return mapToGlobal(row_label->rect().center()).y();
}

void EffectUI::show_context_menu(const QPoint& pos) {
  if (effect_->meta->type == EFFECT_TYPE_EFFECT) {
    QMenu menu;

    Clip* c = effect_->parent_clip;

    int index = c->IndexOfEffect(effect_);

    menu.addAction(tr("Cu&t"), this, SIGNAL(CutRequested()));
    menu.addAction(tr("&Copy"), this, SIGNAL(CopyRequested()));

    olive::MenuHelper.create_effect_paste_action(&menu);

    menu.addSeparator();

    if (index > 0) {
      menu.addAction(tr("Move &Up"), this, SLOT(move_up()));
    }

    if (index < c->effects.size() - 1) {
      menu.addAction(tr("Move &Down"), this, SLOT(move_down()));
    }

    menu.addSeparator();

    menu.addAction(tr("D&elete"), this, SLOT(delete_self()));

    menu.addSeparator();

    menu.addAction(tr("Load Settings From File"), this, SLOT(load_from_file()));

    menu.addAction(tr("Save Settings to File"), this, SLOT(save_to_file()));

    menu.exec(title_bar->mapToGlobal(pos));
  }
}
