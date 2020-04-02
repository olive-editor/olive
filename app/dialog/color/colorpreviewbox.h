#ifndef COLORPREVIEWBOX_H
#define COLORPREVIEWBOX_H

#include <QWidget>

#include "render/color.h"
#include "render/colorprocessor.h"

class ColorPreviewBox : public QWidget
{
  Q_OBJECT
public:
  ColorPreviewBox(QWidget* parent = nullptr);

  void SetColorProcessor(ColorProcessorPtr to_linear, ColorProcessorPtr to_display);

public slots:
  void SetColor(const Color& c);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private:
  Color color_;

  ColorProcessorPtr to_linear_processor_;

  ColorProcessorPtr to_display_processor_;

};

#endif // COLORPREVIEWBOX_H
