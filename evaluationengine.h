#ifndef EVALUATIONENGINE_H
#define EVALUATIONENGINE_H

#include <vector>
#include <QString>

class EvaluationEngine final
{
public:
    EvaluationEngine() = delete;
    static float evaluate(std::vector<QString> operations, bool* err);
};

#endif // EVALUATIONENGINE_H
