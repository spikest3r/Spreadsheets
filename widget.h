#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableView>
#include <QSet>
#include <QTableView>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>

#define _USE_MATH_DEFINES
#include <cmath>

enum FormulaOP {
    NOTHING,
    CELLREF,
    SUM,
    POWER,
    SQRT,
    MOD,
    ABS,
    ROUND,
    FLOOR,
    CEIL,
    PI
};

// TODO: enum FormulaParserError

enum SmartFillError {
    SFE_NONE,

    // standard errors
    NODATA,
    TWODIMRANGE,
    INVALIDDATA,
    BADPATTERN,
    MIXED_TYPES,

    // formula fill errors
    NOCELLREFS,

    // debug
    NOTIMPLEMENTED
};

enum SmartFillOperation {
    SFO_NONE,
    ARITHMETIC_PROGRESSION,
    GEOMETRICAL_PROGRESSION,
    FORMULA_OFFSET,
    COPY
};

enum FillDirection {
    ROW_POS,  // left to right
    ROW_NEG,  // right to left
    COL_POS,  // top to bottom
    COL_NEG   // bottom to top
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
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onSelectionChangedSlot(const QItemSelection &selected, const QItemSelection &deselected);
    void onFormulaBarEdited();

private:
    QTableView *view;
    QLineEdit* formulaBar;

    void averageBtn();
    void sumBtn();
    void smartFillBtn();

    float averageOp(bool* ok); // range average
    float sumOp(bool* ok); // range sum
    int countOp(); // count of items in range

    void smartFillOperation(SmartFillError& error);
    void formulaSmartFill(SmartFillError& error, int rowCount, int colCount);

    QLabel *labelAverage;
    QLabel *labelSum;
    QLabel *labelCount;
    float parseFormula(QString formula, bool* err, QSet<QPair<int,int>>& dependencies);
    float parseOperation(FormulaOP operation, std::vector<QString> args, bool* error, QSet<QPair<int,int>>& dependencies);
    FormulaOP strToOp(QString str);
    std::vector<QString> tokenizeFormula(QString formula);
    std::vector<QString> evaluateExpression(std::vector<QString> tokens, bool* err, QSet<QPair<int,int>>& dependencies);

    QString getErrorMessage(SmartFillError error);
};
#endif // WIDGET_H
