#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>

#include "render/color.h"
#include "render/colormanager.h"

class ColorButton : public QPushButton
{
  Q_OBJECT
public:
  ColorButton(ColorManager* color_manager, QWidget* parent = nullptr);

  const Color& GetColor() const;

public slots:
  void SetColor(const Color& c);

signals:
  void ColorChanged(const Color& c);

private slots:
  void ShowColorDialog();

private:
  void UpdateColor();

  ColorManager* color_manager_;

  Color color_;

  QString cm_input_;

  ColorProcessorPtr color_processor_;

};

#endif // COLORBUTTON_H
