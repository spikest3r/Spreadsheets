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

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        int r = index.row();
        int c = index.column();

        if (r < data_.size() && c < data_[r].size())
            return data_[r][c];
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
