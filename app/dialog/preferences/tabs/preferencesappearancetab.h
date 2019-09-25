#ifndef PREFERENCESAPPEARANCETAB_H
#define PREFERENCESAPPEARANCETAB_H

#include <QComboBox>
#include <QLineEdit>

#include "preferencestab.h"
#include "ui/style/style.h"

class PreferencesAppearanceTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesAppearanceTab();

  virtual void Accept() override;

private:
  /**
   * @brief Show a file dialog to browse for an external CSS file to load for styling the application.
   */
  void BrowseForCSS();

  /**
   * @brief UI widget for selecting the current UI style
   */
  QComboBox* style_;

  /**
   * @brief List of internal styles
   */
  QList<StyleDescriptor> style_list_;

  QString custom_style_path_;
};

#endif // PREFERENCESAPPEARANCETAB_H
