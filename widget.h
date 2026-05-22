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
#include <QTimer>
#include <QApplication>
#include <QColorDialog>
#include <QFileDialog>
#include "global.h"
#include "tablemodel.h"

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

enum FormulaParserError {
    FPE_NONE,
    MATH_EVALUATION_ERROR,
    INCORRECT_ARGUMENT_COUNT,
    NON_NUMERIC_VALUE,
    INVALID_SYNTAX
};

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
    bool eventFilter(QObject *obj, QEvent *event);
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QTableView *view;
    QLineEdit* formulaBar;

    void averageBtn();
    void sumBtn();
    void smartFillBtn();
    void undoBtn();
    void redoBtn();
    void bgColorBtn();
    void fgColorBtn();
    bool saveBtn();
    bool loadBtn();
    void newBtn();

    bool isSaved = false;
    bool modifiedSinceSave = false;
    QString filename;

    float averageOp(bool* ok); // range average
    float sumOp(bool* ok); // range sum
    int countOp(); // count of items in range

    void smartFillOperation(SmartFillError& error, SmartFillOperation& operation);
    void formulaSmartFill(SmartFillError& error, int rowCount, int colCount);

    QLabel* statusBarText;
    QLabel *labelAverage;
    QLabel *labelSum;
    QLabel *labelCount;

    float parseFormula(QString formula, FormulaParserError& err, QSet<QPair<int,int>>& dependencies);
    float parseOperation(FormulaOP operation, std::vector<QString> args, FormulaParserError& error, QSet<QPair<int,int>>& dependencies);
    FormulaOP strToOp(QString str);
    std::vector<QString> tokenizeFormula(QString formula);
    std::vector<QString> evaluateExpression(std::vector<QString> tokens, FormulaParserError& err, QSet<QPair<int,int>>& dependencies);

    QString getErrorMessage(SmartFillError error);
    QString getErrorMessage(FormulaParserError error);

    QString getCellError(FormulaParserError error);
    FormulaParserError STR2FPE(QString cellError);

    void pushStatusMessage(QString message);
    QTimer* statusMessageTimer;
    void statusMessageTimerAction();
};
#endif // WIDGET_H
