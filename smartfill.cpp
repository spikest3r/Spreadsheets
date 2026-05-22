#include "widget.h"

FillDirection getFillDirection(const QModelIndexList &indexes, int rowCount) {
    if (rowCount == 1) {
        return indexes.first().column() < indexes.last().column() ? ROW_POS : ROW_NEG;
    } else {
        return indexes.first().row() < indexes.last().row() ? COL_POS : COL_NEG;
    }
}

void Widget::smartFillOperation(SmartFillError& error, SmartFillOperation& operation) {
    QItemSelectionModel *sel = view->selectionModel();
    QModelIndexList indexes = sel->selectedIndexes();

    if(indexes.count() == 0) {
        error = NODATA;
        return;
    }

    // get row and column count
    int minRow = INT_MAX, maxRow = INT_MIN;
    int minCol = INT_MAX, maxCol = INT_MIN;

    for (const QModelIndex &idx : indexes) {
        minRow = std::min(minRow, idx.row());
        maxRow = std::max(maxRow, idx.row());
        minCol = std::min(minCol, idx.column());
        maxCol = std::max(maxCol, idx.column());
    }

    int rowCount    = maxRow - minRow + 1;
    int columnCount = maxCol - minCol + 1;

    // check dimension validity
    bool isOneDimensional = (rowCount == 1 || columnCount == 1);
    if(!isOneDimensional) {
        error = TWODIMRANGE;
        return;
    }

    // sort indexes
    std::sort(indexes.begin(), indexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() == b.row() ? a.column() < b.column() : a.row() < b.row();
    });

    // check if it contains formulas
    bool hasEquals = false;
    bool hasRegularValue = false;
    for (const QModelIndex &idx : indexes) {
        auto str = idx.data(Qt::EditRole).toString();
        if(str.startsWith("=")) hasEquals = true;
        else if(!str.isEmpty()) hasRegularValue = true;
    }

    // check types
    if(hasEquals && hasRegularValue) {
        // contains both formulas and regular values
        error = MIXED_TYPES;
        return;
    } else if(hasEquals) {
        // formula progression
        operation = FORMULA_OFFSET;
        formulaSmartFill(error, rowCount, columnCount);
        return;
    }

    // run checks
    float previousValue = 0.0f;
    SmartFillOperation op = SFO_NONE;
    bool fill = false;

    float k1 = 0.0f;
    float k2 = 0.0f;
    float previousK1 = 0.0f;
    float previousK2 = 0.0f;
    bool ratio = false;

    QVector<QString> results;

    int count = 0;
    for (const QModelIndex &idx : indexes) {
        bool ok;
        auto data = idx.data();
        auto strData = data.toString();
        if(strData.isEmpty() && !fill) {
            if(op == SFO_NONE) {
                error = NODATA;
                return;
            } else {
                fill = true;
            }
        }

        auto value = data.toFloat(&ok);
        if(!ok && !fill) {
            error = INVALIDDATA;
            return;
        } else if(ok && !fill) {
            results.push_back(strData);
        }

        if(count > 0 && !fill) {
            if(value == previousValue) {
                op = COPY;
            } else {
                float difference = value - previousValue;
                float ratio = value / previousValue; // TODO: Zero division handler

                k1 = difference;
                k2 = ratio;

                if(k1 == previousK1) op = ARITHMETIC_PROGRESSION;
                else if(k2 == previousK2) op = GEOMETRICAL_PROGRESSION;
                else if (count > 1) {
                    error = BADPATTERN;
                    return;
                }

                previousK1 = k1;
                previousK2 = k2;
            }
        }

        if(fill) {
            float newCellValue = 0.0f;
            switch(op) {
            case COPY:
            {
                newCellValue = previousValue;
                break;
            }
            case ARITHMETIC_PROGRESSION:
            {
                float newValue = previousValue + k1;
                previousValue = newValue;
                newCellValue = newValue;
                break;
            }
            case GEOMETRICAL_PROGRESSION:
            {
                float newValue = previousValue * k2;
                previousValue = newValue;
                newCellValue = newValue;
                break;
            }
            default:
            {
                error = NOTIMPLEMENTED;
                return;
            }
            }
            results.push_back(QString::number(newCellValue));
        }

        if(!fill) previousValue = value;
        count++;
    }

    minRow = INT_MAX; minCol = INT_MAX;
    maxRow = INT_MIN; maxCol = INT_MIN;

    for (const QModelIndex& idx : indexes) {
        minRow = qMin(minRow, idx.row());
        minCol = qMin(minCol, idx.column());
        maxRow = qMax(maxRow, idx.row());
        maxCol = qMax(maxCol, idx.column());
    }

    QPair<int,int> topLeft     = {minRow, minCol};
    QPair<int,int> bottomRight = {maxRow, maxCol};

    auto model = (TableModel*)view->model();
    auto fd = getFillDirection(indexes, rowCount);
    model->setRangeValues(topLeft, bottomRight, results, fd);

    operation = op;
}

