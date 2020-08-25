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

#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include <QStringList>

#include "common/define.h"

class CommandLineParser
{
public:
  ~CommandLineParser();

  DISABLE_COPY_MOVE(CommandLineParser)

  class Option {
  public:
    Option()
    {
      is_set_ = false;
    }

    bool IsSet() const
    {
      return is_set_;
    }

    void Set()
    {
      is_set_ = true;
    }

  private:
    bool is_set_;

  };

  class PositionalArgument
  {
  public:
    PositionalArgument() = default;

    const QString& GetSetting() const
    {
      return setting_;
    }

    void SetSetting(const QString& s)
    {
      setting_ = s;
    }

  private:
    QString setting_;

  };

  CommandLineParser() = default;

  const Option* AddOption(const QStringList& strings, const QString& description);

  const PositionalArgument* AddPositionalArgument(const QString& name, const QString& description, bool required = false);

  void Process(int argc, char** argv);

  void PrintHelp(const char* filename);

private:
  struct KnownOption {
    QStringList args;
    QString description;
    Option* option;
  };

  struct KnownPositionalArgument {
    QString name;
    QString description;
    PositionalArgument* option;
    bool required;
  };

  QVector<KnownOption> options_;

  QVector<KnownPositionalArgument> positional_args_;

};

#endif // COMMANDLINEPARSER_H
