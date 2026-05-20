#include "widget.h"
#include "tablemodel.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 8);

    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->setStyleSheet(
        "QMenuBar {"
        "  background-color: #2c3e50;" // Different tone
        "  color: white;"
        "  border-bottom: 1px solid #1a252f;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: #3498db;" // Accent highlight
        "}"
        );

    QStatusBar *statusBar = new QStatusBar(this);
    labelAverage = new QLabel(this);
    labelAverage->setText("Average");
    statusBar->addWidget(labelAverage);

    QMenu* opsMenu = menuBar->addMenu("Data");
    QMenu* rangeMenu = opsMenu->addMenu("Range");
    QAction* rangeAverageAction = rangeMenu->addAction("Range average");
    QAction* rangeSumAction = rangeMenu->addAction("Range sum");
    opsMenu->addSeparator();
    QAction* smartFillAction = opsMenu->addAction("Smart Fill");
    smartFillAction->setShortcut(QKeySequence("F2"));
    connect(rangeAverageAction, &QAction::triggered, this, &Widget::averageBtn);
    connect(rangeSumAction, &QAction::triggered, this, &Widget::sumBtn);
    connect(smartFillAction, &QAction::triggered, this, &Widget::smartFillBtn);

    layout->addWidget(menuBar);

    formulaBar = new QLineEdit(this);
    layout->addWidget(formulaBar);

    view = new QTableView(this);
    view->setModel(new TableModel(this));
    view->setSortingEnabled(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(view->model(), &QAbstractItemModel::dataChanged,
            this, &Widget::onDataChanged);
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &Widget::onSelectionChangedSlot);
    connect(formulaBar, &QLineEdit::returnPressed, this, &Widget::onFormulaBarEdited);

    QItemSelectionModel *selectionModel = view->selectionModel();

    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &Widget::onRangeSelectionChanged);

    layout->addWidget(view);

    layout->addWidget(statusBar);

    view->show();
}

Widget::~Widget() {}

void Widget::onFormulaBarEdited() {
    QModelIndex current = view->currentIndex();
    if (!current.isValid()) return;
    view->model()->setData(current, formulaBar->text(), Qt::EditRole);
}

void Widget::onSelectionChangedSlot(const QItemSelection &selected, const QItemSelection &deselected) {
    QModelIndexList indexes = selected.indexes();
    if (indexes.isEmpty()) return;
    QString raw = indexes.first().data(Qt::EditRole).toString();
    formulaBar->setText(raw);
}

void Widget::resizeEvent(QResizeEvent *event) {
    QSize newSize = event->size();
    QSize oldSize = event->oldSize();

    QWidget::resizeEvent(event);
}

void Widget::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    auto model = ((TableModel*)view->model());

    int row = topLeft.row();
    int col = topLeft.column();

    QVariant value = topLeft.data(Qt::EditRole); // get formula that user typed
    QString text = value.toString();
    formulaBar->setText(text);
    if(text.startsWith("=")) {
        bool error = false;
        QSet<QPair<int,int>> deps;
        float result = parseFormula(text, &error, deps);
        QString cellValue = error ? "#ERROR" : QString("%0").arg(result);
        model->computedValues[{row, col}] = cellValue;
        model->clearDependencies({row, col});
        for(QPair<int,int> dep: deps) {
            model->addDependency(dep, {row, col});
        }
    }

    auto dependents = model->getDependents({row,col});
    if(dependents.count() > 0) {
        for(auto dep: dependents) {
            int row = dep.first;
            int col = dep.second;

            emit model->dataChanged(model->index(row, col), model->index(row, col));
        }
    }
}

void Widget::onRangeSelectionChanged(const QItemSelection &selected,
                                      const QItemSelection &deselected) {
    Q_UNUSED(deselected);

    bool ok = false;
    float average = averageOp(&ok);
    if(!ok) return;
    labelAverage->setText(QString("%0").arg(average));
}

void Widget::averageBtn() {
    bool ok = false;
    float av = averageOp(&ok);
    if(!ok) {
        QMessageBox::warning(this, "Error", QString("No range selected. Highlight cells before applying this operation."));
        return;
    }
    if(std::isnan(av)) av = 0.0f;
    QMessageBox::information(this, "Range average", QString("%0").arg(av));
}

void Widget::sumBtn() {
    bool ok = false;
    float sum = sumOp(&ok);
    if(!ok) {
        QMessageBox::warning(this, "Error", QString("No range selected. Highlight cells before applying this operation."));
        return;
    }
    if(std::isnan(sum)) sum = 0.0f;
    QMessageBox::information(this, "Range sum", QString("%0").arg(sum));
}

void Widget::smartFillBtn() {
    SmartFillError err = SFE_NONE;
    smartFillOperation(err);
    if(err != SFE_NONE) {
        QString message = getErrorMessage(err);
        QMessageBox::warning(this, "Smart Fill error", message);
    }
}

QString Widget::getErrorMessage(SmartFillError error) {
    switch (error) {
    case SFE_NONE:       return "";
    case NODATA:         return "No data to fill from.";
    case TWODIMRANGE:    return "Select a single row or column.";
    case INVALIDDATA:    return "Selection contains non-numeric data.";
    case BADPATTERN:     return "No pattern detected. Try Simple Fill (Shift+F2).";
    case MIXED_TYPES: return "Selection mixes formulas and values.";
    case NOCELLREFS: return "Formula contains no cell references to offset.";
    case NOTIMPLEMENTED: return "Not implemented yet.";
    }
}
