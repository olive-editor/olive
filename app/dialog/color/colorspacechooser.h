#ifndef COLORSPACECHOOSER_H
#define COLORSPACECHOOSER_H

#include <QComboBox>
#include <QGroupBox>

#include "render/colormanager.h"

class ColorSpaceChooser : public QGroupBox
{
  Q_OBJECT
public:
  ColorSpaceChooser(ColorManager* color_manager, bool enable_input_field = true, QWidget* parent = nullptr);

  QString input() const;
  QString display() const;
  QString view() const;
  QString look() const;

  void set_input(const QString& s);

signals:
  void ColorSpaceChanged(const QString& input, const QString& display, const QString& view, const QString& look);

  void DisplayColorSpaceChanged(const QString& display, const QString& view, const QString& look);

private slots:
  void UpdateViews(const QString &s);

private:
  ColorManager* color_manager_;

  QComboBox* input_combobox_;

  QComboBox* display_combobox_;

  QComboBox* view_combobox_;

  QComboBox* look_combobox_;

private slots:
  void ComboBoxChanged();

};

#endif // COLORSPACECHOOSER_H
