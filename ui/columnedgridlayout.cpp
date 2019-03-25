#include "columnedgridlayout.h"

ColumnedGridLayout::ColumnedGridLayout(QWidget* parent,
                                       int maximum_columns) :
  QGridLayout (parent),
  maximum_columns_(maximum_columns)
{
}

void ColumnedGridLayout::Add(QWidget *widget)
{
  if (maximum_columns_ > 0) {

    int row = count() / maximum_columns_;
    int column = count() % maximum_columns_;

    addWidget(widget, row, column);

  } else {

    addWidget(widget);

  }
}

int ColumnedGridLayout::MaximumColumns() const
{
  return maximum_columns_;
}

void ColumnedGridLayout::SetMaximumColumns(int maximum_columns)
{
  maximum_columns_ = maximum_columns;
}
