#include "evaluationengine.h"
#include <cmath>

// evaluation algorith from https://github.com/spikest3r/PicoCalculator/blob/main/evaluate.c
// rewritten for qt c++

static bool isNum(const QString &s) {
    if (s.isEmpty()) return false;
    bool ok = false;
    s.toFloat(&ok);
    return ok;
}

static bool isOp(const QChar c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

static int getPrec(const QChar c) {
    if (c == '+' || c == '-') return 1;
    if (c == '*' || c == '/') return 2;
    return 0;
}

static std::vector<QString> tokenize(const QString &expr) {
    std::vector<QString> tokens;
    QString num;
    for (int i = 0; i < expr.size(); i++) {
        QChar ch = expr[i];
        if (ch == '-' && (i == 0 || isOp(expr[i - 1]))) {
            num += ch;
            continue;
        }
        if (ch.isDigit() || ch == '.') {
            num += ch;
        } else {
            if (!num.isEmpty()) {
                tokens.push_back(num);
                num.clear();
            }
            if (isOp(ch))
                tokens.push_back(QString(ch));
        }
    }
    if (!num.isEmpty())
        tokens.push_back(num);
    return tokens;
}

static std::vector<QString> shuntingYard(const std::vector<QString> &tokens) {
    std::vector<QString> out;
    std::vector<QChar> ops;
    for (const QString &t : tokens) {
        if (isNum(t)) {
            out.push_back(t);
        } else {
            QChar op = t[0];
            while (!ops.empty() && getPrec(ops.back()) >= getPrec(op))
                out.push_back(QString(ops.back())), ops.pop_back();
            ops.push_back(op);
        }
    }
    while (!ops.empty())
        out.push_back(QString(ops.back())), ops.pop_back();
    return out;
}

static float evalRPN(const std::vector<QString> &rpn, int *error) {
    std::vector<float> stack;
    for (const QString &t : rpn) {
        if (isNum(t)) {
            stack.push_back(t.toFloat());
        } else {
            if (stack.size() < 2) { *error = 1; return 0; }
            float b = stack.back(); stack.pop_back();
            float a = stack.back(); stack.pop_back();
            switch (t[0].toLatin1()) {
            case '+': stack.push_back(a + b); break;
            case '-': stack.push_back(a - b); break;
            case '*': stack.push_back(a * b); break;
            case '/': stack.push_back(a / b); break;
            }
        }
    }
    if (stack.empty()) { *error = 1; return 0; }
    return stack[0];
}

float EvaluationEngine::evaluate(std::vector<QString> operations, bool* err) {
    // join operations into a single expression string
    QString expr;
    for (const QString &op : operations)
        expr += op;

    int error = 0;
    auto tokens = tokenize(expr);
    auto rpn    = shuntingYard(tokens);
    auto result = evalRPN(rpn, &error);

    *err = error;
    return result;
}
