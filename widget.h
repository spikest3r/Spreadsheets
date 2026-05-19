#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableView>

enum FormulaOP {
    NOTHING,
    CELLREF,
    SUM
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void onRangeSelectionChanged(const QItemSelection &selected,
                                         const QItemSelection &deselected);

private:
    QTableView *view;

    void averageBtn();
    void testParseBtn(); // TODO: Remove

    float averageOp();
    QLabel *labelAverage;
    float parseFormula(QString formula, bool* err);
    float parseOperation(FormulaOP operation, std::vector<QString> args, bool* error);
    FormulaOP strToOp(QString str);
    std::vector<QString> tokenizeFormula(QString formula);
    std::vector<QString> evaluateExpression(std::vector<QString> tokens, bool* err);
};
#endif // WIDGET_H
