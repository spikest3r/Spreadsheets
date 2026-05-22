#include "widget.h"

QString Widget::getErrorMessage(SmartFillError error) {
    switch (error) {
    case SFE_NONE:       return "";
    case NODATA:         return "No data to fill from.";
    case TWODIMRANGE:    return "Select a single row or column.";
    case INVALIDDATA:    return "Selection contains non-numeric data.";
    case BADPATTERN:     return "No pattern detected. Try Simple Fill (Shift+F2).";
    case MIXED_TYPES: return "Selection mixes formulas and values.";
    case NOCELLREFS: return "Formula contains no cell references to offset.";
    case NOTIMPLEMENTED: return "Not implemented yet.";
    }
}

QString Widget::getErrorMessage(FormulaParserError error) {
    switch (error) {
    case MATH_EVALUATION_ERROR:    return "Mathematical calculation error";
    case INCORRECT_ARGUMENT_COUNT: return "Wrong number of arguments";
    case NON_NUMERIC_VALUE:        return "Expected a numeric value";
    case INVALID_SYNTAX:           return "Invalid formula syntax";
    case REFERENCE:                return "Invalid reference";
    case FPE_NONE:
    default:                       return "";
    }
}

QString Widget::getCellError(FormulaParserError error) {
    switch (error) {
    case MATH_EVALUATION_ERROR:    return "#NUM!";
    case INCORRECT_ARGUMENT_COUNT: return "#N/A";
    case NON_NUMERIC_VALUE:        return "#VALUE!";
    case INVALID_SYNTAX:           return "#ERROR!";
    case REFERENCE:                return "#REF!";
    case FPE_NONE:
    default:                       return "";
    }
}

FormulaParserError Widget::STR2FPE(QString str) {
    if(str == "#NUM!") return MATH_EVALUATION_ERROR;
    if(str == "#N/A") return INCORRECT_ARGUMENT_COUNT;
    if(str == "#VALUE!") return NON_NUMERIC_VALUE;
    if(str == "#ERROR!") return INVALID_SYNTAX;
    if(str == "#REF!") return REFERENCE;
    return FPE_NONE;
}
