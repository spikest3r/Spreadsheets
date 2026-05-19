#include "widget.h"
#include "tablemodel.h"

#include <QTableView>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>

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

    QMenu* opsMenu = menuBar->addMenu("Operations");
    QAction* averageOpAction = opsMenu->addAction("Average");
    connect(averageOpAction, &QAction::triggered, this, &Widget::averageBtn);

    //TODO: Remove (debug)
    QAction* testParseAction = opsMenu->addAction("Test parse");
    connect(testParseAction, &QAction::triggered, this, &Widget::testParseBtn);

    layout->addWidget(menuBar);

    view = new QTableView(this);
    view->setModel(new TableModel(this));
    view->setSortingEnabled(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QItemSelectionModel *selectionModel = view->selectionModel();

    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &Widget::onRangeSelectionChanged);

    layout->addWidget(view);

    layout->addWidget(statusBar);

    view->show();
}

Widget::~Widget() {}

void Widget::resizeEvent(QResizeEvent *event) {
    QSize newSize = event->size();
    QSize oldSize = event->oldSize();

    QWidget::resizeEvent(event);
}

void Widget::onRangeSelectionChanged(const QItemSelection &selected,
                                      const QItemSelection &deselected) {
    Q_UNUSED(deselected);

    float average = averageOp();
    labelAverage->setText(QString("%0").arg(average));
}

void Widget::averageBtn() {
    float av = averageOp();
    QMessageBox::information(this, "Range average", QString("%0").arg(av));
}

void Widget::testParseBtn() {
    bool error = false;
    float result = parseFormula("=C(1,0)+C(0,1)", &error);
    QMessageBox::information(this, "DEBUG", QString("%0").arg(result));
}
