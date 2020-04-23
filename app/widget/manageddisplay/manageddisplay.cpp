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

ManagedDisplayObject::ManagedDisplayObject(QWidget *parent) :
  QOpenGLWidget(parent),
  color_manager_(nullptr),
  color_service_(nullptr)
{
}

void ManagedDisplayObject::ConnectColorManager(ColorManager *color_manager)
{
  if (color_manager_ == color_manager) {
    return;
  }

  if (color_manager_ != nullptr) {
    disconnect(color_manager_, &ColorManager::ConfigChanged, this, &ManagedDisplayObject::ColorConfigChanged);
  }

  color_manager_ = color_manager;

  if (color_manager_ != nullptr) {
    connect(color_manager_, &ColorManager::ConfigChanged, this, &ManagedDisplayObject::ColorConfigChanged);
  }

  ColorConfigChanged();
}

ColorManager *ManagedDisplayObject::color_manager() const
{
  return color_manager_;
}

void ManagedDisplayObject::DisconnectColorManager()
{
  ConnectColorManager(nullptr);
}

const ColorTransform &ManagedDisplayObject::GetColorTransform() const
{
  return color_transform_;
}

void ManagedDisplayObject::ColorConfigChanged()
{
  if (!color_manager_) {
    color_service_ = nullptr;
    return;
  }

  SetColorTransform(color_manager_->GetCompliantColorSpace(color_transform_, true));
}

OpenGLColorProcessorPtr ManagedDisplayObject::color_service()
{
  return color_service_;
}

void ManagedDisplayObject::ContextCleanup()
{
  makeCurrent();

  color_service_ = nullptr;

  doneCurrent();
}

void ManagedDisplayObject::MenuDisplaySelect(QAction *action)
{
  const ColorTransform& old_transform = GetColorTransform();

  ColorTransform new_transform = color_manager()->GetCompliantColorSpace(ColorTransform(action->data().toString(),
                                                                                                              old_transform.view(),
                                                                                                              old_transform.look()));

  SetColorTransform(new_transform);
}

void ManagedDisplayObject::SetColorTransform(const ColorTransform &transform)
{
  color_transform_ = transform;
  SetupColorProcessor();
  update();
}

void ManagedDisplayObject::initializeGL()
{
  SetupColorProcessor();

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ManagedDisplayObject::ContextCleanup, Qt::DirectConnection);
}

Menu* ManagedDisplayObject::GetDisplayMenu(QMenu* parent, bool auto_connect)
{
  QStringList displays = color_manager()->ListAvailableDisplays();

  Menu* ocio_display_menu = new Menu(tr("Display"), parent);

  foreach (const QString& d, displays) {
    QAction* action = ocio_display_menu->addAction(d);
    action->setCheckable(true);
    action->setChecked(color_transform_.display() == d);
    action->setData(d);
  }

  return ocio_display_menu;
}

Menu* ManagedDisplayObject::GetViewMenu(QMenu* parent)
{
  QStringList views = color_manager()->ListAvailableViews(color_transform_.display());

  Menu* ocio_view_menu = new Menu(tr("View"), parent);

  foreach (const QString& v, views) {
    QAction* action = ocio_view_menu->addAction(v);
    action->setCheckable(true);
    action->setChecked(color_transform_.view() == v);
    action->setData(v);
  }

  return ocio_view_menu;
}

Menu* ManagedDisplayObject::GetLookMenu(QMenu* parent, bool auto_connect)
{
  QStringList looks = color_manager()->ListAvailableLooks();

  Menu* ocio_look_menu = new Menu(tr("Look"), parent);

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

void ManagedDisplayObject::SetupColorProcessor()
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

      makeCurrent();
      color_service_->Enable(context(), true);
      doneCurrent();

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
