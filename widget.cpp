#include "widget.h"
#include "tablemodel.h"

#include <QTableView>
#include <QResizeEvent>
#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

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
