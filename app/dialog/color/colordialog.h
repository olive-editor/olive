#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QDialog>

#include "colorspacechooser.h"
#include "colorvalueswidget.h"
#include "render/color.h"
#include "render/colormanager.h"
#include "widget/colorwheel/colorwheelwidget.h"
#include "widget/colorwheel/colorgradientwidget.h"

class ColorDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief ColorDialog Constructor
   *
   * @param color_manager
   *
   * The ColorManager to use for color management. This must be valid.
   *
   * @param start
   *
   * The color to start with. This must be in the color_manager's reference space
   *
   * @param input_cs
   *
   * The input range that the user should see. The start color will be converted to this for UI object.
   *
   * @param parent
   *
   * QWidget parent.
   */
  ColorDialog(ColorManager* color_manager, Color start = Color(1.0f, 1.0f, 1.0f), QString input_cs = QString(), QWidget* parent = nullptr);

  /**
   * @brief Retrieves the color selected by the user
   *
   * The color is always returned in the ColorManager's reference space (usually scene linear).
   */
  Color GetSelectedColor() const;

  QString GetColorSpaceInput() const;

  QString GetColorSpaceDisplay() const;

  QString GetColorSpaceView() const;

  QString GetColorSpaceLook() const;

private:
  ColorManager* color_manager_;

  ColorWheelWidget* color_wheel_;

  ColorGradientWidget* hsv_value_gradient_;

  ColorValuesWidget* color_values_widget_;

  ColorProcessorPtr to_linear_processor_;

  ColorSpaceChooser* chooser_;

private slots:
  void ColorSpaceChanged(const QString& input, const QString& display, const QString& view, const QString& look);

};

#endif // COLORDIALOG_H
