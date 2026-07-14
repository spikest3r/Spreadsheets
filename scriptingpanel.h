#pragma once

#include <QMainWindow>

#include "lumen-inc/programfile.h"
#include "global.h"

class QPlainTextEdit;

class ScriptingPanel : public QMainWindow {
    Q_OBJECT

public:
    static void showPanel();
    static void closeIfOpen();
    static void appendOutput(const QString& text); // call this from your VM's print hook
    static QVector<Script> scripts;
    static void initialize();
    static void updateMenu();

private:
    explicit ScriptingPanel(QWidget* parent = nullptr);
    ~ScriptingPanel() override = default;

    void setupMenu();
    void setupLayout();

    void clearConsole();
    void openDocsUrl();

    void newScript();
    void renameScript();
    void removeScript();
    // void exportScript();
    // void importScript();

    void updateView();
    void addScriptsEntry(const QString& name, const int& index);

    int selectedScript = 0;

    static ScriptingPanel* s_instance;

    QPlainTextEdit* textEdit_;
    QPlainTextEdit* console_;
    QMenu* scriptsMenu;

    BinaryProgram program;

private slots:
    void onRun();
};