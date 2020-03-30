#ifndef NODECOPYPASTEWIDGET_H
#define NODECOPYPASTEWIDGET_H

#include <QWidget>
#include <QUndoCommand>

#include "node/node.h"
#include "project/item/sequence/sequence.h"

class NodeCopyPasteWidget
{
public:
  NodeCopyPasteWidget() = default;

protected:
  void CopyNodesToClipboard(const QList<Node*>& nodes, void* userdata = nullptr);

  QList<Node*> PasteNodesFromClipboard(Sequence *graph, QUndoCommand *command, void* userdata = nullptr);

  virtual void CopyNodesToClipboardInternal(QXmlStreamWriter *writer, void* userdata);

  virtual void PasteNodesFromClipboardInternal(QXmlStreamReader *reader, void* userdata);

};

#endif // NODECOPYPASTEWIDGET_H
