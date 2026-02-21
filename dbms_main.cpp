#include "dbms_main.h"
#include "./ui_dbms_main.h"
#include "dbms_connhandler.h"
#include "dbhandler.h"
#include "dbms_create_table.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QMenu>

dbms_main::dbms_main(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::dbms_main)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    // Deshabilitar inputs y btns ya que no hay conexiones
    changeToolsState(0);

    ui->twDataView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->twDataView, &QTreeWidget::customContextMenuRequested,
            this, &dbms_main::onDataViewContextMenu);
    connect(ui->twDataView, &QTreeWidget::itemDoubleClicked,
            this, &dbms_main::on_twDataView_itemDoubleClicked);
}

dbms_main::~dbms_main()
{
    delete ui;
}

QString dbms_main::generateDDL(QTreeWidgetItem* item)
{
    if (!item || !item->parent()) return "";
    if (!activeConn) return "";
    QSqlDatabase db = QSqlDatabase::database(activeConn->getConnId());
    if (!db.isOpen()) return "";

    QString parentText = item->parent()->text(0);
    QString objectName = item->text(0).remove("● ").trimmed();
    QString sql = "";
    QSqlQuery query(db);
    if (parentText.contains("Tablas"))
        sql = QString("SHOW CREATE TABLE `%1`").arg(objectName);
    else if (parentText.contains("Vistas"))
        sql = QString("SHOW CREATE VIEW `%1`").arg(objectName);
    else if (parentText.contains("Procedimientos"))
        sql = QString("SHOW CREATE PROCEDURE `%1`").arg(objectName);
    else if (parentText.contains("Funciones"))
        sql = QString("SHOW CREATE FUNCTION `%1`").arg(objectName);
    else if (parentText.contains("Triggers"))
        sql = QString("SHOW CREATE TRIGGER `%1`").arg(objectName);
    else if (parentText.contains("Usuarios")) {
        // SHOW CREATE USER necesita 'user'@'host'
        int atPos = objectName.lastIndexOf('@');
        QString user = objectName.left(atPos);
        QString host = objectName.mid(atPos + 1);
        sql = QString("SHOW CREATE USER '%1'@'%2'").arg(user).arg(host);
        if (query.exec(sql) && query.next())
            return query.value(0).toString();
    } else if (parentText.contains("Índices")) {
        // Recuperar table_name guardado en UserRole
        QString tableName = item->data(0, Qt::UserRole).toString();
        QString indexName = objectName.left(objectName.indexOf(" ("));
        sql = QString("SHOW INDEX FROM `%1`.`%2` WHERE Key_name = '%3'")
                  .arg(activeConn->getDbName())
                  .arg(tableName)
                  .arg(indexName);
        if (!query.exec(sql)) {
            showStatusMessage("ERROR: Fallo al leer índice: " + query.lastError().text(), true);
            return "";
        }
        // Reconstruir DDL agrupando columnas por Seq_in_index = Column_name
        bool isUnique = false;
        QString indexType = "BTREE";
        QMap<int, QString> columns;
        while (query.next()) {
            isUnique   = (query.value("Non_unique").toInt() == 0);
            indexType  = query.value("Index_type").toString();
            int seq    = query.value("Seq_in_index").toInt();
            columns[seq] = query.value("Column_name").toString();
        }

        if (columns.isEmpty()) return "";
        QStringList colList;
        for (const QString& col : columns)
            colList << "`" + col + "`";
        QString ddl = QString("CREATE %1INDEX `%2` ON `%3`.`%4` (%5) USING %6;")
                          .arg(isUnique ? "UNIQUE " : "")
                          .arg(indexName)
                          .arg(activeConn->getDbName())
                          .arg(tableName)
                          .arg(colList.join(", "))
                          .arg(indexType);
        return ddl;
    }
    else
        return "";
    if (query.exec(sql) && query.next())
        return query.value(1).toString();
    if (query.lastError().isValid())
        showStatusMessage("ERROR SQL: " + query.lastError().text(), true);
    return "";
}

