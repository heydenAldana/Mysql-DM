#include "dbms_main.h"
#include "./ui_dbms_main.h"
#include "dbms_connhandler.h"
#include "dbhandler.h"

dbms_main::dbms_main(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::dbms_main)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    // Deshabilitar inputs y btns ya que no hay conexiones
    changeToolsState(0);
}

dbms_main::~dbms_main()
{
    delete ui;
}

void dbms_main::on_btnAddConn_clicked()
{
    dbms_connHandler uiConnConfig(this);

    if (uiConnConfig.exec() == QDialog::Accepted) {
        dbHandler* newConn = uiConnConfig.getHandler();
        if (newConn) {
            // Revisar si ya existe en la lista de conexiones
            bool connExists = false;
            for (dbHandler* existingConn : activeConnList) {
                if (existingConn->getServerName() == newConn->getServerName() &&
                    existingConn->getDbName() == newConn->getDbName() &&
                    existingConn->getDbUsername() == newConn->getDbUsername()) {
                    connExists = true;
                    break;
                }
            }
            // Crear nueva conexion o rechazar conexion nueva si ya existe
            if (connExists) {
                qDebug() << "La conexion ya existe en la lista. No se instanciará";
                newConn->disconnectSession();
                delete newConn;
            } else {
                activeConnList.append(newConn);
                qDebug() << "Nueva conexion agregada. Total:" << activeConnList.size();
                updateConnTree();
                changeToolsState(1);
            }
        }
    }
}


void dbms_main::on_btnDeleteConn_clicked()
{
    if(!activeConnList.isEmpty()) {
        dbHandler* lastConn = activeConnList.takeLast();
        lastConn->disconnectSession();
        delete lastConn;
        updateConnTree();
        qDebug() << "Sesión finalizada y removida de la lista.";
    } else {
        qDebug() << "No hay sesiones activas para cerrar.";
    }
}

void dbms_main::updateConnTree()
{
    ui->twDatabaseConn->clear();
    for (dbHandler* conn : activeConnList) {
        QTreeWidgetItem* connItem = new QTreeWidgetItem(ui->twDatabaseConn);
        connItem->setText(0, conn->getDbUsername() + "@" + conn->getServerName() + " [" + conn->getDbName() + "]");
        QTreeWidgetItem* dbItem = new QTreeWidgetItem(connItem);
        dbItem->setText(0, "Base de Datos: " + conn->getDbName());
        QTreeWidgetItem* connIdItem = new QTreeWidgetItem(connItem);
        connIdItem->setText(0, "ID: " + conn->getConnId());
    }
}

void dbms_main::changeToolsState(bool setActive)
{
    if (setActive) {
        ui->btnEditConn->setEnabled(1);
        ui->btnDeleteConn->setEnabled(1);
        ui->btnDeleteAllConn->setEnabled(1);
        ui->btnExecuteSql->setEnabled(1);
        ui->btnCopySql->setEnabled(1);
        ui->btnAnalyzeSql->setEnabled(1);
        ui->pteSqlCommand->setEnabled(1);
    } else {
        ui->btnEditConn->setEnabled(0);
        ui->btnDeleteConn->setEnabled(0);
        ui->btnDeleteAllConn->setEnabled(0);
        ui->btnExecuteSql->setEnabled(0);
        ui->btnCopySql->setEnabled(0);
        ui->btnAnalyzeSql->setEnabled(0);
        ui->pteSqlCommand->setEnabled(0);
    }
}


void dbms_main::on_btnDeleteAllConn_clicked()
{
    if (activeConnList.isEmpty()) {
        qDebug() << "No hay conexiones activas para eliminar.";
        return;
    }
    for (dbHandler* existingConn : activeConnList) {
        qDebug() << "Sesion con ID " << existingConn->getConnId() << "Desconectada";
        existingConn->disconnectSession();
        delete existingConn;
    }
    activeConnList.clear();
    updateConnTree();
    qDebug() << "TODAS las conexiones fueron removidas de la memoria y de la lista";
    changeToolsState(0);
}

