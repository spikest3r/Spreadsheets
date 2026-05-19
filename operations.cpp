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
    std::vector<QString> tokens = tokenizeFormula(formula);

    // parse tokens into math engine ready ops
    std::vector<QString> expression = evaluateExpression(tokens, err);
    if(*err) return 0.0f;

    // compute result
    float result = EvaluationEngine::evaluate(expression, err);
    if(*err) return 0.0f;

    return result;
}

float Widget::parseOperation(FormulaOP operation, std::vector<QString> args, bool* error) {
    if(operation == NOTHING) return 0.0f;

    // parse arguments
    int argIndex = 0;
    for(QString arg: args) {
        if(arg.contains('(')) {
            // argument is a formula

            std::vector<QString> tokens = tokenizeFormula(arg);

            std::vector<QString> expression = evaluateExpression(tokens, error);
            if(*error) return 0.0f;

            float result = EvaluationEngine::evaluate(expression, error);
            if(*error) return 0.0f;

            args[argIndex] = QString("%0").arg(result);
        }
        argIndex ++;
    }

    switch(operation) {
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
    case SUM: {
        float result = 0.0f;
        for(QString arg: args) {
            result += arg.toFloat();
        }
        return result;
    }
    }
}

std::vector<QString> Widget::tokenizeFormula(QString formula) {
    std::vector<QString> tokens;
    QString token = "";
    bool isParOpen = false;
    for(QChar c: formula) {
        if(c == '=') continue; // drop equals

        if(c == '(' || c == ')') {
            if(token.length() > 0) tokens.push_back(token.simplified());
            tokens.push_back(c);
            token = "";
            continue;
        } else if(c == ',') {
            if(token.length() > 0) tokens.push_back(token.simplified());
            tokens.push_back(",");
            token = "";
            continue;
        } else if(c == '+' || c == '-' || c == '*' || c == '/') {
            tokens.push_back(c);
            continue;
        }

        token += c;
    }
    return tokens;
}

std::vector<QString> Widget::evaluateExpression(std::vector<QString> tokens, bool* err) {
    std::vector<QString> operations;
    FormulaOP activeOp;
    bool isOpActive = false;
    bool isParOpen = false;
    std::vector<QString> args;
    QString arg = "";
    int parDepth = 0;
    for(QString token: tokens) {
        if(!isOpActive) {
            // check math signs
            if(token == "+" || token == "-" || token == "*" || token == "/") {
                operations.push_back(token);
                continue;
            }

            // next operation
            FormulaOP fop = strToOp(token);
            if(fop == NOTHING) {
                *err = true;
                return {};
            }
            activeOp = fop;
            isOpActive = true;
        } else {
            if(token == "(") {
                parDepth++;
                if(!isParOpen) {
                    isParOpen = true;
                    continue;
                }
            } else if(token == ")") {
                parDepth--;

                if(parDepth == 0) {
                    // push final argument
                    if(arg.length() > 0) {
                        args.push_back(arg);
                        arg = "";
                    }

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
                        return {};
                    }

                    continue;
                }
            } else if(token == ",") {
                // push argument
                if(parDepth < 2) {
                    args.push_back(arg);
                    arg = "";
                    continue;
                }
            }
            arg += token;
        }
    }
    return operations;
}

FormulaOP Widget::strToOp(QString str) {
    if(str == "C" || str == "CELL") {
        // CELLREF function
        return CELLREF;
    } else if(str == "SUM") {
        return SUM;
    } else {
        // probably a number or invalid token
        return NOTHING;
    }
}
