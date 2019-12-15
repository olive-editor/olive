#ifndef EXPORTVIDEOTAB_H
#define EXPORTVIDEOTAB_H

#include <QWidget>

class ExportVideoTab : public QWidget
{
public:
  ExportVideoTab(QWidget* parent = nullptr);

private:
  QWidget* SetupResolutionSection();
  QWidget* SetupColorSection();
  QWidget* SetupCodecSection();
};

#endif // EXPORTVIDEOTAB_H
