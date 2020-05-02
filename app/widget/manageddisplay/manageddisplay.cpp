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

#include "manageddisplay.h"

#include <QMessageBox>

OLIVE_NAMESPACE_ENTER

ManagedDisplayWidget::ManagedDisplayWidget(QWidget *parent) :
  QOpenGLWidget(parent),
  color_manager_(nullptr),
  color_service_(nullptr)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
}

ManagedDisplayWidget::~ManagedDisplayWidget()
{
  ContextCleanup();
}

void ManagedDisplayWidget::ConnectColorManager(ColorManager *color_manager)
{
  if (color_manager_ == color_manager) {
    return;
  }

  if (color_manager_ != nullptr) {
    disconnect(color_manager_, &ColorManager::ConfigChanged, this, &ManagedDisplayWidget::ColorConfigChanged);
  }

  color_manager_ = color_manager;

  if (color_manager_ != nullptr) {
    connect(color_manager_, &ColorManager::ConfigChanged, this, &ManagedDisplayWidget::ColorConfigChanged);
  }

  ColorConfigChanged();
  emit ColorManagerChanged(color_manager_);
}

ColorManager *ManagedDisplayWidget::color_manager() const
{
  return color_manager_;
}

void ManagedDisplayWidget::DisconnectColorManager()
{
  ConnectColorManager(nullptr);
}

const ColorTransform &ManagedDisplayWidget::GetColorTransform() const
{
  return color_transform_;
}

void ManagedDisplayWidget::ColorConfigChanged()
{
  if (!color_manager_) {
    color_service_ = nullptr;
    return;
  }

  SetColorTransform(color_manager_->GetCompliantColorSpace(color_transform_, true));
}

OpenGLColorProcessorPtr ManagedDisplayWidget::color_service()
{
  return color_service_;
}

void ManagedDisplayWidget::ContextCleanup()
{
  makeCurrent();

  color_service_ = nullptr;

  doneCurrent();
}

void ManagedDisplayWidget::ShowDefaultContextMenu()
{
  Menu m(this);

  if (color_manager_) {
    m.addMenu(GetDisplayMenu(&m));
    m.addMenu(GetViewMenu(&m));
    m.addMenu(GetLookMenu(&m));
  } else {
    QAction* a = m.addAction(tr("No color manager connected"));
    a->setEnabled(false);
  }

  m.exec(QCursor::pos());
}

void ManagedDisplayWidget::MenuDisplaySelect(QAction *action)
{
  const ColorTransform& old_transform = GetColorTransform();

  ColorTransform new_transform = color_manager()->GetCompliantColorSpace(ColorTransform(action->data().toString(),
                                                                                        old_transform.view(),
                                                                                        old_transform.look()));

  SetColorTransform(new_transform);
}

void ManagedDisplayWidget::MenuViewSelect(QAction *action)
{
  const ColorTransform& old_transform = GetColorTransform();

  ColorTransform new_transform = color_manager()->GetCompliantColorSpace(ColorTransform(old_transform.display(),
                                                                                        action->data().toString(),
                                                                                        old_transform.look()));

  SetColorTransform(new_transform);
}

void ManagedDisplayWidget::MenuLookSelect(QAction *action)
{
  const ColorTransform& old_transform = GetColorTransform();

  ColorTransform new_transform = color_manager()->GetCompliantColorSpace(ColorTransform(old_transform.display(),
                                                                                        old_transform.view(),
                                                                                        action->data().toString()));

  SetColorTransform(new_transform);
}

void ManagedDisplayWidget::SetColorTransform(const ColorTransform &transform)
{
  makeCurrent();

  color_transform_ = transform;
  SetupColorProcessor();
  ColorProcessorChangedEvent();

  doneCurrent();
}

void ManagedDisplayWidget::initializeGL()
{
  SetupColorProcessor();

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ManagedDisplayWidget::ContextCleanup, Qt::DirectConnection);
}

void ManagedDisplayWidget::EnableDefaultContextMenu()
{
  connect(this, &ManagedDisplayWidget::customContextMenuRequested, this, &ManagedDisplayWidget::ShowDefaultContextMenu);
}

void ManagedDisplayWidget::ColorProcessorChangedEvent()
{
  update();
}

Menu* ManagedDisplayWidget::GetDisplayMenu(QMenu* parent, bool auto_connect)
{
  QStringList displays = color_manager()->ListAvailableDisplays();

  Menu* ocio_display_menu = new Menu(tr("Display"), parent);

  if (auto_connect) {
    connect(ocio_display_menu, &Menu::triggered, this, &ManagedDisplayWidget::MenuDisplaySelect);
  }

  foreach (const QString& d, displays) {
    QAction* action = ocio_display_menu->addAction(d);
    action->setCheckable(true);
    action->setChecked(color_transform_.display() == d);
    action->setData(d);
  }

  return ocio_display_menu;
}

Menu* ManagedDisplayWidget::GetViewMenu(QMenu* parent, bool auto_connect)
{
  QStringList views = color_manager()->ListAvailableViews(color_transform_.display());

  Menu* ocio_view_menu = new Menu(tr("View"), parent);

  if (auto_connect) {
    connect(ocio_view_menu, &Menu::triggered, this, &ManagedDisplayWidget::MenuViewSelect);
  }

  foreach (const QString& v, views) {
    QAction* action = ocio_view_menu->addAction(v);
    action->setCheckable(true);
    action->setChecked(color_transform_.view() == v);
    action->setData(v);
  }

  return ocio_view_menu;
}

Menu* ManagedDisplayWidget::GetLookMenu(QMenu* parent, bool auto_connect)
{
  QStringList looks = color_manager()->ListAvailableLooks();

  Menu* ocio_look_menu = new Menu(tr("Look"), parent);

  if (auto_connect) {
    connect(ocio_look_menu, &Menu::triggered, this, &ManagedDisplayWidget::MenuLookSelect);
  }

  // Setup "no look" action
  QAction* no_look_action = ocio_look_menu->addAction(tr("(None)"));
  no_look_action->setCheckable(true);
  no_look_action->setChecked(color_transform_.look().isEmpty());
  no_look_action->setData(QString());

  // Set up the rest of the looks
  foreach (const QString& l, looks) {
    QAction* action = ocio_look_menu->addAction(l);
    action->setCheckable(true);
    action->setChecked(color_transform_.look() == l);
    action->setData(l);
  }

  return ocio_look_menu;
}

void ManagedDisplayWidget::SetupColorProcessor()
{
  if (!context()) {
    return;
  }

  color_service_ = nullptr;

  if (color_manager_) {
    // (Re)create color processor

    try {

      color_service_ = OpenGLColorProcessor::Create(color_manager_,
                                                    color_manager_->GetReferenceColorSpace(),
                                                    color_transform_);

      color_service_->Enable(context(), true);

    } catch (OCIO::Exception& e) {

      QMessageBox::critical(this,
                            tr("OpenColorIO Error"),
                            tr("Failed to set color configuration: %1").arg(e.what()),
                            QMessageBox::Ok);

    }

  } else {
    color_service_ = nullptr;
  }

  emit ColorProcessorChanged(std::static_pointer_cast<ColorProcessor>(color_service_));
}

OLIVE_NAMESPACE_EXIT
