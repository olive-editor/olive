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

#include "commandlineparser.h"

#include <QCoreApplication>
#include <QDebug>

CommandLineParser::~CommandLineParser()
{
  foreach (const KnownOption& o, options_) {
    delete o.option;
  }

  foreach (const KnownPositionalArgument& a, positional_args_) {
    delete a.option;
  }
}

const CommandLineParser::Option *CommandLineParser::AddOption(const QStringList &strings, const QString &description, bool takes_arg, const QString &arg_placeholder)
{
  Option* o = new Option();

  options_.append({strings, description, o, takes_arg, arg_placeholder});

  return o;
}

const CommandLineParser::PositionalArgument *CommandLineParser::AddPositionalArgument(const QString &name, const QString &description, bool required)
{
  PositionalArgument* a = new PositionalArgument();

  positional_args_.append({name, description, a, required});

  return a;
}

void CommandLineParser::Process(int argc, char **argv)
{
  int positional_index = 0;

  for (int i=1; i<argc; i++) {
    if (argv[i][0] == '-') {
      // Must be an option

      // Skip past first dashes
      const char* arg_basename = &argv[i][1];

      bool matched_known = false;

      for (int j=0; j<options_.size(); j++) {
        KnownOption& o = options_[j];

        foreach (const QString& s, o.args) {
          if (!s.compare(arg_basename, Qt::CaseInsensitive)) {
            // Flag discovered!
            o.option->Set();

            if (o.takes_arg && i+1 < argc) {
              o.option->SetSetting(argv[i+1]);
              i++;
            }

            matched_known = true;
            goto found_flag;
          }
        }
      }

found_flag:
      if (!matched_known) {
        qWarning() << "Unknown parameter:" << argv[i];
      }

    } else {
      // Must be a positional flag
      if (positional_index < positional_args_.size()) {
        positional_args_[positional_index].option->SetSetting(argv[i]);
        positional_index++;
      } else {
        qWarning() << "Unknown parameter:" << argv[i];
      }
    }
  }
}

void CommandLineParser::PrintHelp(const char* filename)
{
  printf("%s %s\n",
         QCoreApplication::applicationName().toUtf8().constData(),
         QCoreApplication::applicationVersion().toUtf8().constData());

  printf("Copyright (C) 2018-2020 Olive Team\n");

  QString positional_args;
  for (int i=0; i<positional_args_.size(); i++) {
    if (i > 0) {
      positional_args.append(' ');
    }

    positional_args.append('[');
    positional_args.append(positional_args_.at(i).name);
    positional_args.append(']');
  }

  const char* basename;
#ifdef Q_OS_WINDOWS
  basename = strrchr(filename, '\\');
  if (!basename) {
    basename = strrchr(filename, '/');
  }
#else
  basename = strrchr(filename, '/');
#endif

  if (basename) {
    // Slash found, increment pointer to avoid showing the slash itself
    basename++;
  } else {
    // If no slashes are found, assume string is already a basename
    basename = filename;
  }

  printf("Usage: %s [options] %s\n\n", basename, positional_args.toUtf8().constData());
  foreach (const KnownOption& o, options_) {
    QString all_args;

    for (int i=0; i<o.args.size(); i++) {
      if (i > 0) {
        all_args.append(QStringLiteral(", "));
      }

      const QString& this_arg = o.args.at(i);

      all_args.append('-');
      all_args.append(this_arg);
    }

    if (o.arg_placeholder.isEmpty()) {
      printf("    %s\n", all_args.toUtf8().constData());
    } else {
      printf("    %s <%s>\n", all_args.toUtf8().constData(), o.arg_placeholder.toUtf8().constData());
    }

    printf("        %s\n\n", o.description.toUtf8().constData());
  }

  printf("\n");
}
