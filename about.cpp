#include "about.h"
#include <QLabel>

about::about(QWidget *parent) : QDialog(parent) {
    setFixedSize(230,170);
    setWindowIcon(QIcon(":/icons/tomato"));

    QLabel *app = new QLabel("Spreadsheets v1.0",this);
    QLabel *author = new QLabel("Made by spikest3r",this);
    QLabel *repo = new QLabel("Open source project<br><a href=\"https://github.com/spikest3r/Spreadsheets\">GitHub</a>",this);
    QLabel *website = new QLabel("<a href=\"https://olehsheremeta.com\">https://olehsheremeta.com</a>",this);

    app->setGeometry(10,10,200,40);
    author->setGeometry(10,30,200,40);
    repo->setGeometry(10,80,200,40);
    website->setGeometry(10,130,200,40);

    repo->setTextFormat(Qt::RichText);
    website->setTextFormat(Qt::RichText);
    repo->setOpenExternalLinks(true);
    website->setOpenExternalLinks(true);

    QFont font = app->font();
    font.setBold(true);
    app->setFont(font);
}
