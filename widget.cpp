#include "widget.h"
#include "tablemodel.h"
#include "about.h"
#include "lumen-inc/vm.h"

#include "scriptingpanel.h"

Widget* Widget::instance = nullptr;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    instance = this;

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
    QAction* newAction = fileMenu->addAction("New");
    connect(newAction, &QAction::triggered, this, &Widget::newBtn);
    newAction->setShortcut(QKeySequence::New);
    QAction* saveAction = fileMenu->addAction("Save");
    connect(saveAction, &QAction::triggered, this, &Widget::saveBtn);
    saveAction->setShortcut(QKeySequence::Save);
    QAction* loadAction = fileMenu->addAction("Open");
    connect(loadAction, &QAction::triggered, this, &Widget::loadBtn);
    loadAction->setShortcut(QKeySequence::Open);
    fileMenu->addSeparator();
    QAction* undoAction = fileMenu->addAction("Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &Widget::undoBtn);
    QAction* redoAction = fileMenu->addAction("Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &Widget::redoBtn);
    fileMenu->addSeparator();
    QAction *aboutAction = fileMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, this, &Widget::aboutBtn);

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

    QAction* boldAction = styleMenu->addAction("Bold");
    boldAction->setShortcut(QKeySequence::Bold);
    connect(boldAction, &QAction::triggered, this, &Widget::boldBtn);

    QAction* italicAction = styleMenu->addAction("Italic");
    italicAction->setShortcut(QKeySequence::Italic);
    connect(italicAction, &QAction::triggered, this, &Widget::italicBtn);

    QAction* underlineAction = styleMenu->addAction("Underline");
    underlineAction->setShortcut(QKeySequence::Underline);
    connect(underlineAction, &QAction::triggered, this, &Widget::underlineBtn);

    QAction* strikethruAction = styleMenu->addAction("Strike-through");
    connect(strikethruAction, &QAction::triggered, this, &Widget::strikethroughBtn);

    styleMenu->addSeparator();

    QAction* cellBgAction = styleMenu->addAction("Set cell background color");
    connect(cellBgAction, &QAction::triggered, this, &Widget::bgColorBtn);
    QAction* cellFgAction = styleMenu->addAction("Set cell text color");
    connect(cellFgAction, &QAction::triggered, this, &Widget::fgColorBtn);

    styleMenu->addSeparator();

    QAction* styleBrushAction = styleMenu->addAction("Style Brush");
    styleBrushAction->setShortcut(QKeySequence("F3"));
    connect(styleBrushAction, &QAction::triggered, this, &Widget::styleBrushBtn);

    QAction* removeStylesAction = styleMenu->addAction("Clear styles");
    removeStylesAction->setShortcut(QKeySequence("Shift+F3"));
    connect(removeStylesAction, &QAction::triggered, this, &Widget::removeStylesBtn);

    QAction* scriptEditorAction = menuBar->addAction("Scripting");
    connect(scriptEditorAction, &QAction::triggered, this, &Widget::scriptEditorBtn);

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
    view->setCurrentIndex(model->index(0, 0));

    QPalette palette = QApplication::palette();
    QColor defaultBgColor = palette.color(QPalette::Base);
    QColor defaultFgColor = palette.color(QPalette::Text);
    model->defaultBg = defaultBgColor;
    model->defaultFg = defaultFgColor;

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

    ScriptingPanel::initialize();

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

void Widget::newBtn() {
    if(modifiedSinceSave) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes",
            "Do you want to save before creating new spreadsheet?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Yes) {
            bool save = saveBtn();
            if (!save) {
                // saving has failed
                return;
            }

            auto model = (TableModel*)view->model();

            // accept
            model->reset();
            modifiedSinceSave = true;
            isSaved = false;
            filename.clear();
            pushStatusMessage("New spreadsheet");
        } else if (reply == QMessageBox::No) {
            // accept
            auto model = (TableModel*)view->model();
            model->reset();
            modifiedSinceSave = true;
            isSaved = false;
            filename.clear();
            pushStatusMessage("New spreadsheet");
        } else { // Cancel
            // ignore
        }
    } else {
        // accept
        auto model = (TableModel*)view->model();
        model->reset();
        modifiedSinceSave = true;
        isSaved = false;
        filename.clear();
        pushStatusMessage("New spreadsheet");
    }
}

