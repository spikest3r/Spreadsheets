#ifndef TABLEMODEL_H
#define TABLEMODEL_H
#include <QAbstractTableModel>
#include <QVariant>
#include <QModelIndex>
#include <QPair>

struct EditOperation {
    QPair<int, int> cell;
    QVariant previousValue;
};

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
    QVector<EditOperation> editStack;
    QVector<EditOperation> redoStack; // push what was undid, purge on new edit

public:
    QHash<QPair<int,int>, QList<QPair<int,int>>> dependencyGraph;
    QMap<QPair<int,int>, QString> computedValues;
    void addDependency(QPair<int,int> dependency, QPair<int,int> dependent);
    void removeDependency(QPair<int,int> dependency, QPair<int,int> dependent);
    void clearDependencies(QPair<int,int> dependent);
    QList<QPair<int,int>> getDependents(QPair<int,int> dependency);
    bool undoLastEdit();
    bool redoEdit();
};

#endif // TABLEMODEL_H
