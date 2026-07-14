#include "scriptingpanel.h"

#include <QPlainTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>

#include "lumen-inc/compiler.h"
#include "lumen-inc/vm.h"

ScriptingPanel* ScriptingPanel::s_instance = nullptr;

void ScriptingPanel::showPanel() {
    if (!s_instance) {
        s_instance = new ScriptingPanel();
    }
    s_instance->show();
    s_instance->raise();
    s_instance->activateWindow();
}

void ScriptingPanel::closeIfOpen() {
    if (s_instance) {
        s_instance->close();
    }
}

void ScriptingPanel::appendOutput(const QString& text) {
    if (s_instance) {
        QTextCursor cursor(s_instance->console_->document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text);
        s_instance->console_->setTextCursor(cursor); // keeps view scrolled to the inserted text
    }
}

ScriptingPanel::ScriptingPanel(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("ScriptingPanel");
    resize(700, 500);

    setupLayout();
    setupMenu();
}

void ScriptingPanel::setupLayout() {
    auto* splitter = new QSplitter(Qt::Vertical, this);

    textEdit_ = new QPlainTextEdit(splitter);
    textEdit_->setPlaceholderText("Write your script here...");

    console_ = new QPlainTextEdit(splitter);
    console_->setReadOnly(true);
    console_->setPlaceholderText("Output will appear here...");

    splitter->addWidget(textEdit_);
    splitter->addWidget(console_);
    splitter->setStretchFactor(0, 3); // editor gets more space
    splitter->setStretchFactor(1, 1); // console smaller by default

    setCentralWidget(splitter);
}

void ScriptingPanel::clearConsole() {
    if (s_instance) {
        s_instance->console_->clear();
    }
}

void ScriptingPanel::setupMenu() {
    QMenu* file = menuBar()->addMenu("File");

    QAction* runAction = menuBar()->addAction("Run");
    runAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(runAction, &QAction::triggered, this, &ScriptingPanel::onRun);

    QAction* clearConsoleAction = menuBar()->addAction("Clear console");
    connect(clearConsoleAction, &QAction::triggered, this, &ScriptingPanel::clearConsole);
}

void ScriptingPanel::onRun() {
    clearConsole();
    const QString source = textEdit_->toPlainText();

    program.bytecode.clear();
    program.stringPool.clear();
    program.variableIndex = 0;

    // compile source code
    int status = compile(
        source.toStdString(),
        program.bytecode,
        program.stringPool,
        program.variableIndex
    );

    if(status != 0) {
        // TODO: Export error message
        QMessageBox::critical(this, "Scripting", "Compilation has failed!");
        return;
    }

    status = run(program.bytecode, program.stringPool, program.variableIndex);

    program.save("/home/oleh/spr.bin");
}