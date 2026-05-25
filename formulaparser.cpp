#include "widget.h"
#include "evaluationengine.h"

// Formula parsing

QString Widget::parseFormula(QString formula, FormulaParserError& err, QSet<QPair<int,int>>& dependencies) {
    // break into tokens
    std::vector<QString> tokens = tokenizeFormula(formula);

    // parse tokens into math engine ready ops
    std::vector<QString> expression = evaluateExpression(tokens, err, dependencies);
    if(err != FPE_NONE) {
        return "";
    }

    // compute result
    QString result;
    if(expression.front().contains(",")) {
        // range
        result = expression[0];
    } else {
        bool error = false;
        float evaluated = EvaluationEngine::evaluate(expression, &error);
        if(error)
        {
            error = MATH_EVALUATION_ERROR;
            return "";
        }
        result = QString::number(evaluated);
    }

    return result;
}

QString Widget::parseOperation(FormulaOP operation, std::vector<QString> args, FormulaParserError& error, QSet<QPair<int,int>>& dependencies) {
    if(operation == NOTHING) return "";

    // parse arguments
    auto tempArgs = args; // copy
    args.clear();
    for(QString arg: tempArgs) {
        if(!arg.contains(QRegularExpression("^\\$?\\d+$"))) {
            // argument is a formula

            std::vector<QString> tokens = tokenizeFormula(arg);

            std::vector<QString> expression = evaluateExpression(tokens, error, dependencies);
            if(error != FPE_NONE) {
                return "";
            }

            if(expression.front().contains(",")) {
                // range
                auto split = expression.front().split(",");
                for(auto& element: split) {
                    args.push_back(element);
                }
                continue;
            } else {
                bool error = false;
                float evaluated = EvaluationEngine::evaluate(expression, &error);
                if(error)
                {
                    error = MATH_EVALUATION_ERROR;
                    return "";
                }
                QString result = QString::number(evaluated);
                args.push_back(result);
                continue;
            }
        }
        args.push_back(arg);
    }

    switch(operation) {
    case CELLREF:
    {
        if(args.size() != 2) { error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok1, ok2;
        // leave out absolute ref marker for toFloat
        auto arg0 = args[0].replace("$", "");
        auto arg1 = args[1].replace("$", "");
        float row = arg0.toFloat(&ok1);
        float col = arg1.toFloat(&ok2);
        if(!ok1 || !ok2) { error = NON_NUMERIC_VALUE; return ""; }
        dependencies.insert({row, col});
        QVariant v = view->model()->data(view->model()->index(row, col), Qt::DisplayRole);
        return v.toString();
    }
    case SUM: {
        float result = 0.0f;
        for(QString arg: args) {
            bool ok;
            float v = arg.toFloat(&ok);
            if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
            result += v;
        }
        return QString::number(result);
    }
    case POWER:
    {
        float exp = 0;
        if(args.size() == 1) {
            exp = 2;
        } else if(args.size() == 2) {
            bool ok;
            exp = args[1].toFloat(&ok);
            if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
        } else {
            error = INCORRECT_ARGUMENT_COUNT;
            return "";
        }
        bool ok;
        float value = args[0].toFloat(&ok);
        if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
        return QString::number(std::pow(value, exp));
    }
    case SQRT:
    {
        if(args.size() != 1) { error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok;
        float value = args[0].toFloat(&ok);
        if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
        return QString::number(std::sqrt(value));
    }
    case MOD: {
        if(args.size() != 2) { error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok1, ok2;
        float value = args[0].toFloat(&ok1);
        float div = args[1].toFloat(&ok2);
        if(!ok1 || !ok2) { error = NON_NUMERIC_VALUE; return ""; }
        return QString::number(std::fmod(value, div));
    }
    case ABS: {
        if(args.size() != 1) { error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok;
        float value = args[0].toFloat(&ok);
        if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
        return QString::number(std::abs(value));
    }
    case ROUND: {
        if(args.size() != 2) { error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok1, ok2;
        float value = args[0].toFloat(&ok1);
        float dec = args[1].toFloat(&ok2);
        if(!ok1 || !ok2) { error = NON_NUMERIC_VALUE; return ""; }
        return QString::number(std::round(value * std::pow(10,dec)) / std::pow(10,dec));
    }
    case FLOOR: {
        if(args.size() != 1) { error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok;
        float value = args[0].toFloat(&ok);
        if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
        return QString::number(std::floor(value));
    }
    case CEIL: {
        if(args.size() != 1) { error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok;
        float value = args[0].toFloat(&ok);
        if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
        return QString::number(std::ceil(value));
    }
    case AVERAGE: {
        float result = 0.0f;
        for(QString arg: args) {
            bool ok;
            float v = arg.toFloat(&ok);
            if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
            result += v;
        }
        result /= (float)args.size();
        return QString::number(result);
        break;
    }
    case RANGE: {
        if(args.size() != 4) {error = INCORRECT_ARGUMENT_COUNT; return ""; }
        bool ok;
        int top = args[0].toInt(&ok);
        int left = args[1].toInt(&ok);
        int bottom = args[2].toInt(&ok);
        int right = args[3].toInt(&ok);
        if(!ok) { error = NON_NUMERIC_VALUE; return ""; }
        auto model = (TableModel*)view->model();
        QString result;
        for (int r = top; r <= bottom; r++) {
            for (int c = left; c <= right; c++) {
                auto cell = model->getCell(r, c);
                result += cell->value.toString();
                if(!(r == right && c == bottom)) {
                    result += ",";
                }
            }
        }
        return result;
    }
    case PI:
    {
        return QString::number(M_PI);
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
            if(token.length() > 0) tokens.push_back(token.simplified());
            tokens.push_back(c);
            token = "";
            continue;
        }

        token += c;
    }
    if(token.length() != 0) {
        tokens.push_back(token.simplified());
    }
    return tokens;
}

std::vector<QString> Widget::evaluateExpression(std::vector<QString> tokens, FormulaParserError& err, QSet<QPair<int,int>>& dependencies) {
    std::vector<QString> operations;
    FormulaOP activeOp;
    bool isOpActive = false;
    std::vector<QString> args;
    QString arg = "";
    int parDepth = 0;
    for(QString token: tokens) {
        if(token.length() == 0) continue; // skip malformed tokens
        if(!isOpActive) {
            // check math
            if(token.contains(QRegularExpression("^\\d+$"))) {
                operations.push_back(token);
                continue;
            }

            if(token.contains("+") || token.contains("-") || token.contains("*") || token.contains("/")) {
                operations.push_back(token);
                continue;
            }

            // next operation
            FormulaOP fop = strToOp(token);
            if(fop == NOTHING) {
                err = INVALID_SYNTAX;
                return {};
            }
            activeOp = fop;
            isOpActive = true;
        } else {
            if(token == "(") {
                parDepth++;
                if(parDepth < 2) continue;
            } else if(token == ")") {
                parDepth--;

                if(parDepth == 0) {
                    // push final argument
                    if(arg.length() > 0) {
                        args.push_back(arg);
                        arg = "";
                    }

                    // parse operation
                    QString opResult = parseOperation(activeOp, args, err, dependencies);
                    if(err == FPE_NONE) {
                        args.clear();
                        isOpActive = false;
                        operations.push_back(opResult);
                    } else {
                        // operation evaluation error
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
    } else if(str == "POWER" || str == "POW") {
        return POWER;
    } else if(str == "SQRT") {
        return SQRT;
    } else if(str == "MOD") {
        return MOD;
    } else if(str == "ABS") {
        return ABS;
    } else if(str == "ROUND") {
        return ROUND;
    } else if(str == "FLOOR") {
        return FLOOR;
    } else if(str == "CEIL") {
        return CEIL;
    } else if(str == "AVERAGE" || str == "AVG") {
        return AVERAGE;
    } else if(str == "RANGE") {
        return RANGE;
    } else if(str == "PI") {
        return PI;
    } else {
        // probably a number or invalid token
        return NOTHING;
    }
}
