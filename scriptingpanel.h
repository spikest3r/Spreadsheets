#pragma once

#include <QMainWindow>

#include "lumen-inc/programfile.h"
#include "global.h"
#include <QMutex>

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
    static void setRunActionText();
    void startOutputFlushTimer();

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
    QAction* runAction;

    static QString s_pendingOutput;
    static QMutex s_outputMutex;
    static QTimer* s_flushTimer;
    static constexpr int MAX_OUTPUT_CHARS = 1'000'000;
    static int outputChars;

    BinaryProgram program;

private slots:
    void onRun();
};
