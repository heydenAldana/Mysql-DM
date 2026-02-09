#include "dbms_connhandler.h"
#include "ui_dbms_connhandler.h"

dbms_connHandler::dbms_connHandler(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::dbms_connHandler)
{
    ui->setupUi(this);
}

dbms_connHandler::~dbms_connHandler()
{
    delete ui;
}

void dbms_connHandler::on_btnCancel_clicked()
{
    this->close();
}

