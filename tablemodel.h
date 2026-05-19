#ifndef TABLEMODEL_H
#define TABLEMODEL_H
#include <QAbstractTableModel>
#include <QVariant>
#include <QModelIndex>
#include <QPair>

inline size_t qHash(const QPair<int,int> &key, size_t seed = 0) {
    return qHash(key.first, seed) ^ qHash(key.second, seed << 1);
}

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    explicit TableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent) {}

protected:
    QVector<QVector<QVariant>> data_;

public:
    QHash<QPair<int,int>, QList<QPair<int,int>>> dependencyGraph;
    QMap<QPair<int,int>, QString> computedValues;
    void addDependency(QPair<int,int> dependency, QPair<int,int> dependent);
    void removeDependency(QPair<int,int> dependency, QPair<int,int> dependent);
    void clearDependencies(QPair<int,int> dependent);
    QList<QPair<int,int>> getDependents(QPair<int,int> dependency);
};

#endif // TABLEMODEL_H
