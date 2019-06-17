#include "core.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "ui/icons/icons.h"
#include "ui/style/style.h"

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
  // Start GUI
  //

  // Load all icons
  olive::icon::LoadAll();

  // Set UI style
  olive::style::SetDefault();

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
