#ifndef GLOBAL_H
#define GLOBAL_H
#include <QVariant>
#include <QColor>

enum FillDirection {
    ROW_POS,  // left to right
    ROW_NEG,  // right to left
    COL_POS,  // top to bottom
    COL_NEG   // bottom to top
};

struct Cell {
    QVariant value;
    QColor cellColor;
    QColor textColor;
    quint8 fontFlags = 0;
    bool styled = false;
};

enum FontFlag {
    FF_Bold      = 1 << 0, // 0b00000001
    FF_Italic    = 1 << 1, // 0b00000010
    FF_Underline = 1 << 2, // 0b00000100
    FF_StrikeOut = 1 << 3, // 0b00001000
};
#endif // GLOBAL_H
