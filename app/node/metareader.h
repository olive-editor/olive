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

#ifndef NODEMETAREADER_H
#define NODEMETAREADER_H

#include <QMap>
#include <QString>
#include <QXmlStreamReader>

#include "input.h"

OLIVE_NAMESPACE_ENTER

class NodeMetaReader
{
public:
  NodeMetaReader(const QString& xml_meta_filename);

  QString Name() const;
  QString ShortName() const;
  const QString& id() const;
  QString Category() const;
  QString Description() const;

  const QString& filename() const;

  const QString& frag_code() const;
  const QString& vert_code() const;

  const int& iterations() const;
  NodeInput* iteration_input() const;

  const QList<NodeInput*>& inputs() const;

  void Retranslate();

private:
  using LanguageMap = QMap<QString, QString>;

  void XMLReadLanguageString(QXmlStreamReader* reader, LanguageMap *map);
  void XMLReadEffect(QXmlStreamReader *reader);
  void XMLReadIterations(QXmlStreamReader* reader);
  void XMLReadParam(QXmlStreamReader* reader);
  void XMLReadShader(QXmlStreamReader* reader, QString& destination);

  static QString GetStringForCurrentLanguage(const LanguageMap *language_map);

  NodeInput* GetInputWithID(const QString& id) const;

  QString xml_filename_;

  LanguageMap names_;
  LanguageMap short_names_;
  LanguageMap descriptions_;
  LanguageMap categories_;
  QMap<QString, LanguageMap > param_names_;
  QMap<QString, QList<LanguageMap> > combo_names_;
  QMap<QString, QList<LanguageMap> > combo_descriptions_;

  QString id_;

  QString frag_code_;
  QString vert_code_;

  int iterations_;
  NodeInput* iteration_input_;

  QList<NodeInput*> inputs_;
};

OLIVE_NAMESPACE_EXIT

#endif // NODEMETAREADER_H