void Widget::closeEvent(QCloseEvent* event) {
    if(modifiedSinceSave) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes",
            "Do you want to save before closing?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Yes) {
            bool save = saveBtn();
            if(!save) {
                // saving has failed
                event->ignore();
                return;
            }
            setVMhalt();
            if (vmThread.joinable())
                vmThread.join();
            event->accept();
            ScriptingPanel::closeIfOpen();
        } else if (reply == QMessageBox::No) {
            setVMhalt();
            if (vmThread.joinable())
                vmThread.join();
            event->accept();
            ScriptingPanel::closeIfOpen();
        } else { // Cancel
            event->ignore();
        }
    } else {
        setVMhalt();
        if (vmThread.joinable())
            vmThread.join();
        event->accept();
        ScriptingPanel::closeIfOpen();
    }
}

void Widget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        auto selected = view->selectionModel()->selectedIndexes();
        if (!selected.isEmpty()) {
            auto model = (TableModel*)view->model();

            int minRow = INT_MAX, minCol = INT_MAX;
            int maxRow = INT_MIN, maxCol = INT_MIN;

            for (const QModelIndex& idx : selected) {
                minRow = qMin(minRow, idx.row());
                minCol = qMin(minCol, idx.column());
                maxRow = qMax(maxRow, idx.row());
                maxCol = qMax(maxCol, idx.column());
            }

            QPair<int,int> topLeft     = {minRow, minCol};
            QPair<int,int> bottomRight = {maxRow, maxCol};

            model->setRangeValue(topLeft, bottomRight, "");
            return;
        }
    } else if (event->matches(QKeySequence::Copy)) {
        // Handle Ctrl+C
        pushStatusMessage("Copy");
        copyBuffer.clear();

        auto model = (TableModel*)view->model();
        auto selected = view->selectionModel()->selectedIndexes();

        int minRow = INT_MAX, maxRow = INT_MIN;
        int minCol = INT_MAX, maxCol = INT_MIN;

        for (const auto &idx : selected) {
            minRow = std::min(minRow, idx.row());
            maxRow = std::max(maxRow, idx.row());
            minCol = std::min(minCol, idx.column());
            maxCol = std::max(maxCol, idx.column());
        }

        for (int row = minRow; row <= maxRow; row++) {
            copyBuffer.push_back({});
            for (int col = minCol; col <= maxCol; col++) {
                auto cell = model->getCell(row, col);
                copyBuffer.back().push_back(*cell);
            }
        }

        event->accept();
    } else if (event->matches(QKeySequence::Paste)) {
        // Handle Ctrl+V
        auto model = (TableModel*)view->model();
        auto selected = view->selectionModel()->selectedIndexes();

        if(copyBuffer.empty()) {
            pushStatusMessage("Nothing to paste");
        } else {
            pushStatusMessage("Paste");

            // get top left ordered
            auto first = selected.first();
            for (auto& idx : selected) {
                if (idx.row() < first.row() ||
                    (idx.row() == first.row() && idx.column() < first.column()))
                    first = idx;
            }
            int row = first.row();
            int col = first.column();

            // uniform rectangular region
            QPair<int, int> topLeft = {row, col};
            QPair<int, int> bottomRight = {
                row + copyBuffer.size() - 1,
                col + copyBuffer[0].size() - 1
            };

            model->applyRange(topLeft, bottomRight, copyBuffer);
        }
        event->accept();
    } else {
        QWidget::keyPressEvent(event); // pass unhandled events up
    }
}

