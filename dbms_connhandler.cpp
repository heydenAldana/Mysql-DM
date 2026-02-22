#include "dbms_connhandler.h"
#include "ui_dbms_connhandler.h"

dbms_connHandler::dbms_connHandler(QWidget *parent, Mode mode)
    : QDialog(parent)
    , ui(new Ui::dbms_connHandler)
    , handler(nullptr)
    , currentMode(mode)
{
    ui->setupUi(this);
    ui->btnConnect->setFocus();
    applyMode();
}

dbms_connHandler::~dbms_connHandler()
{
    delete ui;
}

void dbms_connHandler::applyMode()
{
    switch (currentMode) {
    case ModeNewConnection:
        setWindowTitle("Nueva conexión");
        break;

    case ModeEditConnection:
        setWindowTitle("Editar conexión");
        break;

    case ModeValidate:
        setWindowTitle("Autenticar conexión");
        // Bloquear todos los campos excepto contraseña
        ui->leServerName->setReadOnly(true);
        ui->leDatabaseName->setReadOnly(true);
        ui->leUsername->setReadOnly(true);
        ui->lePassword->setReadOnly(false);
        ui->lePassword->clear();
        // Estilo visual para indicar campos bloqueados
        QString lockedStyle = "background-color: #2a2a2a; color: #888888;";
        ui->leServerName->setStyleSheet(lockedStyle);
        ui->leDatabaseName->setStyleSheet(lockedStyle);
        ui->leUsername->setStyleSheet(lockedStyle);
        break;
    }
}

void dbms_connHandler::prefillData(const QString& server, const QString& db, const QString& user, const QString& password)
{
    ui->leServerName->setText(server);
    ui->leDatabaseName->setText(db);
    ui->leUsername->setText(user);
    if (currentMode != ModeValidate)
        ui->lePassword->setText(password);
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
        );
    if (success) {
        this->accept();
    } else {
        ui->lMessage->setText("Error: " + handler->getDbErrorMsg());
        delete handler;
        handler = nullptr;
    }
}
