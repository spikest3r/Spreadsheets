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

    statusBarText = new QLabel(this);
    statusBarText->setContentsMargins(8, 0, 8, 0);
    statusBarText->setFixedWidth(240);
    statusBarText->setText("Ready");
    statusBar->addWidget(statusBarText);

    labelAverage = new QLabel(this);
    labelAverage->setContentsMargins(8, 0, 8, 0);
    statusBar->addWidget(labelAverage);

    labelSum = new QLabel(this);
    labelSum->setContentsMargins(8, 0, 8, 0);
    statusBar->addWidget(labelSum);

    labelCount = new QLabel(this);
    labelCount->setContentsMargins(8, 0, 8, 0);
    statusBar->addWidget(labelCount);

    labelAverage->setVisible(false);
    labelSum->setVisible(false);
    labelCount->setVisible(false);

    QMenu* fileMenu = menuBar->addMenu("File");
    QAction* undoAction = fileMenu->addAction("Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &Widget::undoBtn);
    QAction* redoAction = fileMenu->addAction("Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &Widget::redoBtn);

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

    QMenu* styleMenu = menuBar->addMenu("Style");
    QAction* cellBgAction = styleMenu->addAction("Set cell background color");
    connect(cellBgAction, &QAction::triggered, this, &Widget::bgColorBtn);

    layout->addWidget(menuBar);

    formulaBar = new QLineEdit(this);
    layout->addWidget(formulaBar);

    view = new QTableView(this);
    auto model = new TableModel(this);
    view->setModel(model);
    view->setSortingEnabled(true);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setEditTriggers(QAbstractItemView::AnyKeyPressed |
                          QAbstractItemView::DoubleClicked);
    view->installEventFilter(this);

    QPalette palette = QApplication::palette();
    QColor defaultBgColor = palette.color(QPalette::Base);
    model->defaultBg = defaultBgColor;

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

    statusMessageTimer = new QTimer(this);
    statusMessageTimer->setInterval(2500);
    statusMessageTimer->setSingleShot(true);
    connect(statusMessageTimer, &QTimer::timeout, this, &Widget::statusMessageTimerAction);

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
    auto cell = indexes.first();
    QString raw = cell.data(Qt::EditRole).toString();
    formulaBar->setText(raw);
    QString computed = cell.data(Qt::DisplayRole).toString();
    if(computed.startsWith("#") && raw.startsWith("=")) { // filter user typed # and formula error induced #
        auto error = STR2FPE(computed);
        auto message = getErrorMessage(error);
        pushStatusMessage(message);
    }
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
        FormulaParserError error = FPE_NONE;
        QSet<QPair<int,int>> deps;
        float result = parseFormula(text, error, deps);
        QString cellValue = error != FPE_NONE ? getCellError(error) : QString("%0").arg(result);
        if(error != FPE_NONE) pushStatusMessage(getErrorMessage(error));
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

    int count = countOp();
    if(count < 2) {
        labelAverage->setVisible(false);
        labelSum->setVisible(false);
        labelCount->setVisible(false);
        return;
    } else {
        labelAverage->setVisible(true);
        labelSum->setVisible(true);
        labelCount->setVisible(true);
    }

    bool ok = false;

    float average = averageOp(&ok);
    float sum = sumOp(&ok);

    if(!ok) return;

    if(std::isnan(average)) average = 0.0f;

    labelAverage->setText("Average: " + QString::number(average, 'f', 0));
    labelSum->setText("Sum: " + QString::number(sum, 'f', 0));
    labelCount->setText("Count: " + QString::number(count, 'f', 0));
}

bool Widget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == view && event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent*>(event);
        QModelIndex idx = view->currentIndex();

        QModelIndex nextIdx;
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            nextIdx = view->model()->index(idx.row() + 1, idx.column());
        } else if (key->key() == Qt::Key_Tab) {
            nextIdx = view->model()->index(idx.row(), idx.column() + 1);
        }

        if (nextIdx.isValid()) {
            QMetaObject::invokeMethod(view, [this, nextIdx]() {
                view->setCurrentIndex(nextIdx);
            }, Qt::QueuedConnection);

            return true; // Mark event as handled
        }
    }
    return QWidget::eventFilter(obj, event);
}

