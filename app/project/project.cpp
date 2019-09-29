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

#include "project.h"

Project::Project()
{
  name_ = tr("(untitled)");
}

Folder *Project::root()
{
  return &root_;
}

const QString &Project::name()
{
  return name_;
}

void Project::set_name(const QString &s)
{
  name_ = s;
}

const QString &Project::ocio_config()
{
  return ocio_config_;
}

void Project::set_ocio_config(const QString &ocio_config)
{
  ocio_config_ = ocio_config;
}

const QString &Project::default_input_colorspace()
{
  return default_input_colorspace_;
}

void Project::set_default_input_colorspace(const QString &colorspace)
{
  default_input_colorspace_ = colorspace;
}
