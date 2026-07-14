#pragma once

#include <QMainWindow>

#include "lumen-inc/programfile.h"

class QPlainTextEdit;

class ScriptingPanel : public QMainWindow {
    Q_OBJECT

public:
    static void showPanel();
    static void closeIfOpen();
    static void appendOutput(const QString& text); // call this from your VM's print hook

private:
    explicit ScriptingPanel(QWidget* parent = nullptr);
    ~ScriptingPanel() override = default;

    void setupMenu();
    void setupLayout();

    void clearConsole();

    static ScriptingPanel* s_instance;

    QPlainTextEdit* textEdit_;
    QPlainTextEdit* console_;

    BinaryProgram program;

private slots:
    void onRun();
};