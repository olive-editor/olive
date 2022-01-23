#ifndef OPACITYEFFECT_H
#define OPACITYEFFECT_H

#include "node/group/group.h"

namespace olive {

class OpacityEffect : public NodeGroup
{
public:
  OpacityEffect();

  NODE_DEFAULT_DESTRUCTOR(OpacityEffect)

  NODE_COPY_FUNCTION(OpacityEffect)

  virtual QString Name() const override
  {
    return tr("Opacity");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.opacityeffect");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryFilter, kCategoryVideoEffect};
  }

  virtual QString Description() const override
  {
    return tr("Alter a video's opacity.\n\nThis is equivalent to multiplying a video by a number between 0.0 and 1.0.");
  }

  virtual void Retranslate() override;

private:
  QString tex_in_pass_;
  QString value_in_pass_;

};

}

#endif // OPACITYEFFECT_H
