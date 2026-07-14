#include "functionreferencedialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

FunctionReferenceDialog::FunctionReferenceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Spreadsheets — Lumen functions");
    setModal(true);
    setFixedWidth(360);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    auto* title = new QLabel("Available functions");
    title->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(title);

    struct FuncInfo { QString sig; QString desc; };
    const QVector<FuncInfo> functions = {
                                          { "getCell <row> <col> &<var>", "Reads a cell's value into a variable." },
                                          { "setCell <row> <col> <value>", "Writes a value into a cell." },
                                          };

    for (const auto& f : functions) {
        auto* sig = new QLabel(f.sig);
        sig->setStyleSheet("font-family: monospace; font-weight: bold;");
        sig->setWordWrap(true);
        layout->addWidget(sig);

        auto* desc = new QLabel(f.desc);
        desc->setStyleSheet("color: gray;");
        desc->setWordWrap(true);
        layout->addWidget(desc);

        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setFrameShadow(QFrame::Sunken);
        layout->addWidget(sep);
    }

    auto* docsButton = new QPushButton("Open full Lumen docs");
    connect(docsButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://lumen.olehsheremeta.com/"));
    });
    layout->addWidget(docsButton);

    auto* closeButton = new QPushButton("Close");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton);
}