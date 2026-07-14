#include "scriptingpanel.h"

#include <QPlainTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>

#include "widget.h"
#include "functionreferencedialog.h"
#include "lumen-inc/compiler.h"
#include "lumen-inc/vm.h"

ScriptingPanel* ScriptingPanel::s_instance = nullptr;
QVector<Script> ScriptingPanel::scripts;

void ScriptingPanel::initialize() {
    if (!s_instance) {
        s_instance = new ScriptingPanel();
    }
}

void ScriptingPanel::showPanel() {
    s_instance->show();
    s_instance->raise();
    s_instance->activateWindow();
    s_instance->updateView();
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
    setWindowTitle("Scripting - untitled.lmn");
    resize(700, 500);

    scripts.push_back({"untitled.lmn", ""});

    setupLayout();
    setupMenu();
}

void ScriptingPanel::setupLayout() {
    auto* splitter = new QSplitter(Qt::Vertical, this);

    textEdit_ = new QPlainTextEdit(splitter);
    textEdit_->setPlaceholderText("Write your script here...");

    connect(textEdit_, &QPlainTextEdit::textChanged, this, [this]() {
        scripts[selectedScript].code = textEdit_->toPlainText();
        Widget::instance->setModifiedFlag();
    });

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
    QMenu* file = menuBar()->addMenu("Script");

    scriptsMenu = file->addMenu("Scripts");
    addScriptsEntry(scripts[0].name, 0);

    QAction* newAction = file->addAction("New");
    connect(newAction, &QAction::triggered, this, &ScriptingPanel::newScript);

    file->addSeparator();

    QAction* renameAction = file->addAction("Rename");
    connect(renameAction, &QAction::triggered, this, &ScriptingPanel::renameScript);

    QAction* removeAction = file->addAction("Remove");
    connect(removeAction, &QAction::triggered, this, &ScriptingPanel::removeScript);

    file->addSeparator();

    // QAction* exportAction = file->addAction("Export");
    // connect(exportAction, &QAction::triggered, this, &ScriptingPanel::exportScript);

    // QAction* importAction = file->addAction("Import");
    // connect(importAction, &QAction::triggered, this, &ScriptingPanel::importScript);

    QAction* runAction = menuBar()->addAction("Run");
    runAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(runAction, &QAction::triggered, this, &ScriptingPanel::onRun);

    QAction* clearConsoleAction = menuBar()->addAction("Clear console");
    connect(clearConsoleAction, &QAction::triggered, this, &ScriptingPanel::clearConsole);

    QAction* docsAction = menuBar()->addAction("Lumen Docs");
    connect(docsAction, &QAction::triggered, this, &ScriptingPanel::openDocsUrl);
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
        QMessageBox::critical(this, "Scripting", "Compilation has failed!");
        return;
    }

    status = run(program.bytecode, program.stringPool, program.variableIndex);
}

void ScriptingPanel::openDocsUrl() {
    FunctionReferenceDialog dlg(this);
    dlg.exec();
}

void ScriptingPanel::newScript() {
    scripts.push_back({"untitled.lmn", ""});
    selectedScript++;
    addScriptsEntry(scripts[selectedScript].name, selectedScript);
    updateView();
    Widget::instance->setModifiedFlag();
}

void ScriptingPanel::renameScript() {
    bool ok = false;
    QString text = QInputDialog::getText(
        this,
        "Input",
        "Please provide new script name",
        QLineEdit::Normal,
        "", // default text
        &ok
        );

    if (ok && !text.trimmed().isEmpty()) {
        if(!text.endsWith(".lmn")) text += ".lmn";
        scripts[selectedScript].name = text;
        scriptsMenu->actions().at(selectedScript)->setText(text);
        updateView();
    } else if (ok) {
        // TODO: Status message
    }
    Widget::instance->setModifiedFlag();
}

void ScriptingPanel::removeScript() {
    if(scripts.size() == 1) {
        QMessageBox::warning(this, "Remove script", "There has to be at least 1 script");
        return;
    }
    auto result = QMessageBox::question(this, "Remove script", "Are you sure you want to remove script " + scripts[selectedScript].name);
    if(result == QMessageBox::Yes) {
        delete scriptsMenu->actions().at(selectedScript);
        scripts.removeAt(selectedScript);
        selectedScript = 0;
        updateView();
    }
    Widget::instance->setModifiedFlag();
}

void ScriptingPanel::updateView() {
    textEdit_->setPlainText(scripts[selectedScript].code);
    setWindowTitle("Scripting - " + scripts[selectedScript].name);
}

void ScriptingPanel::addScriptsEntry(const QString& name, const int& index) {
    QAction* scriptAction = scriptsMenu->addAction(name);
    connect(scriptAction, &QAction::triggered, this, [this, index]{
        selectedScript = index;
        updateView();
    });
}

void ScriptingPanel::updateMenu() {
    s_instance->selectedScript = 0;
    s_instance->scriptsMenu->clear();
    int i = 0;
    for(const auto& script: scripts) {
        s_instance->addScriptsEntry(script.name, i);
        i++;
    }
}