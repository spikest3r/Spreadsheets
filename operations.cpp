#include "widget.h"

#include <QMessageBox>

// Range operations

float Widget::averageOp(bool* ok) {
    QItemSelectionModel *sel = view->selectionModel();
    QModelIndexList indexes = sel->selectedIndexes();

    if(indexes.count() == 0) {
        *ok = false;
        return 0.0f;
    }

    float average = 0.0f;
    int count = 0;

    for (const QModelIndex &index : indexes) {
        int row = index.row();
        int col = index.column();
        QVariant value = index.data();

        bool ok = false;
        float v = value.toFloat(&ok);
        if(!ok) continue; // TODO: Illegal value warning
        average += v;
        count++;
    }

    average /= (float)count;

    *ok = true;
    return average;
}

float Widget::sumOp(bool* ok) {
    QItemSelectionModel *sel = view->selectionModel();
    QModelIndexList indexes = sel->selectedIndexes();

    if(indexes.count() == 0) {
        *ok = false;
        return 0.0f;
    }

    float sum = 0.0f;

    for (const QModelIndex &index : indexes) {
        int row = index.row();
        int col = index.column();
        QVariant value = index.data();

        bool ok = false;
        float v = value.toFloat(&ok);
        if(!ok) continue; // TODO: Illegal value warning
        sum += v;
    }

    *ok = true;
    return sum;
}

int Widget::countOp() {
    QItemSelectionModel *sel = view->selectionModel();
    QModelIndexList indexes = sel->selectedIndexes();
    int count = indexes.count();
    return count;
}
