#ifndef COLUMNEDGRIDLAYOUT_H
#define COLUMNEDGRIDLAYOUT_H

#include <QGridLayout>

/**
 * @brief The ColumnedGridLayout class
 *
 * A simple derivative of QGridLayout that provides a automatic row/column layout based on a specified maximum
 * column count.
 */
class ColumnedGridLayout : public QGridLayout
{
  Q_OBJECT
public:
  ColumnedGridLayout(QWidget* parent = nullptr,
                     int maximum_columns = 0);

  void Add(QWidget* widget);
  int MaximumColumns() const;
  void SetMaximumColumns(int maximum_columns);

private:
  int maximum_columns_;
};

#endif // COLUMNEDGRIDLAYOUT_H