void Widget::averageBtn() {
    bool ok = false;
    float av = averageOp(&ok);
    if(!ok) {
        QMessageBox::warning(this, "Error", QString("No range selected. Highlight cells before applying this operation."));
        return;
    }
    if(std::isnan(av)) av = 0.0f;
    QMessageBox::information(this, "Range average", QString::number(av, 'f', 0));
}

void Widget::sumBtn() {
    bool ok = false;
    float sum = sumOp(&ok);
    if(!ok) {
        QMessageBox::warning(this, "Error", QString("No range selected. Highlight cells before applying this operation."));
        return;
    }
    if(std::isnan(sum)) sum = 0.0f;
    QMessageBox::information(this, "Range sum", QString::number(sum, 'f', 0));
}

void Widget::smartFillBtn() {
    SmartFillError err = SFE_NONE;
    SmartFillOperation op = SFO_NONE;
    smartFillOperation(err, op);
    if(err != SFE_NONE) {
        QString message = getErrorMessage(err);
        QMessageBox::warning(this, "Smart Fill error", message);
        pushStatusMessage("Smart Fill failed");
    } else {
        QString status;
        switch(op) {
        case ARITHMETIC_PROGRESSION:
            status = "Arithmetic progression";
            break;
        case GEOMETRICAL_PROGRESSION:
            status = "Geometrical progression";
            break;
        case FORMULA_OFFSET:
            status = "Relative formula pattern";
            break;
        case COPY:
            status = "Repeating sequence";
            break;
        default:
            status = "Not implemented";
            break;
        }
        pushStatusMessage("Smart Fill: " + status);
    }
}

void Widget::undoBtn() {
    bool ok = ((TableModel*)view->model())->undoLastEdit();
    if(ok) pushStatusMessage("Undo edit");
    else pushStatusMessage("Nothing to undo");
}

void Widget::redoBtn() {
    bool ok = ((TableModel*)view->model())->redoEdit();
    if(ok) pushStatusMessage("Redo edit");
    else pushStatusMessage("Nothing to redo");
}


void Widget::bgColorBtn() {
    QColor initialColor = Qt::white;

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

        for(const QModelIndex &idx : indexes) {
            int r = idx.row();
            int c = idx.column();

            model->setCellColor({r,c},selectedColor);
        }

        pushStatusMessage("Updated cell color");
    } else {
        pushStatusMessage("Color selection canceled");
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

QString Widget::getErrorMessage(FormulaParserError error) {
    switch (error) {
    case MATH_EVALUATION_ERROR:    return "Mathematical calculation error";
    case INCORRECT_ARGUMENT_COUNT: return "Wrong number of arguments";
    case NON_NUMERIC_VALUE:        return "Expected a numeric value";
    case INVALID_SYNTAX:           return "Invalid formula syntax";
    case FPE_NONE:
    default:                       return "";
    }
}

QString Widget::getCellError(FormulaParserError error) {
    switch (error) {
    case MATH_EVALUATION_ERROR:    return "#NUM!";
    case INCORRECT_ARGUMENT_COUNT: return "#N/A";
    case NON_NUMERIC_VALUE:        return "#VALUE!";
    case INVALID_SYNTAX:           return "#ERROR!";
    case FPE_NONE:
    default:                       return "";
    }
}

FormulaParserError Widget::STR2FPE(QString str) {
    if(str == "#NUM!") return MATH_EVALUATION_ERROR;
    if(str == "#N/A") return INCORRECT_ARGUMENT_COUNT;
    if(str == "#VALUE!") return NON_NUMERIC_VALUE;
    if(str == "#ERROR!") return INVALID_SYNTAX;
}

void Widget::pushStatusMessage(QString message) {
    if(statusMessageTimer->isActive()) {
        statusMessageTimer->stop();
    }

    statusBarText->setText(message);

    statusMessageTimer->start();
}

void Widget::statusMessageTimerAction() {
    statusBarText->setText("Ready");
}
