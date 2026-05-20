#include "widget.h"

void Widget::smartFillOperation(SmartFillError& error) {
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
        else hasRegularValue = true;
    }

    // check types
    if(hasEquals && hasRegularValue) {
        // contains both formulas and regular values
        error = MIXED_TYPES;
        return;
    } else if(hasEquals) {
        // formula progression
        // TODO: Handle formulas
        error = NOTIMPLEMENTED;
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
            view->model()->setData(idx, newCellValue);
        }

        if(!fill) previousValue = value;
        count++;
    }
}
