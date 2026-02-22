#include "dbms_connhandler.h"
#include "ui_dbms_connhandler.h"

dbms_connHandler::dbms_connHandler(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::dbms_connHandler)
    , handler(nullptr)
{
    ui->setupUi(this);
}

dbms_connHandler::~dbms_connHandler()
{
    delete ui;
}

void dbms_connHandler::on_btnCancel_clicked()
{
    this->reject();
}

void dbms_connHandler::on_btnConnect_clicked()
{
    handler = new dbHandler();
    bool success = handler->startSession(
        ui->leServerName->text(),
        ui->leDatabaseName->text(),
        ui->leUsername->text(),
        ui->lePassword->text()
        // ui->lePort->text()
        );

    if (success) {
        qDebug() << "Conexion exitosa";
        this->accept();
    } else {
        ui->lMessage->setText("Error: " + handler->getDbErrorMsg());
        qDebug() << "Conexion fallida: " << handler->getDbErrorMsg();
        delete handler;
        handler = nullptr;
    }
}

void dbms_connHandler::prefillData(const QString& server, const QString& db, const QString& user,  const QString& password)
{
    ui->leServerName->setText(server);
    ui->leDatabaseName->setText(db);
    ui->leUsername->setText(user);
    ui->lePassword->setText(password);
}
