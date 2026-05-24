#include "widget.h"

void Widget::bgColorBtn() {
    QColor initialColor = Qt::black;

    QColor selectedColor = QColorDialog::getColor(
        initialColor,               // Default selected color
        this,                       // Parent widget
        "Select Cell Background"    // Dialog title
        );

    if (selectedColor.isValid()) {
        auto model = (TableModel*)view->model();
        auto selected = view->selectionModel();

        QModelIndexList indexes = selected->selectedIndexes();
        if (indexes.isEmpty()) {
            pushStatusMessage("No cells selected");
            return;
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

        model->setCellColor(topLeft, bottomRight, selectedColor);

        pushStatusMessage("Updated cell color");
    } else {
        pushStatusMessage("Color selection canceled");
    }
}

void Widget::fgColorBtn() {
    QColor initialColor = Qt::white;

    QColor selectedColor = QColorDialog::getColor(
        initialColor,               // Default selected color
        this,                       // Parent widget
        "Select Text Color"    // Dialog title
        );

    if (selectedColor.isValid()) {
        auto model = (TableModel*)view->model();
        auto selected = view->selectionModel();

        QModelIndexList indexes = selected->selectedIndexes();
        if (indexes.isEmpty()) {
            pushStatusMessage("No cells selected");
            return;
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

        model->setTextColor(topLeft, bottomRight, selectedColor);

        pushStatusMessage("Updated cell color");
    } else {
        pushStatusMessage("Color selection canceled");
    }
}

struct StyleCount {
    QColor cellColor;
    QColor textColor;
    quint8 fontStyle;
    int count;
};

void Widget::styleBrushBtn(bool smartFill) {
    auto model = (TableModel*)view->model();
    auto selected = view->selectionModel();

    QModelIndexList indexes = selected->selectedIndexes();
    if (indexes.isEmpty()) {
        pushStatusMessage("No cells selected");
        return;
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

    QVector<StyleCount> styles;
    StyleCount* dominant = nullptr;

    if(!smartFill) {
        // 1. find most common style pattern
        for (const QModelIndex& idx : indexes) {
            int row = idx.row();
            int col = idx.column();

            auto cell = model->getCell(row, col);
            if(!cell) continue;
            if(!cell->styled) continue;

            QColor bg = cell->cellColor;
            QColor fg = cell->textColor;
            quint8 fs = cell->fontFlags;

            bool found = false;
            for (StyleCount& sc : styles) {
                if (sc.cellColor == bg && sc.textColor == fg && sc.fontStyle == fs) {
                    sc.count++;
                    found = true;
                    break;
                }
            }
            if (!found) styles.push_back({bg, fg, fs, 1});
        }
        dominant = &styles[0];
        for (StyleCount& sc : styles) {
            if (sc.count > dominant->count) dominant = &sc;
        }
    } else {
        auto tlCell = model->getCell(topLeft.first, topLeft.second);
        styles.push_back({tlCell->cellColor, tlCell->textColor, 1});
        dominant = &styles[0];
    }

    if(dominant == nullptr) {
        // all cells had styled == false
        pushStatusMessage("Style Brush: No style to apply");
        return;
    }

    // 2. apply most common style pattern to all cells
    model->setCellColor(topLeft, bottomRight, dominant->cellColor);
    model->setTextColor(topLeft, bottomRight, dominant->textColor);
    model->setRangeFontFlags(topLeft, bottomRight, dominant->fontStyle);

    if(!smartFill) pushStatusMessage("Style Brush: Applied style");
}

void Widget::removeStylesBtn() {
    auto model = (TableModel*)view->model();
    auto selected = view->selectionModel();

    QModelIndexList indexes = selected->selectedIndexes();
    if (indexes.isEmpty()) {
        pushStatusMessage("No cells selected");
        return;
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

    model->setCellColor(topLeft, bottomRight, model->defaultBg, false);
    model->setTextColor(topLeft, bottomRight, model->defaultFg, false);
    model->setRangeFontFlags(topLeft, bottomRight, 0, false);

    pushStatusMessage("Removed all styles");
}

void Widget::toggleStyleFlag(quint8 flag) {
    auto model = (TableModel*)view->model();
    QModelIndexList indexes = view->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) { pushStatusMessage("No cells selected"); return; }

    int minRow = INT_MAX, minCol = INT_MAX, maxRow = INT_MIN, maxCol = INT_MIN;
    for (const QModelIndex& idx : indexes) {
        minRow = qMin(minRow, idx.row()); minCol = qMin(minCol, idx.column());
        maxRow = qMax(maxRow, idx.row()); maxCol = qMax(maxCol, idx.column());
    }

    bool anyHasFlag = false;
    for (const QModelIndex& idx : indexes)
        if (model->getCell(idx.row(), idx.column())->fontFlags & flag)
        { anyHasFlag = true; break; }

    model->setRangeFontFlag({minRow, minCol}, {maxRow, maxCol}, flag, !anyHasFlag);
}

void Widget::boldBtn() {
    toggleStyleFlag(FF_Bold);
}

void Widget::italicBtn() {
    toggleStyleFlag(FF_Italic);
}

void Widget::underlineBtn() {
    toggleStyleFlag(FF_Underline);
}

void Widget::strikethroughBtn() {
    toggleStyleFlag(FF_StrikeOut);
}
