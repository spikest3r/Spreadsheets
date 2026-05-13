#include "widget.h"

#include <QMessageBox>

void Widget::averageOp() {
    QItemSelectionModel *sel = view->selectionModel();
    QModelIndexList indexes = sel->selectedIndexes();

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
    QMessageBox::information(this, "Range average", QString("%1").arg(average));
}

