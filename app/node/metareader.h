#ifndef NODEMETAREADER_H
#define NODEMETAREADER_H

#include <QMap>
#include <QString>
#include <QXmlStreamReader>

#include "input.h"

class NodeMetaReader
{
public:
  NodeMetaReader(const QString& xml_meta_filename);

  QString Name() const;
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
  void XMLReadLanguageString(QXmlStreamReader* reader, QMap<QString, QString>* map);
  void XMLReadEffect(QXmlStreamReader *reader);
  void XMLReadIterations(QXmlStreamReader* reader);
  void XMLReadParam(QXmlStreamReader* reader);
  void XMLReadShader(QXmlStreamReader* reader, QString& destination);

  static QString GetStringForCurrentLanguage(const QMap<QString, QString> *language_map);

  NodeInput* GetInputWithID(const QString& id) const;

  QString xml_filename_;

  QMap<QString, QString> names_;
  QMap<QString, QString> descriptions_;
  QMap<QString, QString> categories_;
  QMap<QString, QMap<QString, QString> > param_names_;

  QString id_;

  QString frag_code_;
  QString vert_code_;

  int iterations_;
  NodeInput* iteration_input_;

  QList<NodeInput*> inputs_;
};

#endif // NODEMETAREADER_H
