#include "widget.h"
#include "tablemodel.h"

#include <QTableView>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QMenu>
#include <QMenuBar>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

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

    QMenu* opsMenu = menuBar->addMenu("Operations");
    QAction* averageOpAction = opsMenu->addAction("Average");
    connect(averageOpAction, &QAction::triggered, this, &Widget::averageOp);

    layout->addWidget(menuBar);

    view = new QTableView(this);
    view->setModel(new TableModel(this));
    view->setSortingEnabled(true);
    view->setEditTriggers(QAbstractItemView::DoubleClicked);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->addWidget(view);

    view->show();
}

Widget::~Widget() {}

void Widget::resizeEvent(QResizeEvent *event) {
    QSize newSize = event->size();
    QSize oldSize = event->oldSize();

    QWidget::resizeEvent(event);
}
