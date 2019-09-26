#include "preferencesappearancetab.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

PreferencesAppearanceTab::PreferencesAppearanceTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  QGridLayout* appearance_layout = new QGridLayout();
  layout->addLayout(appearance_layout);

  int row = 0;

  // Appearance -> Theme
  appearance_layout->addWidget(new QLabel(tr("Theme")), row, 0);

  style_ = new QComboBox();

  style_list_ = StyleManager::ListInternal();

  foreach (StyleDescriptor s, style_list_) {
    style_->addItem(s.name(), s.path());

    if (s.path() == Config::Current()["Style"]) {
      style_->setCurrentIndex(style_->count()-1);
    }
  }

  appearance_layout->addWidget(style_, row, 1, 1, 2);

  row++;

  layout->addStretch();
}

void PreferencesAppearanceTab::Accept()
{
  QString style_path = style_->currentData().toString();
  StyleManager::SetStyle(style_path);
  Config::Current()["Style"] = style_path;

  if (style_->currentIndex() < style_list_.size()) {
    // This is an internal style, set accordingly

  } else {
    StyleManager::SetStyle(style_->currentData().toString());
  }
}
