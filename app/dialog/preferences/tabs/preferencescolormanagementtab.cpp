#include "preferencescolormanagementtab.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

PreferencesColorManagementTab::PreferencesColorManagementTab()
{
  QGridLayout* color_management_layout = new QGridLayout(this);

  int row = 0;

  QGroupBox* opencolorio_groupbox = new QGroupBox();
  QGridLayout* opencolorio_groupbox_layout = new QGridLayout(opencolorio_groupbox);

  // COLOR MANAGEMENT -> OpenColorIO Config File
  opencolorio_groupbox_layout->addWidget(new QLabel(tr("OpenColorIO Config File:")), 0, 0);

  ocio_config_file = new QLineEdit();
  //  ocio_config_file->setText(olive::config.ocio_config_path);
  connect(ocio_config_file, SIGNAL(textChanged(const QString &)), this, SLOT(update_ocio_config(const QString&)));
  opencolorio_groupbox_layout->addWidget(ocio_config_file, 0, 1, 1, 4);

  QPushButton* ocio_config_browse_btn = new QPushButton(tr("Browse"));
  connect(ocio_config_browse_btn, SIGNAL(clicked(bool)), this, SLOT(browse_ocio_config()));
  opencolorio_groupbox_layout->addWidget(ocio_config_browse_btn, 0, 5);

  // COLOR MANAGEMENT -> Default Input Color Space
  ocio_default_input = new QComboBox();
  opencolorio_groupbox_layout->addWidget(new QLabel(tr("Default Input Color Space:")), 1, 0);
  opencolorio_groupbox_layout->addWidget(ocio_default_input, 1, 1, 1, 5);

  // COLOR MANAGEMENT -> Display
  ocio_display = new QComboBox();
  connect(ocio_display, SIGNAL(currentIndexChanged(int)), this, SLOT(update_ocio_view_menu()));
  opencolorio_groupbox_layout->addWidget(new QLabel(tr("Display:")), 2, 0);
  opencolorio_groupbox_layout->addWidget(ocio_display, 2, 1);

  // COLOR MANAGEMENT -> View
  ocio_view = new QComboBox();
  opencolorio_groupbox_layout->addWidget(new QLabel(tr("View:")), 2, 2);
  opencolorio_groupbox_layout->addWidget(ocio_view, 2, 3);

  // COLOR MANAGEMENT -> Look
  ocio_look = new QComboBox();
  opencolorio_groupbox_layout->addWidget(new QLabel(tr("Look:")), 2, 4);
  opencolorio_groupbox_layout->addWidget(ocio_look, 2, 5);

  color_management_layout->addWidget(opencolorio_groupbox, row, 0);

  row++;

  // COLOR MANAGEMENT -> Bit Depth
  QGroupBox* bit_depth_groupbox = new QGroupBox(tr("Bit Depth"));
  QGridLayout* bit_depth_groupbox_layout = new QGridLayout(bit_depth_groupbox);

  // COLOR MANAGEMENT -> Bit Depth -> Playback
  playback_bit_depth = new QComboBox();
  /*for (int i=0;i<olive::pixel_formats.size();i++) {
      playback_bit_depth->addItem(olive::pixel_formats.at(i).name, i);
    }
    playback_bit_depth->setCurrentIndex(olive::config.playback_bit_depth);*/
  bit_depth_groupbox_layout->addWidget(new QLabel(tr("Playback (Offline):")), 0, 0);
  bit_depth_groupbox_layout->addWidget(playback_bit_depth, 0, 1);

  // COLOR MANAGEMENT -> Bit Depth -> Export
  export_bit_depth = new QComboBox();
  /*for (int i=0;i<olive::pixel_formats.size();i++) {
      export_bit_depth->addItem(olive::pixel_formats.at(i).name, i);
    }
    export_bit_depth->setCurrentIndex(olive::config.export_bit_depth);*/
  bit_depth_groupbox_layout->addWidget(new QLabel(tr("Export (Online):")), 0, 2);
  bit_depth_groupbox_layout->addWidget(export_bit_depth, 0, 3);

  color_management_layout->addWidget(bit_depth_groupbox, row, 0);

  //row++;

  populate_ocio_menus(OCIO::GetCurrentConfig());
}

