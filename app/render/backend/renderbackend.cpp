#include "renderbackend.h"

RenderBackend::RenderBackend() :
  viewer_node_(nullptr)
{

}

RenderBackend::~RenderBackend()
{
}

const QString &RenderBackend::GetError()
{
  return error_;
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ != nullptr) {
    disconnect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(Compile()));
  }

  viewer_node_ = viewer_node;

  if (viewer_node_ != nullptr) {
    connect(viewer_node_, SIGNAL(TextureChangedBetween(const rational&, const rational&)), this, SLOT(Compile()));
  }

  Decompile();
}

void RenderBackend::InvalidateCache(const rational &start_range, const rational &end_range)
{
  if (!params_.is_valid()) {
      return;
    }

    // Adjust range to min/max values
    rational start_range_adj = qMax(rational(0), start_range);
    rational end_range_adj = qMin(viewer_node_->Length(), end_range);

    qDebug() << "Cache invalidated between"
             << start_range_adj.toDouble()
             << "and"
             << end_range_adj.toDouble();

    // Snap start_range to timebase
    double start_range_dbl = start_range_adj.toDouble();
    double start_range_numf = start_range_dbl * static_cast<double>(params_.time_base().denominator());
    int64_t start_range_numround = qFloor(start_range_numf/static_cast<double>(params_.time_base().numerator())) * params_.time_base().numerator();
    rational true_start_range(start_range_numround, params_.time_base().denominator());

    for (rational r=true_start_range;r<=end_range_adj;r+=params_.time_base()) {
      // Try to order the queue from closest to the playhead to furthest
      rational last_time = last_time_requested_;

      rational diff = r - last_time;

      if (diff < 0) {
        // FIXME: Hardcoded number
        // If the number is before the playhead, we still prioritize its closeness but not nearly as much (5:1 in this
        // example)
        diff = qAbs(diff) * 5;
      }

      bool contains = false;
      bool added = false;
      QLinkedList<rational>::iterator insert_iterator;

      for (QLinkedList<rational>::iterator i = cache_queue_.begin();i != cache_queue_.end();i++) {
        rational compare = *i;

        if (!added) {
          rational compare_diff = compare - last_time;

          if (compare_diff > diff) {
            insert_iterator = i;
            added = true;
          }
        }

        if (compare == r) {
          contains = true;
          break;
        }
      }

      if (!contains) {
        if (added) {
          cache_queue_.insert(insert_iterator, r);
        } else {
          cache_queue_.append(r);
        }
      }
    }

    CacheNext();
}

void RenderBackend::SetError(const QString &error)
{
  error_ = error;
}

ViewerOutput *RenderBackend::viewer_node() const
{
  return viewer_node_;
}
