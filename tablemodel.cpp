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

    if (role == Qt::ForegroundRole) {
        if (r < data_.size() && c < data_[r].size()) {
            return data_[r][c].textColor;
        }
    }

    if(role == Qt::FontRole) {
        if (r < data_.size() && c < data_[r].size()) {
            Cell cell = data_[index.row()][index.column()];
            QFont font;
            font.setBold(cell.fontFlags & FF_Bold);
            font.setItalic(cell.fontFlags & FF_Italic);
            font.setUnderline(cell.fontFlags & FF_Underline);
            font.setStrikeOut(cell.fontFlags & FF_StrikeOut);
            return font;
        }
        return QFont();
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
    checkSize(r, c);

    if(!redoStack.empty()) redoStack.clear();

    CellEdit op;
    op.cell = {r, c};
    op.before = data_[r][c];

    data_[r][c].value = value;

    op.after = data_[r][c];

    editStack.push_back(op);

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

bool TableModel::addDependency(QPair<int,int> dependency, QPair<int,int> dependent) {
    if(dependency == dependent) return false;

    QSet<QPair<int,int>> visited;
    if (hasPath(dependent, dependency, visited)) {
        return false;
    }

    dependencyGraph[dependency].append(dependent);
    return true;
}

bool TableModel::hasPath(QPair<int,int> current, QPair<int,int> target, QSet<QPair<int,int>>& visited) {
    if (current == target) return true;
    if (visited.contains(current)) return false;

    visited.insert(current);

    for (const auto& nextCell : dependencyGraph.value(current)) {
        if (hasPath(nextCell, target, visited)) {
            return true;
        }
    }

    return false;
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

    std::visit([&](auto&& op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, CellEdit>) {
            applyCell(op.cell, op.before);
        } else if constexpr (std::is_same_v<T, RangeEdit>) {
            applyRange(op.topLeft, op.bottomRight, op.before);
        }
    }, op);

    redoStack.push_back(op);

    return true;
}

bool TableModel::redoEdit() {
    if(redoStack.empty()) return false;

    EditOperation op = redoStack.back();
    redoStack.pop_back();

    std::visit([&](auto&& op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, CellEdit>) {
            applyCell(op.cell, op.after);
        } else if constexpr (std::is_same_v<T, RangeEdit>) {
            applyRange(op.topLeft, op.bottomRight, op.after);
        }
    }, op);

    editStack.push_back(op);

    return true;
}

void TableModel::setCellColor(QPair<int, int> topLeft, QPair<int, int> bottomRight, QColor color, bool flagStyled) {
    checkSize(bottomRight.first, bottomRight.second);

    if(!redoStack.empty()) redoStack.clear();

    RangeEdit op;
    op.topLeft = topLeft;
    op.bottomRight = bottomRight;

    int top = topLeft.first;
    int left = topLeft.second;
    int bottom = bottomRight.first;
    int right = bottomRight.second;

    for(int i = top; i <= bottom; i++) {
        op.before.push_back({});
        op.after.push_back({});
        for(int j = left; j <= right; j++) {
            op.before[i - top].push_back(data_[i][j]);
            data_[i][j].cellColor = color;
            data_[i][j].styled = flagStyled;
            op.after[i - top].push_back(data_[i][j]);
            emit dataChanged(index(i, j), index(i, j));
        }
    }

    editStack.push_back(op);
}

void TableModel::setTextColor(QPair<int, int> topLeft, QPair<int, int> bottomRight, QColor color, bool flagStyled) {
    checkSize(bottomRight.first, bottomRight.second);

    if(!redoStack.empty()) redoStack.clear();

    RangeEdit op;
    op.topLeft = topLeft;
    op.bottomRight = bottomRight;

    int top = topLeft.first;
    int left = topLeft.second;
    int bottom = bottomRight.first;
    int right = bottomRight.second;

    for(int i = top; i <= bottom; i++) {
        op.before.push_back({});
        op.after.push_back({});
        for(int j = left; j <= right; j++) {
            op.before[i - top].push_back(data_[i][j]);
            data_[i][j].textColor = color;
            data_[i][j].styled = flagStyled;
            op.after[i - top].push_back(data_[i][j]);
            emit dataChanged(index(i, j), index(i, j));
        }
    }

    editStack.push_back(op);
}

void TableModel::setRangeFontFlag(QPair<int,int> topLeft, QPair<int,int> bottomRight, quint8 flag, bool on, bool flagStyled) {
    int top = topLeft.first, left = topLeft.second;
    int bottom = bottomRight.first, right = bottomRight.second;
    checkSize(bottom, right);
    if (!redoStack.empty()) redoStack.clear();
    RangeEdit op;
    op.topLeft = topLeft;
    op.bottomRight = bottomRight;
    for (int i = top; i <= bottom; i++) {
        op.before.push_back({});
        op.after.push_back({});
        for (int j = left; j <= right; j++) {
            op.before[i - top].push_back(data_[i][j]);
            if (on) data_[i][j].fontFlags |= flag;
            else    data_[i][j].fontFlags &= ~flag;
            data_[i][j].styled = flagStyled;
            op.after[i - top].push_back(data_[i][j]);
        }
    }
    editStack.push_back(op);
    emit dataChanged(index(top, left), index(bottom, right));
}

