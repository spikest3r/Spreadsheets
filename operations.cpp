#include "widget.h"
#include "evaluationengine.h"

#include <QMessageBox>

float Widget::averageOp() {
    QItemSelectionModel *sel = view->selectionModel();
    QModelIndexList indexes = sel->selectedIndexes();

    float average = 0.0f;
    int count = 0;

    for (const QModelIndex &index : indexes) {
        int row = index.row();
        int col = index.column();
        QVariant value = index.data();

        bool ok = false;
        float v = value.toFloat(&ok);
        if(!ok) continue; // TODO: Illegal value warning
        average += v;
        count++;
    }

    average /= (float)count;
    return average;
}

float Widget::parseFormula(QString formula, bool* err) {
    // break into tokens
    std::vector<QString> tokens;
    QString token = "";
    bool isParOpen = false;
    for(QChar c: formula) {
        if(c == '=') continue; // drop equals

        if(c == '(' || c == ')') {
            if((c == '(' && isParOpen) || (c == ')' && !isParOpen)) {
                // invalid syntax
                *err = true;
                return 0.0f;
            }
            isParOpen = c == '(' ? true : false;
            tokens.push_back(token);
            tokens.push_back(c);
            token = "";
            continue;
        } else if(c == ',') {
            if(isParOpen) {
                tokens.push_back(token);
                token = "";
            } else {
                // invalid syntaxx
                *err = true;
                return 0.0f;
            }
            continue;
        } else if(c == '+' || c == '-' || c == '*' || c == '/') {
            tokens.push_back(c);
            continue;
        }

        token += c;
    }

    // parse tokens into math engine ready ops
    std::vector<QString> operations;
    FormulaOP activeOp;
    bool isOpActive = false;
    std::vector<QString> args;
    for(QString token: tokens) {
        if(!isOpActive) {
            if(token == "C" || token == "CELL") {
                // CELLREF function
                activeOp = CELLREF;
                isOpActive = true;
            } else if(token == "+" || token == "-" || token == "*" || token == "/") {
                operations.push_back(token);
                continue;
            } else {
                // Unknown function
                *err = true;
                return 0.0f;
            }
        } else {
            if(token == "(") {
                if(isOpActive) continue;
            } else if(token == ")") {
                // parse operation
                bool error = false;
                float opResult = parseOperation(activeOp, args, &error);
                if(!error) {
                    args.clear();
                    isOpActive = false;
                    operations.push_back(QString("%0").arg(opResult));
                } else {
                    // operation evaluation error
                    *err = true;
                    return 0.0f;
                }
                continue;
            }
            args.push_back(token);
        }
    }

    // compute result
    float evalResult = EvaluationEngine::evaluate(operations, err);
    if(*err) return 0.0f; // evaluation error

    return evalResult;
}

float Widget::parseOperation(FormulaOP operation, std::vector<QString> args, bool* error) {
    switch(operation) {
    case NOTHING:
        return 0.0f;
    case CELLREF:
    {
        if(args.size() != 2) {
            *error = true;
            return 0.0f;
        }
        float row = args[0].toFloat();
        float col = args[1].toFloat();
        QVariant v = view->model()->data(view->model()->index(row, col), Qt::DisplayRole);
        float value = v.toFloat();
        return value;
    }
    }
}
