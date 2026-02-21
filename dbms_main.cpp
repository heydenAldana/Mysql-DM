#include "dbms_main.h"
#include "./ui_dbms_main.h"
#include "dbms_connhandler.h"
#include "dbhandler.h"
#include <QSqlQuery>
#include <QSqlError>

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
        ui->twDataView->clear();
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
        ui->btnExportDDL->setEnabled(1);
        ui->pteSqlCommand->setEnabled(1);
    } else {
        ui->btnEditConn->setEnabled(0);
        ui->btnDeleteConn->setEnabled(0);
        ui->btnDeleteAllConn->setEnabled(0);
        ui->btnExecuteSql->setEnabled(0);
        ui->btnCopySql->setEnabled(0);
        ui->btnExportDDL->setEnabled(0);
        ui->pteSqlCommand->setEnabled(0);
    }
}

void dbms_main::refreshDbInfo(dbHandler *handler)
{
    if (!handler) return;
    ui->twDataView->clear();
    QSqlDatabase db = QSqlDatabase::database(handler->getConnId());
    if (!db.isOpen()) return;

    QString dbName = handler->getDbName();
    QTreeWidgetItem* root = new QTreeWidgetItem(ui->twDataView);
    root->setText(0, "DB: " + dbName);
    root->setExpanded(true);

    // Funcion lambda para Agregar los objetos al twDataView
    auto addCategory = [&](const QString& title, const QString& query, int nameCol, const QString& emoji)
    {
        QTreeWidgetItem* catItem = new QTreeWidgetItem(root);
        catItem->setText(0, emoji + " " + title);
        QSqlQuery q(db);
        if (q.exec(query)) {
            while (q.next()) {
                QTreeWidgetItem* child = new QTreeWidgetItem(catItem);
                child->setText(0, "● " + q.value(nameCol).toString());
            }
        } else {
            QTreeWidgetItem* errItem = new QTreeWidgetItem(catItem);
            errItem->setText(0, "● ERROR: " + q.lastError().text());
        }
        catItem->setExpanded(false);
    };

    // Agregar categorias de objetos admitidos por MySQL
    addCategory("Tablas",
                QString("SHOW FULL TABLES IN `%1` WHERE Table_type = 'BASE TABLE'").arg(dbName),
                0, "📊");
    addCategory("Vistas",
                QString("SHOW FULL TABLES IN `%1` WHERE Table_type = 'VIEW'").arg(dbName),
                0, "🖼️");
    addCategory("Procedimientos",
                QString("SHOW PROCEDURE STATUS WHERE Db = '%1'").arg(dbName),
                1, "⚙️");
    addCategory("Funciones",
                QString("SHOW FUNCTION STATUS WHERE Db = '%1'").arg(dbName),
                1, "📝");
    addCategory("Triggers",
                QString("SHOW TRIGGERS FROM `%1`").arg(dbName),
                0, "⚡");
    addCategory("Índices",
                QString("SELECT DISTINCT index_name, table_name "
                        "FROM mysql.innodb_index_stats "
                        "WHERE database_name = '%1' "
                        "AND stat_name = 'size'").arg(dbName),
                0, "🔑");
    addCategory("Usuarios",
                "SELECT CONCAT(User, '@', Host) FROM mysql.user",
                0, "👤");
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
    ui->twDataView->clear();
    updateConnTree();
    qDebug() << "TODAS las conexiones fueron removidas de la memoria y de la lista";
    changeToolsState(0);
}

void dbms_main::on_twDatabaseConn_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item) return;
    // Solo reaccionar si es un item raíz (conexión)
    if (item->parent() != nullptr) return;
    int index = ui->twDatabaseConn->indexOfTopLevelItem(item);
    if (index >= 0 && index < activeConnList.size()) {
        refreshDbInfo(activeConnList[index]);
    }
}



