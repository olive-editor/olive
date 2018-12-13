#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QAbstractItemModel>

class Media;

class ProjectModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ProjectModel(QObject* parent = 0);
    ~ProjectModel() override;

    void destroy_root();
    void clear();
    Media* get_root();
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int topLevelItemCount();
    Media* topLevelItem(int i);
    void addTopLevelItem(Media*);
    Media* takeTopLevelItem(int i);
    void removeTopLevelItem(Media* m);
    void update_data();

private:
    Media* root_item;
};

#endif // PROJECTMODEL_H
