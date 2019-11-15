#include "audiorenderworker.h"

#include "audio/audiomanager.h"

AudioRenderWorker::AudioRenderWorker(DecoderCache *decoder_cache, QObject *parent) :
  RenderWorker(decoder_cache, parent)
{
}

void AudioRenderWorker::SetParameters(const AudioRenderingParams &audio_params)
{
  audio_params_ = audio_params;
}

void AudioRenderWorker::RenderAsSibling(NodeDependency dep)
{
  NodeOutput* output = dep.node();
  Node* node = output->parent();
  QList<NodeInput*> connected_inputs;
  QVariant value;

  // Set working state
  working_++;

  bool locked = false;

  // Firstly we check if this node is a "Block", if it is that means it's part of a linked list of mutually exclusive
  // nodes based on time and we might need to locate which Block to attach to
  if (node->IsBlock()
      && (dep.range().in() < static_cast<Block*>(node)->in()
          || dep.range().out() > static_cast<Block*>(node)->out())) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    value = RenderBlock(output, dep.range());
  } else {
    node->LockProcessing();
    value = ProcessNodeNormally(NodeDependency(output, dep.range()));
    locked = true;
  }

  if (!locked) {
    node->LockProcessing();
  }

  // Place the value into the output
  output->cache_value(dep.range(), value);

  // We're done!
  node->UnlockProcessing();

  // End this working state
  working_--;
}

bool AudioRenderWorker::InitInternal()
{
  // Nothing to init yet
  return true;
}

void AudioRenderWorker::CloseInternal()
{
  // Nothing to init yet
}

QVariant AudioRenderWorker::RenderBlock(NodeOutput* output, const TimeRange &range)
{
  QList<Block*> active_blocks = ValidateBlockRange(static_cast<Block*>(output->parent()), range);

  // All these blocks will need to output to a buffer so we create one here
  QByteArray block_range_buffer(audio_params_.time_to_bytes(range.length()), 0);

  // Loop through active blocks retrieving their audio
  while (!active_blocks.isEmpty()) {
    int block_for_this_thread = -1;

    for (int i=0;i<active_blocks.size();i++) {
      Block* b = active_blocks.at(i);
      NodeOutput* connected_output = static_cast<NodeOutput*>(b->GetParameterWithID(output->id()));

      TimeRange range_for_block(qMax(b->in(), range.in()),
                                qMin(b->out(), range.out()));

      // If the block is locked, we assume another thread has it. Otherwise, we'll work with it
      if (connected_output->has_cached_value(range_for_block)) {
        // This output already has this value, no need to process it again
        QByteArray samples_from_this_block = connected_output->get_cached_value(range_for_block).toByteArray();

        int destination_offset = audio_params_.time_to_bytes(range_for_block.in() - range.in());

        memcpy(block_range_buffer.data()+destination_offset,
               samples_from_this_block.data(),
               static_cast<size_t>(samples_from_this_block.size()));

        active_blocks.removeAt(i);
        i--;
      } else if (!b->IsProcessingLocked()) {
        if (block_for_this_thread == -1) {
          block_for_this_thread = i;
        } else {
          emit RequestSibling(NodeDependency(connected_output,
                                             range_for_block));
        }
      }
    }

    if (block_for_this_thread > -1) {
      Block* b = active_blocks.at(block_for_this_thread);
      TimeRange range_for_block(qMax(b->in(), range.in()),
                                qMin(b->out(), range.out()));

      RenderAsSibling(NodeDependency(static_cast<NodeOutput*>(b->GetParameterWithID(output->id())),
                                     range_for_block));
    } else {
      QThread::msleep(500);
    }
  }

  return block_range_buffer;
}
