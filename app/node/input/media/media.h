#ifndef IMAGE_H
#define IMAGE_H

#include <QOpenGLTexture>

#include "node/node.h"

/**
 * @brief A node that imports an image
 *
 * FIXME: This will likely be replaced by the Media node as the Media node will be set up to pull from various decoders
 *        from the beginning.
 */
class MediaInput : public Node
{
  Q_OBJECT
public:
  MediaInput();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  NodeOutput* texture_output();

public slots:
  virtual void Process(const rational &time) override;

private:
  NodeInput* footage_input_;

  NodeOutput* texture_output_;

  QOpenGLTexture* texture_;
};

#endif // IMAGE_H
