#ifndef PREFERENCESQUALITYTAB_H
#define PREFERENCESQUALITYTAB_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QStackedWidget>

#include "preferencestab.h"

class PreferencesQualityGroup : public QGroupBox
{
  Q_OBJECT
public:
  PreferencesQualityGroup(const QString& title, QWidget* parent = nullptr);

  QComboBox* bit_depth_combobox();

  QComboBox* ocio_method();

  QComboBox* sample_fmt_combobox();

private:
  QComboBox* bit_depth_combobox_;

  QComboBox* ocio_method_;

  QComboBox* sample_fmt_combobox_;

};

class PreferencesQualityTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesQualityTab();

  virtual void Accept() override;

private:
  QStackedWidget* quality_stack_;

  PreferencesQualityGroup* offline_group_;

  PreferencesQualityGroup* online_group_;

};

#endif // PREFERENCESQUALITYTAB_H
