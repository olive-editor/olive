#ifndef MULTICAMNODE_H
#define MULTICAMNODE_H

#include "node/node.h"
#include "node/output/track/tracklist.h"

namespace olive {

class Sequence;

class MultiCamNode : public Node
{
  Q_OBJECT
public:
  MultiCamNode();

  NODE_DEFAULT_FUNCTIONS(MultiCamNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual ActiveElements GetActiveElementsAtTime(const QString &input, const TimeRange &r) const override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void Retranslate() override;

  static const QString kCurrentInput;
  static const QString kSourcesInput;
  static const QString kSequenceInput;
  static const QString kSequenceTypeInput;

  int GetCurrentSource() const
  {
    return GetStandardValue(kCurrentInput).toInt();
  }

  int GetSourceCount() const;

  static void GetRowsAndColumns(int sources, int *rows, int *cols);
  void GetRowsAndColumns(int *rows, int *cols) const
  {
    return GetRowsAndColumns(GetSourceCount(), rows, cols);
  }

  void SetSequenceType(Track::Type t)
  {
    SetStandardValue(kSequenceTypeInput, t);
  }

  static void IndexToRowCols(int index, int total_rows, int total_cols, int *row, int *col);

  static int RowsColsToIndex(int row, int col, int total_rows, int total_cols)
  {
    return col + row * total_cols;
  }

  virtual Node *GetConnectedRenderOutput(const QString& input, int element = -1) const override;
  virtual bool IsInputConnectedForRender(const QString& input, int element = -1) const override;

  virtual QVector<QString> IgnoreInputsForRendering() const override;

protected:
  virtual void InputConnectedEvent(const QString &input, int element, Node *output) override;
  virtual void InputDisconnectedEvent(const QString &input, int element, Node *output) override;

private:
  TrackList *GetTrackList() const;

  Sequence *sequence_;

};

}

#endif // MULTICAMNODE_H
