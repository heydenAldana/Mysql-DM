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
}

dbms_main::~dbms_main()
{
    delete ui;
}

void dbms_main::on_btnAddConn_clicked()
{
    // PENDIENTE: Verificar no haya sesiones DUPLICADAS.

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
        qDebug() << "Sesión finalizada y removida de la lista.";
    } else {
        qDebug() << "No hay sesiones activas para cerrar.";
    }
}

