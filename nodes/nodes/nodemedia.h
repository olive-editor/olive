#ifndef MEDIANODE_H
#define MEDIANODE_H

#include "nodes/node.h"
#include "rendering/memorycache.h"
#include "project/media.h"

/**
 * @brief The NodeMedia class
 *
 * The NodeMedia node is the main entry point for external video/audio files into the Olive pipeline. Its
 * responsibilities are as follows:
 *
 * * Use a Decoder to retrieve the correct raw frame data
 * * Convert the frame data to the pipeline format (usually RGBA16F or RGBA32F)
 * * Convert the frame to scene linear colorspace with OpenColorIO
 *
 * Naturally this Node is resource intensive and effort should be spent here to make this as performant as possible.
 */
class NodeMedia : public Node
{
  Q_OBJECT
public:
  NodeMedia(NodeGraph *c);

  virtual QString name() override;
  virtual QString id() override;

  virtual void Process(const rational& time) override;

  void SetMedia(Media* f);

  NodeIO* matrix_input();
  NodeIO* texture_output();

private:
  // Matrix input for the transformation to apply to this media (if any)
  NodeIO* matrix_input_;

  // Texture output
  NodeIO* texture_output_;

  // Media object to display
  Media* media_;

  // Texture buffer
  // TODO Probable cache point
  MemoryCache::Reference buffer_;
};

#endif // MEDIANODE_H