void PreferencesColorManagementTab::Accept()
{

}

void PreferencesColorManagementTab::populate_ocio_menus(OCIO::ConstConfigRcPtr config)
{
  if (!config) {

    // Just clear everything
    ocio_display->clear();
    ocio_default_input->clear();
    ocio_view->clear();
    ocio_look->clear();

  } else {

    // Get input color spaces for setting the default input color space
    ocio_default_input->clear();
    for (int i=0;i<config->getNumColorSpaces();i++) {
      QString colorspace = config->getColorSpaceNameByIndex(i);

      ocio_default_input->addItem(colorspace);

      /*if (colorspace == olive::config.ocio_default_input_colorspace) {
        ocio_default_input->setCurrentIndex(i);
      }*/
    }

    // Get current display name (if the config is empty, get the current default display)
    /*QString current_display = olive::config.ocio_display;
    if (current_display.isEmpty()) {
      current_display = config->getDefaultDisplay();
    }*/

    // Populate the display menu
    ocio_display->clear();
    for (int i=0;i<config->getNumDisplays();i++) {
      ocio_display->addItem(config->getDisplay(i));

      // Check if this index is the currently selected
      /*if (config->getDisplay(i) == current_display) {
        ocio_display->setCurrentIndex(i);
      }*/
    }

    update_ocio_view_menu(config);

    // Populate the look menu
    ocio_look->clear();
    ocio_look->addItem(tr("(None)"), QString());
    for (int i=0;i<config->getNumLooks();i++) {
      const char* look = config->getLookNameByIndex(i);

      ocio_look->addItem(look, look);

      /*if (look == olive::config.ocio_look) {
        ocio_look->setCurrentIndex(i+1);
      }*/
    }

  }
}

OCIO::ConstConfigRcPtr PreferencesColorManagementTab::TestOCIOConfig(const QString &url)
{
  // Check whether OCIO can load it
  OCIO::ConstConfigRcPtr config;
  try {
    config = OCIO::Config::CreateFromFile(url.toUtf8());
  } catch (OCIO::Exception& e) {
    QMessageBox::critical(this,
                          tr("OpenColorIO Config Error"),
                          tr("Failed to set OpenColorIO configuration: %1").arg(e.what()),
                          QMessageBox::Ok);
  }
  return config;
}

void PreferencesColorManagementTab::update_ocio_view_menu(OCIO::ConstConfigRcPtr config)
{

  // Get views for the current display set in `ocio_display`
  QString display = ocio_display->currentText();

  // Get current view
  /*QString current_view = olive::config.ocio_view;
  if (current_view.isEmpty()) {
    current_view = config->getDefaultView(display.toUtf8());
  }*/

  // Populate the view menu
  int ocio_view_count = config->getNumViews(display.toUtf8());
  ocio_view->clear();
  for (int i=0;i<ocio_view_count;i++) {
    const char* view = config->getView(display.toUtf8(), i);

    ocio_view->addItem(view);

    /*if (current_view == view) {
      ocio_view->setCurrentIndex(i);
    }*/
  }
}

void PreferencesColorManagementTab::update_ocio_config(const QString &s)
{
  OCIO::ConstConfigRcPtr file_config;

  if (!s.isEmpty() && QFileInfo::exists(s)) {
    file_config = TestOCIOConfig(s);
  }

  populate_ocio_menus(file_config);
}

void PreferencesColorManagementTab::browse_ocio_config()
{
  QString fn = QFileDialog::getOpenFileName(this, tr("Browse for OpenColorIO configuration"));
  if (!fn.isEmpty()) {
    ocio_config_file->setText(fn);
  }
}

void PreferencesColorManagementTab::update_ocio_view_menu()
{
  update_ocio_view_menu(OCIO::GetCurrentConfig());
}