void TableModel::setRangeFontFlags(QPair<int,int> topLeft, QPair<int,int> bottomRight, quint8 flags, bool flagStyled) {
    int top = topLeft.first, left = topLeft.second;
    int bottom = bottomRight.first, right = bottomRight.second;
    checkSize(bottom, right);
    if (!redoStack.empty()) redoStack.clear();
    RangeEdit op;
    op.topLeft = topLeft;
    op.bottomRight = bottomRight;
    for (int i = top; i <= bottom; i++) {
        op.before.push_back({});
        op.after.push_back({});
        for (int j = left; j <= right; j++) {
            op.before[i - top].push_back(data_[i][j]);
            data_[i][j].fontFlags = flags;
            data_[i][j].styled = flagStyled;
            op.after[i - top].push_back(data_[i][j]);
        }
    }
    editStack.push_back(op);
    emit dataChanged(index(top, left), index(bottom, right));
}

void TableModel::setRangeValue(QPair<int, int> topLeft, QPair<int, int> bottomRight, QString value) {
    checkSize(bottomRight.first, bottomRight.second);

    if(!redoStack.empty()) redoStack.clear();

    RangeEdit op;
    op.topLeft = topLeft;
    op.bottomRight = bottomRight;

    int top = topLeft.first;
    int left = topLeft.second;
    int bottom = bottomRight.first;
    int right = bottomRight.second;

    for(int i = top; i <= bottom; i++) {
        op.before.push_back({});
        op.after.push_back({});
        for(int j = left; j <= right; j++) {
            op.before[i - top].push_back(data_[i][j]);
            data_[i][j].value = value;
            op.after[i - top].push_back(data_[i][j]);
            emit dataChanged(index(i, j), index(i, j));
        }
    }

    editStack.push_back(op);
}

void TableModel::setRangeValues(QPair<int,int> topLeft, QPair<int,int> bottomRight,
                                QVector<QString> values, FillDirection fd) {
    checkSize(bottomRight.first, bottomRight.second);
    if (!redoStack.empty()) redoStack.clear();

    RangeEdit op;
    op.topLeft = topLeft;
    op.bottomRight = bottomRight;

    int top = topLeft.first, left = topLeft.second;
    int bottom = bottomRight.first, right = bottomRight.second;
    int idx = 0;

    for (int i = top; i <= bottom; i++) {
        op.before.push_back({});
        op.after.push_back({});
        for (int j = left; j <= right; j++) {
            op.before[i - top].push_back(data_[i][j]);
            if (idx < values.size())
                data_[i][j].value = values[idx++];
            op.after[i - top].push_back(data_[i][j]);
        }
    }

    editStack.push_back(op);
    emit dataChanged(index(top, left), index(bottom, right));
}

void TableModel::checkSize(int r, int c) {
    if (r >= (int)data_.size()) {
        data_.resize(r + 1);
    }
    for (int ri = 0; ri <= r; ri++) {
        int oldCols = data_[ri].size();
        if (c >= oldCols) {
            data_[ri].resize(c + 1);
            for (int ci = oldCols; ci <= c; ci++) {
                data_[ri][ci].cellColor = defaultBg;
                data_[ri][ci].textColor = defaultFg;
            }
        }
    }
}

void TableModel::applyCell(QPair<int, int> cell, Cell before) {
    int r = cell.first;
    int c = cell.second;
    checkSize(r, c);
    data_[r][c] = before;
    emit this->dataChanged(this->index(r, c), this->index(r, c));
}

void TableModel::applyRange(QPair<int, int> topLeft, QPair<int, int> bottomRight, QVector<QVector<Cell>> before) {
    int top = topLeft.first;
    int left = topLeft.second;
    int bottom = bottomRight.first;
    int right = bottomRight.second;
    checkSize(bottom, right);
    for(int i = top; i < bottom + 1; i++) {
        for(int j = left; j < right + 1; j++) {
            data_[i][j] = before[i - top][j - left];
            emit this->dataChanged(this->index(i, j), this->index(i, j));
        }
    }
}

void TableModel::reset() {
    beginResetModel();
    data_.clear();
    editStack.clear();
    redoStack.clear();
    endResetModel();
}

Cell* TableModel::getCell(int row, int col) {
    checkSize(row, col);
    return &data_[row][col];
}
