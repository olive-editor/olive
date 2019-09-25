#ifndef PREFERENCESCOLORMANAGEMENTTAB_H
#define PREFERENCESCOLORMANAGEMENTTAB_H

#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "preferencestab.h"

class PreferencesColorManagementTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesColorManagementTab();

  virtual void Accept() override;

private slots:
  // OCIO function
  void browse_ocio_config();
  void update_ocio_view_menu();
  void update_ocio_view_menu(OCIO::ConstConfigRcPtr config);
  void update_ocio_config(const QString&);

private:
  /**
   * @brief Tests an OpenColorIO configuration file to determine whether it's valid and throws a messagebox if not
   *
   * @param url
   *
   * URL to the OpenColorIO configuration file.
   *
   * @return
   *
   * A OCIO::ConstConfigRcPtr config pointer if the configuration file is valid, nullptr if not.
   */
  OCIO::ConstConfigRcPtr TestOCIOConfig(const QString& url);

  void populate_ocio_menus(OCIO::ConstConfigRcPtr config);

  QLineEdit* ocio_config_file;
  QComboBox* ocio_default_input;
  QComboBox* ocio_display;
  QComboBox* ocio_view;
  QComboBox* ocio_look;
  QComboBox* playback_bit_depth;
  QComboBox* export_bit_depth;
};

#endif // PREFERENCESCOLORMANAGEMENTTAB_H
