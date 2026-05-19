#include "tablemodel.h"

int TableModel::rowCount(const QModelIndex &parent) const {
    return 1000;
}

int TableModel::columnCount(const QModelIndex &parent) const {
    return 100;
}

QVariant TableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    int r = index.row();
    int c = index.column();

    if (role == Qt::EditRole)
    {
        if (r < data_.size() && c < data_[r].size())
            return data_[r][c];
    } else if(role == Qt::DisplayRole) {
        if (r < data_.size() && c < data_[r].size()) {
            if(data_[r][c].toString().startsWith("=")) {
                return computedValues[{r,c}];
            } else {
                return data_[r][c];
            }
        }
    }

    return {};
}

Qt::ItemFlags TableModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

bool TableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    int r = index.row();
    int c = index.column();

    // ensure storage exists
    if (r >= data_.size())
        data_.resize(r + 1);

    if (c >= data_[r].size())
        data_[r].resize(c + 1);

    data_[r][c] = value;

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

void TableModel::addDependency(QPair<int,int> dependency, QPair<int,int> dependent) {
    if(dependency == dependent) return; // edge case
    dependencyGraph[dependency].append(dependent);
}

void TableModel::removeDependency(QPair<int,int> dependency, QPair<int,int> dependent) {
    dependencyGraph[dependency].removeAll(dependent);
}

void TableModel::clearDependencies(QPair<int,int> dependent) {
    for (auto &list : dependencyGraph)
        list.removeAll(dependent);
}

QList<QPair<int,int>> TableModel::getDependents(QPair<int,int> dependency) {
    return dependencyGraph.value(dependency);
}
