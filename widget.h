#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTableView>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QTableView *view;

    void averageOp();
};
#endif // WIDGET_H
