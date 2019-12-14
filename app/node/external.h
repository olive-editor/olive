#ifndef EXTERNALNODE_H
#define EXTERNALNODE_H

#include <QXmlStreamReader>

#include "node.h"

/**
 * @brief A node generated from an external XML metadata file
 */
class ExternalNode : public Node
{
public:
  ExternalNode(const QString& xml_meta_filename);

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual bool IsAccelerated() const override;
  virtual QString AcceleratedCodeVertex() const override;
  virtual QString AcceleratedCodeFragment() const override;
  virtual int AcceleratedCodeIterations() const override;
  virtual NodeInput* AcceleratedCodeIterativeInput() const override;

private:
  void XMLReadLanguageString(QXmlStreamReader* reader, QMap<QString, QString>* map);
  void XMLReadEffect(QXmlStreamReader *reader);
  void XMLReadIterations(QXmlStreamReader* reader);
  void XMLReadParam(QXmlStreamReader* reader);
  void XMLReadShader(QXmlStreamReader* reader, QString& destination);

  static QString GetStringForCurrentLanguage(const QMap<QString, QString> *language_map);

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
};

#endif // EXTERNALNODE_H
