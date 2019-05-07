#ifndef NODESHADER_H
#define NODESHADER_H

#include "nodes/oldeffectnode.h"

class NodeShader : public OldEffectNode {
  Q_OBJECT
public:
  NodeShader(Clip *c,
             const QString& name,
             const QString& id,
             const QString& category,
             const QString& filename);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual bool IsCreatable() override;
  virtual OldEffectNodePtr Create(Clip *c) override;

private:
  QString name_;
  QString id_;
  QString category_;
  QString description_;
  QString filename_;
};

#endif // NODESHADER_H
