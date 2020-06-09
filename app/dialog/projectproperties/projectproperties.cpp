/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "projectproperties.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "config/config.h"
#include "core.h"
#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

ProjectPropertiesDialog::ProjectPropertiesDialog(Project* p, QWidget *parent) :
  QDialog(parent),
  working_project_(p),
  ocio_config_is_valid_(true)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  setWindowTitle(tr("Project Properties for '%1'").arg(working_project_->name()));

  QTabWidget* tabs = new QTabWidget;
  layout->addWidget(tabs);

  {
    // Color management group
    QWidget* color_group = new QWidget();

    QGridLayout* color_layout = new QGridLayout(color_group);

    int row = 0;

    color_layout->addWidget(new QLabel(tr("OpenColorIO Configuration:")), row, 0);

    ocio_filename_ = new QLineEdit();
    ocio_filename_->setPlaceholderText(tr("(default)"));
    color_layout->addWidget(ocio_filename_, row, 1);

    row++;

    color_layout->addWidget(new QLabel(tr("Default Input Color Space:")), row, 0);

    default_input_colorspace_ = new QComboBox();
    color_layout->addWidget(default_input_colorspace_, row, 1, 1, 2);

    row++;

    QPushButton* browse_btn = new QPushButton(tr("Browse"));
    color_layout->addWidget(browse_btn, 0, 2);
    connect(browse_btn, &QPushButton::clicked, this, &ProjectPropertiesDialog::BrowseForOCIOConfig);

    ocio_filename_->setText(working_project_->color_manager()->GetConfigFilename());

    connect(ocio_filename_, &QLineEdit::textChanged, this, &ProjectPropertiesDialog::OCIOFilenameUpdated);
    OCIOFilenameUpdated();

    tabs->addTab(color_group, tr("Color Management"));
  }

  

  {
    // Paths group
    QWidget* paths_group = new QWidget();

    QGridLayout* paths_layout = new QGridLayout(paths_group);

    cache_path_ = new PathWidget(working_project_->cache_path(), this);
    proxy_path_ = new PathWidget(working_project_->proxy_path(), this);

    int row = 0;

    paths_layout->addWidget(new QLabel(tr("Cache Path:")), row, 0);
    paths_layout->addWidget(cache_path_->path_edit(), row, 1);
    paths_layout->addWidget(cache_path_->browse_btn(), row, 2);
    paths_layout->addWidget(cache_path_->default_box(), row, 3);

    row++;

    paths_layout->addWidget(new QLabel(tr("Proxy Path:")), row, 0);
    paths_layout->addWidget(proxy_path_->path_edit(), row, 1);
    paths_layout->addWidget(proxy_path_->browse_btn(), row, 2);
    paths_layout->addWidget(proxy_path_->default_box(), row, 3);

    tabs->addTab(paths_group, tr("Paths"));
  }

  QDialogButtonBox* dialog_btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal);
  layout->addWidget(dialog_btns);
  connect(dialog_btns, &QDialogButtonBox::accepted, this, &ProjectPropertiesDialog::accept);
  connect(dialog_btns, &QDialogButtonBox::rejected, this, &ProjectPropertiesDialog::reject);
}

