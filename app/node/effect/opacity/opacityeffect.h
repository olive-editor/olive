#ifndef OPACITYEFFECT_H
#define OPACITYEFFECT_H

#include "node/group/group.h"

namespace olive {

class OpacityEffect : public Node
{
public:
  OpacityEffect();

  NODE_DEFAULT_FUNCTIONS(OpacityEffect)

  virtual QString Name() const override
  {
    return tr("Opacity");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.opacity");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryFilter};
  }

  virtual QString Description() const override
  {
    return tr("Alter a video's opacity.\n\nThis is equivalent to multiplying a video by a number between 0.0 and 1.0.");
  }

  virtual void Retranslate() override;

  virtual value_t Value(const ValueParams &p) const override;

  static const QString kTextureInput;
  static const QString kValueInput;

};

}

#endif // OPACITYEFFECT_H