void Widget::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    modifiedSinceSave = true;
    auto model = ((TableModel*)view->model());

    for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
        for (int col = topLeft.column(); col <= bottomRight.column(); col++) {
            QModelIndex idx = model->index(row, col);
            QVariant value = idx.data(Qt::EditRole);
            QString text = value.toString();

            // update formula bar only for single cell change
            if (topLeft == bottomRight) formulaBar->setText(text);

            if (text.startsWith("=")) {
                FormulaParserError error = FPE_NONE;
                QSet<QPair<int,int>> deps;
                QString result = parseFormula(text, error, deps);
                QString cellValue = error != FPE_NONE ? getCellError(error) : result;
                if (error != FPE_NONE) pushStatusMessage(getErrorMessage(error));
                model->clearDependencies({row, col});
                for (QPair<int,int> dep : deps) {
                    bool ok = model->addDependency(dep, {row, col});
                    if(!ok) {
                        error = REFERENCE;
                        pushStatusMessage(getErrorMessage(error));
                        model->clearDependencies({row,col});
                        cellValue = getCellError(error);
                        break;
                    }
                }
                model->computedValues[{row, col}] = cellValue;
            }

            auto dependents = model->getDependents({row, col});
            for (auto dep : dependents) {
                emit model->dataChanged(model->index(dep.first, dep.second),
                                        model->index(dep.first, dep.second));
            }
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
            if (key->modifiers() & Qt::ShiftModifier) {
                nextIdx = view->model()->index(idx.row() - 1, idx.column());
            } else {
                nextIdx = view->model()->index(idx.row() + 1, idx.column());
            }
        } else if (key->key() == Qt::Key_Tab) {
            if (key->modifiers() & Qt::ShiftModifier) {
                nextIdx = view->model()->index(idx.row(), idx.column() - 1);
            } else {
                nextIdx = view->model()->index(idx.row(), idx.column() + 1);
            }
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
        styleBrushBtn(true);

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

bool Widget::saveBtn() {
    auto model = (TableModel*)view->model();
    if(!isSaved) {
        QString path = QFileDialog::getSaveFileName(this, "Save File", "", "Table Files (*.tblx);;All Files (*)");
        if (!path.isEmpty()) {
            if (!model->saveToFile(path, ScriptingPanel::scripts)) {
                QMessageBox::critical(this, "Error", "Failed to save file.");
                return false;
            }
            else {
                isSaved = true;
                modifiedSinceSave = false;
                filename = path;
                pushStatusMessage("File saved");
                return true;
            }
        }
    } else {
        if (!model->saveToFile(filename, ScriptingPanel::scripts)) {
            QMessageBox::critical(this, "Error", "Failed to save file.");
            return false;
        }
        else {
            isSaved = true;
            modifiedSinceSave = false;
            pushStatusMessage("File saved");
            return true;
        }
    }
    return false;
}

bool Widget::loadBtn() {
    auto model = (TableModel*)view->model();
    QString path = QFileDialog::getOpenFileName(this, "Open File", "", "Table Files (*.tblx);;All Files (*)");
    if (!path.isEmpty()) {
        if (!model->loadFromFile(path, ScriptingPanel::scripts)) {
            QMessageBox::critical(this, "Error", "Failed to load file.");
            return false;
        }
        else {
            isSaved = true;
            modifiedSinceSave = false;
            filename = path;
            pushStatusMessage("File opened");
            ScriptingPanel::updateMenu();
            return true;
        }
    }
    return false;
}

void Widget::scriptEditorBtn() {
    ScriptingPanel::showPanel();
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

void Widget::aboutBtn() {
    about *dialogAbout = new about();
    dialogAbout->exec();
    delete dialogAbout;
}

TableModel* Widget::getTableModel() {
    return (TableModel*)view->model();
}

void Widget::setModifiedFlag() {
    modifiedSinceSave = true;
}