void dbms_main::onDataViewContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = ui->twDataView->itemAt(pos);
    if (!item || !item->parent() || !item->parent()->parent()) return;
    QMenu contextMenu(this);
    QAction* actionDDL = contextMenu.addAction("Ver DDL");
    QAction* selected = contextMenu.exec(ui->twDataView->viewport()->mapToGlobal(pos));
    if (selected == actionDDL) {
        QString ddl = generateDDL(item);
        if (!ddl.isEmpty()) {
            ui->pteSqlCommand->setPlainText(ddl);
            showStatusMessage("DDL generado correctamente.", false);
        } else
            showStatusMessage("No se pudo generar el DDL para este objeto.", true);
    }
}

void dbms_main::on_btnExportDDL_clicked()
{
    QTreeWidgetItem* item = ui->twDataView->currentItem();
    // Validar que sea objeto concreto
    if (!item || !item->parent() || !item->parent()->parent()) {
        showStatusMessage("ERROR: El objeto seleccionado no es válido para exportar como DDL", true);
        return;
    }
    QString ddl = generateDDL(item);
    if (!ddl.isEmpty()) {
        ui->pteSqlCommand->setPlainText(ddl);
        showStatusMessage("DDL generado correctamente.", false);
    } else
        showStatusMessage("ERROR: No se pudo generar el DDL para este objeto.", true);
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
    /*addCategory("Índices",
                QString("SELECT DISTINCT index_name, table_name "
                        "FROM mysql.innodb_index_stats "
                        "WHERE database_name = '%1' "
                        "AND stat_name = 'size'").arg(dbName),
                0, "🔑");*/
    QTreeWidgetItem* idxCatItem = new QTreeWidgetItem(root);
    idxCatItem->setText(0, "🔑 Índices");
    QSqlQuery idxQuery(db);
    QString idxSql = QString(
                         "SELECT DISTINCT index_name, table_name "
                         "FROM mysql.innodb_index_stats "
                         "WHERE database_name = '%1' "
                         "AND stat_name = 'size' "
                         "AND index_name != 'PRIMARY'"   // PRIMARY ya está en el DDL de la tabla
                         ).arg(dbName);

    if (idxQuery.exec(idxSql)) {
        while (idxQuery.next()) {
            QString indexName = idxQuery.value(0).toString();
            QString tableName = idxQuery.value(1).toString();
            QTreeWidgetItem* child = new QTreeWidgetItem(idxCatItem);
            child->setText(0, indexName + " (en: " + tableName + ")");
            // Guardar table_name para poder reconstruir el DDL después
            child->setData(0, Qt::UserRole, tableName);
        }
    } else {
        QTreeWidgetItem* errItem = new QTreeWidgetItem(idxCatItem);
        errItem->setText(0, "Error: " + idxQuery.lastError().text());
    }
    idxCatItem->setExpanded(false);
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
    if (!item || item->parent() != nullptr) return;
    int index = ui->twDatabaseConn->indexOfTopLevelItem(item);
    if (index >= 0 && index < activeConnList.size()) {
        activeConn = activeConnList[index];
        refreshDbInfo(activeConn);
    }
}

void dbms_main::showStatusMessage(const QString& msg, bool isError)
{
    ui->pleSqlDebugger->clear();
    ui->pleSqlDebugger->appendPlainText(msg);
    ui->pleSqlDebugger->setStyleSheet(isError
                                        ? "color: #ff4444; font-weight: bold;"
                                        : "color: #44ff88; font-weight: bold;");
}

