#include "export.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QSplitter>

ExportDialog::ExportDialog(QWidget *parent) :
  QDialog(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  QWidget* preferences_area = new QWidget();
  QGridLayout* preferences_layout = new QGridLayout(preferences_area);
  preferences_layout->addWidget(new QLabel(tr("Filename:")), 0, 0);
  preferences_layout->addWidget(new QLineEdit(), 0, 1);
  preferences_layout->addWidget(new QLabel(tr("Preset:")), 1, 0);
  preferences_layout->addWidget(new QComboBox(), 1, 1);
  QTabWidget* preferences_tabs = new QTabWidget();
  preferences_tabs->addTab(new QWidget(), tr("Video"));
  preferences_tabs->addTab(new QWidget(), tr("Audio"));
  preferences_layout->addWidget(preferences_tabs, 2, 0, 1, 2);
  preferences_layout->addWidget(new QPushButton(tr("Export")), 3, 0);
  preferences_layout->addWidget(new QPushButton(tr("Cancel")), 3, 1);
  splitter->addWidget(preferences_area);

  QWidget* preview_area = new QWidget();
  QVBoxLayout* preview_layout = new QVBoxLayout(preview_area);
  preview_layout->addWidget(new QLabel(tr("Preview")));
  preview_layout->addWidget(new QOpenGLWidget());
  splitter->addWidget(preview_area);
}
