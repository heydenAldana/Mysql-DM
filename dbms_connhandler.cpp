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


void dbms_connHandler::on_btnConnect_clicked()
{
    // Identificador unico para conexion
    QString nombreConexion = "test";
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", nombreConexion);

    // Parametros de conexion
    QString dsn = QString("DRIVER={MariaDB Unicode};"
                          "SERVER=%1;"
                          "DATABASE=%2;"
                          "UID=%3;"
                          "PWD=%4;"
                          "OPTION=3;")
                      .arg(ui->leServerName->text())
                      .arg(ui->leDatabaseName->text())
                      .arg(ui->leUsername->text())
                      .arg(ui->lePassword->text());
    db.setDatabaseName(dsn);

    if (db.open()) {
        qDebug() << "Conexión exitosa";
        this->accept();
    } else {
        qDebug() << "Error crítico: " << db.lastError().text();
        ui->lMessage->setText(db.lastError().text());
    }
}

