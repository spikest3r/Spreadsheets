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

    if (role == Qt::BackgroundRole) {
        if (r < data_.size() && c < data_[r].size()) {
            bool formula = data_[r][c].value.toString().startsWith("=");
            bool error = computedValues[{r,c}].startsWith("#");
            if(error && formula) {
                return QBrush(QColor(200, 50, 50));
            }
            return data_[r][c].cellColor;
        }
    }

    if (role == Qt::EditRole)
    {
        if (r < data_.size() && c < data_[r].size())
            return data_[r][c].value;
    } else if(role == Qt::DisplayRole) {
        if (r < data_.size() && c < data_[r].size()) {
            if(data_[r][c].value.toString().startsWith("=")) {
                return computedValues[{r,c}];
            } else {
                return data_[r][c].value;
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
    bool resized = false;
    if (r >= data_.size()) {
        data_.resize(r + 1);
        resized = true;
    }

    if (c >= data_[r].size()) {
        data_[r].resize(c + 1);
        resized = true;
    }

    if(resized) {
        data_[r][c].cellColor = defaultBg;
    }

    if(!redoStack.empty()) redoStack.clear();

    EditOperation op;
    op.cell = {r, c};
    op.previousValue = data_[r][c];
    editStack.push_back(op);

    data_[r][c].value = value;

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

bool TableModel::undoLastEdit() {
    if(editStack.empty()) return false;

    EditOperation op = editStack.back();
    editStack.pop_back();

    int row = op.cell.first;
    int col = op.cell.second;

    EditOperation redoOp;
    redoOp.cell = {row, col};
    redoOp.previousValue.value = this->index(row, col).data(Qt::EditRole);
    redoOp.previousValue.cellColor = data_[row][col].cellColor;
    redoStack.push_back(redoOp);

    data_[row][col] = op.previousValue;

    emit this->dataChanged(this->index(row, col), this->index(row, col));
    return true;
}

bool TableModel::redoEdit() {
    if(redoStack.empty()) return false;

    EditOperation op = redoStack.back();
    redoStack.pop_back();

    int row = op.cell.first;
    int col = op.cell.second;

    EditOperation undoOp;
    undoOp.cell = {row, col};
    undoOp.previousValue.value = this->index(row, col).data(Qt::EditRole);
    undoOp.previousValue.cellColor = data_[row][col].cellColor;
    editStack.push_back(undoOp);

    data_[row][col] = op.previousValue;

    emit this->dataChanged(this->index(row, col), this->index(row, col));
    return true;
}

void TableModel::setCellColor(QPair<int, int> cell, QColor color) {
    int r = cell.first;
    int c = cell.second;

    bool resized = false;
    if (r >= data_.size()) {
        data_.resize(r + 1);
        resized = true;
    }

    if (c >= data_[r].size()) {
        data_[r].resize(c + 1);
        resized = true;
    }

    if(resized) {
        data_[r][c].cellColor = defaultBg;
    }

    if(!redoStack.empty()) redoStack.clear();

    EditOperation op;
    op.cell = {r, c};
    op.previousValue = data_[r][c];
    editStack.push_back(op);

    data_[r][c].cellColor = color;
    emit this->dataChanged(this->index(r, c), this->index(r, c));
}
