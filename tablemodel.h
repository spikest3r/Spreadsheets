#ifndef TABLEMODEL_H
#define TABLEMODEL_H
#include <QAbstractTableModel>
#include <QVariant>
#include <QModelIndex>
#include <QPair>
#include <QColor>
#include <QBrush>
#include <QApplication>
#include <variant>
#include <QFile>
#include <QFont>
#include "global.h"

struct CellEdit {
    QPair<int, int> cell;
    Cell before, after;
};

struct RangeEdit {
    QPair<int, int> topLeft;
    QPair<int, int> bottomRight;
    QVector<QVector<Cell>> before;
    QVector<QVector<Cell>> after;
};

using EditOperation = std::variant<CellEdit, RangeEdit>;

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
    QVector<QVector<Cell>> data_;
    QVector<EditOperation> editStack;
    QVector<EditOperation> redoStack; // push what was undid, purge on new edit
    void checkSize(int r, int c);
    void applyCell_internal(QPair<int, int> cell, Cell before);
    void applyRange_internal(QPair<int, int> topLeft, QPair<int, int> bottomRight, QVector<QVector<Cell>> before);

public:
    QHash<QPair<int,int>, QList<QPair<int,int>>> dependencyGraph;
    QMap<QPair<int,int>, QString> computedValues;
    bool addDependency(QPair<int,int> dependency, QPair<int,int> dependent);
    bool hasPath(QPair<int,int> current, QPair<int,int> target, QSet<QPair<int,int>>& visited);
    void removeDependency(QPair<int,int> dependency, QPair<int,int> dependent);
    void clearDependencies(QPair<int,int> dependent);
    QList<QPair<int,int>> getDependents(QPair<int,int> dependency);
    bool undoLastEdit();
    bool redoEdit();

    void setCellColor(QPair<int, int> topLeft, QPair<int, int> bottomRight, QColor color, bool flagStyled = true);
    void setTextColor(QPair<int, int> topLeft, QPair<int, int> bottomRight, QColor color, bool flagStyled = true);
    void setRangeFontFlag(QPair<int,int> topLeft, QPair<int,int> bottomRight, quint8 flag, bool on, bool flagStyled = true);
    void setRangeFontFlags(QPair<int,int> topLeft, QPair<int,int> bottomRight, quint8 flags, bool flagStyled = true);
    void setRangeValue(QPair<int, int> topLeft, QPair<int, int> bottomRight, QString value);
    void setRangeValues(QPair<int,int> topLeft, QPair<int,int> bottomRight,
                        QVector<QString> values, FillDirection fd);
    void applyCell(QPair<int, int> topLeft, Cell cell);
    void applyRange(QPair<int, int> topLeft, QPair<int, int> bottomRight, QVector<QVector<Cell>> range);

    bool saveToFile(const QString& path, const QVector<Script>& scripts);
    bool loadFromFile(const QString& path, QVector<Script>& scripts);
    void reset();

    Cell* getCell(int row, int col);

    QColor defaultBg;
    QColor defaultFg;
};

#endif // TABLEMODEL_H