void ProjectPropertiesDialog::accept()
{
  if (!ocio_config_is_valid_) {
    QMessageBox mb(this);
    mb.setWindowModality(Qt::WindowModal);
    mb.setIcon(QMessageBox::Critical);
    mb.setWindowTitle(tr("OpenColorIO Config Error"));
    mb.setText(tr("Failed to set OpenColorIO configuration: %1").arg(ocio_config_error_));
    mb.addButton(QMessageBox::Ok);
    mb.exec();
    return;
  }

  if (!cache_path_->PathIsValid(true)) {
    QMessageBox mb(this);
    mb.setWindowModality(Qt::WindowModal);
    mb.setIcon(QMessageBox::Critical);
    mb.setWindowTitle(tr("Invalid path"));
    mb.setText(tr("The cache path is invalid. Please check it and try again."));
    mb.addButton(QMessageBox::Ok);
    mb.exec();
    return;
  }

  if (!proxy_path_->PathIsValid(true)) {
    QMessageBox mb(this);
    mb.setWindowModality(Qt::WindowModal);
    mb.setIcon(QMessageBox::Critical);
    mb.setWindowTitle(tr("Invalid path"));
    mb.setText(tr("The proxy path is invalid. Please check it and try again."));
    mb.addButton(QMessageBox::Ok);
    mb.exec();
    return;
  }

  working_project_->set_cache_path(cache_path_->path_edit()->text());
  working_project_->set_proxy_path(proxy_path_->path_edit()->text());

  // This should ripple changes throughout the program that the color config has changed, therefore must be done last
  working_project_->color_manager()->SetConfigAndDefaultInput(ocio_filename_->text(),
                                                              default_input_colorspace_->currentText());

  QDialog::accept();
}

void ProjectPropertiesDialog::BrowseForOCIOConfig()
{
  QString fn = QFileDialog::getOpenFileName(this, tr("Browse for OpenColorIO configuration"));
  if (!fn.isEmpty()) {
    ocio_filename_->setText(fn);
  }
}

void ProjectPropertiesDialog::OCIOFilenameUpdated()
{
  default_input_colorspace_->clear();

  try {
    OCIO::ConstConfigRcPtr c;

    if (ocio_filename_->text().isEmpty()) {
      c = ColorManager::GetDefaultConfig();
    } else {
      c = ColorManager::CreateConfigFromFile(ocio_filename_->text());
    }

    ocio_filename_->setStyleSheet(QString());
    ocio_config_is_valid_ = true;

    // List input color spaces
    QStringList input_cs = ColorManager::ListAvailableColorspaces(c);

    foreach (QString cs, input_cs) {
      default_input_colorspace_->addItem(cs);

      if (cs == working_project_->color_manager()->GetDefaultInputColorSpace()) {
        default_input_colorspace_->setCurrentIndex(default_input_colorspace_->count()-1);
      }
    }

  } catch (OCIO::Exception& e) {
    ocio_config_is_valid_ = false;
    ocio_filename_->setStyleSheet(QStringLiteral("QLineEdit {color: red;}"));
    ocio_config_error_ = e.what();
  }
}

PathWidget::PathWidget(const QString &path, QWidget *parent) :
  QObject(parent)
{
  path_edit_ = new QLineEdit();
  path_edit_->setText(path);
  connect(path_edit_, &QLineEdit::textChanged, this, &PathWidget::LineEditChanged);

  default_box_ = new QCheckBox(tr("Default"));

  browse_btn_ = new QPushButton(tr("Browse"));

  connect(default_box_, &QCheckBox::toggled, this, &PathWidget::DefaultToggled);

  default_box_->setChecked(path.isEmpty());

  connect(browse_btn_, &QPushButton::clicked, this, &PathWidget::BrowseClicked);
}

bool PathWidget::PathIsValid(bool try_to_create) const
{
  return default_box_->isChecked()
      || QDir(path_edit_->text()).exists()
      || (try_to_create && QDir(path_edit_->text()).mkpath(QStringLiteral(".")));
}

void PathWidget::DefaultToggled(bool e)
{
  path_edit_->setEnabled(!e);
}

void PathWidget::BrowseClicked()
{
  QString dir = QFileDialog::getExistingDirectory(static_cast<QWidget*>(parent()),
                                                  tr("Browse for path"),
                                                  path_edit_->text());

  if (!dir.isEmpty()) {
    path_edit_->setText(dir);
  }
}

void PathWidget::LineEditChanged()
{
  if (PathIsValid(false)) {
    path_edit_->setStyleSheet(QString());
  } else {
    path_edit_->setStyleSheet(QStringLiteral("QLineEdit {color: red;}"));
  }
}

OLIVE_NAMESPACE_EXIT
