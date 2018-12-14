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
    Media *getItem(const QModelIndex &index) const;

    void appendChild(Media* parent, Media* child);
    void moveChild(Media *child, Media *to);
    void removeChild(Media *parent, Media* m);
    Media *child(int i, Media* parent = NULL);
    int childCount(Media* parent = NULL);

private:
    Media* root_item;
};

#endif // PROJECTMODEL_H
