#include "export.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>

#include "exportvideotab.h"
#include "exportaudiotab.h"
#include "ui/icons/icons.h"

ExportDialog::ExportDialog(QWidget *parent) :
  QDialog(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  QWidget* preferences_area = new QWidget();
  QGridLayout* preferences_layout = new QGridLayout(preferences_area);

  int row = 0;

  QLabel* fn_lbl = new QLabel(tr("Filename:"));
  fn_lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(fn_lbl, row, 0);
  QLineEdit* filename_edit = new QLineEdit();
  preferences_layout->addWidget(filename_edit, row, 1, 1, 2);
  QPushButton* file_browse_btn = new QPushButton();
  file_browse_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  file_browse_btn->setIcon(olive::icon::Folder);
  preferences_layout->addWidget(file_browse_btn, row, 3);

  row++;

  QLabel* preset_lbl = new QLabel(tr("Preset:"));
  preset_lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_lbl, row, 0);
  preferences_layout->addWidget(new QComboBox(), row, 1);
  QPushButton* preset_load_btn = new QPushButton();
  preset_load_btn->setIcon(olive::icon::Open);
  preset_load_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_load_btn, row, 2);
  QPushButton* preset_save_btn = new QPushButton();
  preset_save_btn->setIcon(olive::icon::Save);
  preset_save_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  preferences_layout->addWidget(preset_save_btn, row, 3);

  row++;

  QFrame* horizontal_line = new QFrame();
  horizontal_line->setFrameShape(QFrame::HLine);
  horizontal_line->setFrameShadow(QFrame::Sunken);
  preferences_layout->addWidget(horizontal_line, row, 0, 1, 4);

  row++;

  preferences_layout->addWidget(new QLabel(tr("Format:")), row, 0);
  preferences_layout->addWidget(new QComboBox(), row, 1, 1, 3);

  row++;

  QTabWidget* preferences_tabs = new QTabWidget();
  QScrollArea* video_area = new QScrollArea();
  video_area->setWidgetResizable(true);
  video_area->setWidget(new ExportVideoTab());
  preferences_tabs->addTab(video_area, tr("Video"));
  QScrollArea* audio_area = new QScrollArea();
  audio_area->setWidgetResizable(true);
  audio_area->setWidget(new ExportAudioTab());
  preferences_tabs->addTab(audio_area, tr("Audio"));
  preferences_layout->addWidget(preferences_tabs, row, 0, 1, 4);

  row++;

  QDialogButtonBox* buttons = new QDialogButtonBox();
  buttons->setCenterButtons(true);
  buttons->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
  buttons->addButton(QDialogButtonBox::Cancel);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  preferences_layout->addWidget(buttons, row, 0, 1, 4);

  splitter->addWidget(preferences_area);

  QWidget* preview_area = new QWidget();
  QVBoxLayout* preview_layout = new QVBoxLayout(preview_area);
  preview_layout->addWidget(new QLabel(tr("Preview")));
  preview_viewer_ = new ViewerWidget();
  preview_viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  preview_layout->addWidget(preview_viewer_);
  splitter->addWidget(preview_area);
}
