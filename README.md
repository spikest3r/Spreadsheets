# Spreadsheets

A simple Qt-based spreadsheet application written in C++.

![Screenshot, showcasing formatting, dependency graph, formulas, Smart Fill and status bar](<Screenshot 2026-05-21 211154.png>)

## Overview

This project implements a lightweight spreadsheet experience with:
- cell entry and navigation
- formula parsing and evaluation
- saving and loading spreadsheet data
- formatting (text and background colors, font styles)
- basic spreadsheet operations and Smart Fill (arithmetic and geometrical progressions, formula cell index offset)
- copy & paste

## Build Instructions

1. Open the project in Qt Creator using `Spreadsheets.pro`.
2. Configure a Qt 6 kit.
3. Build and run the project from Qt Creator.

Alternatively, from a Qt-enabled command prompt:

```powershell
cd d:\Projects\Spreadsheets
qmake Spreadsheets.pro
mingw32-make
```

## Files of Interest

- `main.cpp` - application entry point
- `widget.cpp` / `widget.h` - main UI and spreadsheet widget
- `formulaparser.cpp` - formula parsing logic
- `smartfill.cpp` - Smart Fill logic
- `evaluationengine.cpp` - formula evaluation engine
- `saveload.cpp` - save/load support
- `styles.cpp` - spreadsheet formatting logic
- `helpers.cpp` - various helpers (error messages etc.)
- `tablemodel.cpp` - spreadsheet data model

## TODO

- two-dimensional Smart Fill
- better Style Brush (pattern based)
- RANGE() function for formulas
- Find & Replace
- Print support

## License

See `LICENSE` for license details.