void Widget::formulaSmartFill(SmartFillError& error, int rowCount, int columnCount) {
    QItemSelectionModel *sel = view->selectionModel();
    QModelIndexList indexes = sel->selectedIndexes();
    // previous checks have been ran, data should be safe

    // tokenize first formula
    QModelIndex first = indexes.first();
    QString formula = first.data(Qt::EditRole).toString();
    std::vector<QString> tokens = tokenizeFormula(formula);

    // find positions of all cellrefs in formula (pos of C itself)
    std::vector<int> cellrefs;

    int j = 0;
    for(QString token: tokens) {
        if(token == "C") {
            cellrefs.push_back(j);
        }
        j++;
    }
    if(cellrefs.size() == 0) {
        error = NOCELLREFS;
        return;
    }

    // get fill direction
    FillDirection fd = getFillDirection(indexes, rowCount);
    int rowOffset = 0;
    int colOffset = 0;
    switch(fd) {
    case ROW_POS:
    {
        colOffset = 1;
        break;
    }
    case ROW_NEG:
    {
        colOffset = -1;
        break;
    }
    case COL_POS:
    {
        rowOffset = 1;
        break;
    }
    case COL_NEG:
    {
        rowOffset = -1;
        break;
    }
    }

    QVector<QString> results;

    // fill formulas
    int count = 0;
    for (const QModelIndex &idx : indexes) {
        if(count > 0) { // skip first formula
            // C(0,0)
            // x + 2, y + 4
            bool ok;

            for(auto cellref: cellrefs) {
                auto rowStr = tokens[cellref + 2];
                auto colStr = tokens[cellref + 4];

                bool rowAbs = false;
                bool colAbs = false;

                if(rowStr.startsWith("$")) {
                    rowStr = rowStr.replace("$", "");
                    rowAbs = true;
                }

                if(colStr.startsWith("$")) {
                    colStr = colStr.replace("$", "");
                    colAbs = true;
                }

                int row = rowStr.toFloat(&ok);
                int col = colStr.toFloat(&ok);

                if(!ok) {
                    error = INVALIDDATA;
                    return;
                }

                if(!rowAbs) row += rowOffset;
                if(!colAbs) col += colOffset;

                tokens[cellref + 2] = (rowAbs ? "$" : "") + QString::number(row);
                tokens[cellref + 4] = (colAbs ? "$" : "") + QString::number(col);
            }

            QString result = "=" + QStringList(tokens.begin(), tokens.end()).join("");
            results.push_back(result);
        } else {
            results.push_back(formula);
        }
        count++;
    }

    int minRow = INT_MAX, minCol = INT_MAX;
    int maxRow = INT_MIN, maxCol = INT_MIN;

    for (const QModelIndex& idx : indexes) {
        minRow = qMin(minRow, idx.row());
        minCol = qMin(minCol, idx.column());
        maxRow = qMax(maxRow, idx.row());
        maxCol = qMax(maxCol, idx.column());
    }

    QPair<int,int> topLeft     = {minRow, minCol};
    QPair<int,int> bottomRight = {maxRow, maxCol};

    auto model = (TableModel*)view->model();
    model->setRangeValues(topLeft, bottomRight, results, fd);
}
