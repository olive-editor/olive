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

#include "core.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "ui/icons/icons.h"
#include "ui/style/style.h"
#include "widget/menu/menushared.h"

Core olive::core;

Core::Core() :
  main_window_(nullptr)
{
}

void Core::Start()
{
  //
  // Parse command line arguments
  //

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  // Project from command line option
  // FIXME: What's the correct way to make an "optional" positional argument?
  parser.addPositionalArgument("[project]", tr("Project to open on startup"));

  // Create fullscreen option
  QCommandLineOption fullscreen_option({"f", "fullscreen"}, tr("Start in full screen mode"));
  parser.addOption(fullscreen_option);

  // Parse options
  parser.process(*QApplication::instance());

  QStringList args = parser.positionalArguments();

  // Detect project to load on startup
  if (!args.isEmpty()) {
    startup_project_ = args.first();
  }



  //
  // Start GUI (TODO CLI mode)
  //

  // Load all icons
  olive::icon::LoadAll();

  // Set UI style
  olive::style::AppSetDefault();

  // Set up shared menus
  olive::menu_shared.Initialize();

  // Create main window and open it
  main_window_ = new olive::MainWindow();
  if (parser.isSet(fullscreen_option)) {
    main_window_->showFullScreen();
  } else {
    main_window_->showMaximized();
  }
}

void Core::Stop()
{
  delete main_window_;
}

olive::MainWindow *Core::main_window()
{
  return main_window_;
}
