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

#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <memory>

#include "project/item/folder/folder.h"

/**
 * @brief A project instance containing all the data pertaining to the user's project
 *
 * A project instance uses a parent-child hierarchy of Item objects. Projects will usually contain the following:
 *
 * * Footage
 * * Sequences
 * * Folders
 * * Project Settings
 * * Window Layout
 */
class Project : public QObject
{
  Q_OBJECT
public:
  Project();

  Folder* root();

  const QString& name();
  void set_name(const QString& s);

  const QString& ocio_config();
  void set_ocio_config(const QString& ocio_config);

  const QString& default_input_colorspace();
  void set_default_input_colorspace(const QString& colorspace);

private:
  Folder root_;

  QString name_;

  QString ocio_config_;
  QString default_input_colorspace_;
};

using ProjectPtr = std::shared_ptr<Project>;

#endif // PROJECT_H