void dbms_main::on_btnExecuteSql_clicked()
{
    if (!activeConn) {
        showStatusMessage("ERROR: No hay conexión seleccionada.", true);
        return;
    }
    QString rawSql = ui->pteSqlCommand->toPlainText().trimmed();
    if (rawSql.isEmpty()) {
        showStatusMessage("ERROR: El campo SQL está vacío.", true);
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(activeConn->getConnId());
    if (!db.isOpen()) {
        showStatusMessage("ERROR: La conexión no está activa.", true);
        return;
    }

    // Dividir por ";" y limpiar comentarios y líneas vacías
    QStringList statements;
    for (QString stmt : rawSql.split(";")) {
        QStringList lines = stmt.split("\n");
        QStringList cleanLines;
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (!trimmed.startsWith("--") && !trimmed.isEmpty())
                cleanLines << line;
        }
        QString cleanStmt = cleanLines.join("\n").trimmed();
        if (!cleanStmt.isEmpty())
            statements << cleanStmt;
    }

    int successCount = 0;
    int errorCount = 0;
    QSqlQuery lastSelectQuery;
    bool hasSelectResult = false;
    for (const QString& stmt : statements) {
        QSqlQuery query(db);
        bool ok = query.exec(stmt);
        if (!ok) {
            errorCount++;
            showStatusMessage(
                QString("Error en sentencia %1: %2").arg(successCount + errorCount).arg(query.lastError().text()), true);
            return;
        }
        if (query.isSelect()) {
            lastSelectQuery = query;
            hasSelectResult = true;
        }
        successCount++;
    }
    if (hasSelectResult)
        showQueryResults(lastSelectQuery);
    else {
        ui->tvSqlOutput->setModel(nullptr);
        showStatusMessage(QString("Script ejecutado: %1 sentencia(s) completada(s) correctamente.").arg(successCount), false);
    }
    updateConnTree();
}

void dbms_main::showQueryResults(QSqlQuery& query)
{
    QSqlRecord record = query.record();
    int colCount = record.count();
    QStandardItemModel* model = new QStandardItemModel(this);
    // Encabezados de columna de tabla
    QStringList headers;
    for (int i = 0; i < colCount; i++)
        headers << record.fieldName(i);
    model->setHorizontalHeaderLabels(headers);
    // Filas de datos en tabla
    int row = 0;
    while (query.next()) {
        QList<QStandardItem*> rowItems;
        for (int col = 0; col < colCount; col++) {
            QStandardItem* item = new QStandardItem(query.value(col).toString());
            item->setEditable(false);
            rowItems.append(item);
        }
        model->appendRow(rowItems);
        row++;
    }
    // Reemplazar modelo anterior de tabla
    QAbstractItemModel* oldModel = ui->tvSqlOutput->model();
    ui->tvSqlOutput->setModel(model);
    if (oldModel && oldModel->parent() == this)
        delete oldModel;
    ui->tvSqlOutput->horizontalHeader()->setStretchLastSection(true);
    ui->tvSqlOutput->resizeColumnsToContents();
    updateConnTree();
    showStatusMessage(QString("Consulta exitosa. %1 fila(s) obtenidas.").arg(row), false);
}

void dbms_main::on_btnCleanSqlCommand_clicked()
{
    ui->pteSqlCommand->clear();
    ui->pleSqlDebugger->clear();
}


void dbms_main::on_twDataView_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || !item->parent() || !activeConn) return;
    QString parentText = item->parent()->text(0);
    // Solo actua si es Tabla o Vista
    if (parentText.contains("Tablas") || parentText.contains("Vistas")) {
        QString objectName = item->text(0).remove("● ").trimmed();
        QString sql = QString("SELECT * FROM `%1`;").arg(objectName);
        ui->pteSqlCommand->setPlainText(sql);
        on_btnExecuteSql_clicked();
        showStatusMessage(QString("Visualizando contenido de: %1").arg(objectName), false);
    }
}

void dbms_main::on_btnCreateTable_clicked()
{
    if (!activeConn) {
        showStatusMessage("ERROR: No ha seleccionado ninguna conexión para crear la tabla", true);
        return;
    }
    dbms_create_table dialog(activeConn, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshDbInfo(activeConn);
    }
}

