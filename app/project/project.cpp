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

  // FIXME: Test code
  Folder* f1 = new Folder();
  f1->set_name("F1");
  f1->set_parent(&root_);
  Folder* f2 = new Folder();
  f2->set_name("F2");
  f2->set_parent(f1);
  Folder* f3 = new Folder();
  f3->set_name("F3");
  f3->set_parent(&root_);
  Folder* f4 = new Folder();
  f4->set_name("F4");
  f4->set_parent(f1);
  Folder* f5 = new Folder();
  f5->set_name("F5");
  f5->set_parent(f3);
  // End Test code
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
