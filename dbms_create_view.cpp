#include "dbms_create_view.h"
#include "ui_dbms_create_view.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QComboBox>
#include <QLineEdit>
#include <QHeaderView>

dbms_view_creation::dbms_view_creation(dbHandler* handler, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::dbms_view_creation)
    , handler(handler)
{
    ui->setupUi(this);
    setWindowTitle("Crear nueva vista");
    setupCriteriaGrid();
    loadTables();

    // Actualiza columnas y preview cuando cambie la selección de tablas
    connect(ui->lwTables, &QListWidget::itemChanged,
            this, &dbms_view_creation::onTableSelectionChanged);
    // Actualiza preview cuando cambien columnas seleccionadas
    connect(ui->lwColumns, &QListWidget::itemChanged,
            this, &dbms_view_creation::updateSqlPreview);
    // Actualiza preview cuando cambie el nombre de la vista
    connect(ui->leViewName, &QLineEdit::textChanged,
            this, &dbms_view_creation::updateSqlPreview);
    // Detecta cuando el usuario termina de escribir en 'Valor'
    connect(ui->twCriteria, &QTableWidget::itemChanged,
            this, &dbms_view_creation::updateSqlPreview);
}

dbms_view_creation::~dbms_view_creation()
{
    delete ui;
}

void dbms_view_creation::setupCriteriaGrid()
{
    QStringList headers = {"Tabla", "Campo", "Operador", "Valor"};
    ui->twCriteria->setColumnCount(4);
    ui->twCriteria->setHorizontalHeaderLabels(headers);
    ui->twCriteria->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->twCriteria->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->twCriteria->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->twCriteria->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void dbms_view_creation::loadTables()
{
    ui->lwTables->clear();
    QSqlDatabase db = QSqlDatabase::database(handler->getConnId());
    if (!db.isOpen()) return;
    QSqlQuery query(db);
    QString sql = QString("SHOW FULL TABLES IN `%1` WHERE Table_type = 'BASE TABLE'")
                      .arg(handler->getDbName());

    if (query.exec(sql)) {
        while (query.next()) {
            QListWidgetItem* item = new QListWidgetItem(query.value(0).toString());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            ui->lwTables->addItem(item);
        }
    }
}

void dbms_view_creation::loadColumnsForSelectedTables()
{
    ui->lwColumns->clear();
    QStringList selectedTables = getSelectedTables();
    if (selectedTables.isEmpty()) return;
    QSqlDatabase db = QSqlDatabase::database(handler->getConnId());
    if (!db.isOpen()) return;

    for (const QString& table : selectedTables) {
        QSqlQuery query(db);
        // SHOW COLUMNS para metadata de columnas.
        if (query.exec(QString("SHOW COLUMNS FROM `%1`.`%2`")
                           .arg(handler->getDbName()).arg(table))) {
            while (query.next()) {
                QString colName = query.value(0).toString(); // campo
                QString colType = query.value(1).toString(); // tipo
                QString label   = QString("%1.%2  (%3)").arg(table).arg(colName).arg(colType);
                QListWidgetItem* item = new QListWidgetItem(label);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
                // Guarda "tabla.columna" limpio para el SQL
                item->setData(Qt::UserRole, QString("%1.%2").arg(table).arg(colName));
                ui->lwColumns->addItem(item);
            }
        }
    }
}

QStringList dbms_view_creation::getSelectedTables()
{
    QStringList result;
    for (int i = 0; i < ui->lwTables->count(); i++) {
        QListWidgetItem* item = ui->lwTables->item(i);
        if (item->checkState() == Qt::Checked)
            result << item->text();
    }
    return result;
}

QStringList dbms_view_creation::getSelectedColumns()
{
    QStringList result;
    for (int i = 0; i < ui->lwColumns->count(); i++) {
        QListWidgetItem* item = ui->lwColumns->item(i);
        if (item->checkState() == Qt::Checked)
            result << item->data(Qt::UserRole).toString();
    }
    return result;
}

QString dbms_view_creation::buildJoinClause(const QStringList& tables)
{
    if (tables.size() < 2) return "";
    // SHOW CREATE TABLE para extraer las FK manualmente.
    QSqlDatabase db = QSqlDatabase::database(handler->getConnId());
    QStringList joins;
    // Para cada par de tablas, busca FK que las relacione
    for (int i = 1; i < tables.size(); i++) {
        QString childTable = tables[i];
        QSqlQuery query(db);
        query.exec(QString("SHOW CREATE TABLE `%1`.`%2`")
                       .arg(handler->getDbName()).arg(childTable));
        if (!query.next()) continue;
        QString createSql = query.value(1).toString();

        // Buscar FOREIGN KEY ... REFERENCES tabla_padre(col)
        QRegularExpression re(
            "FOREIGN KEY \\(`(\\w+)`\\) REFERENCES `(\\w+)` \\(`(\\w+)`\\)",
            QRegularExpression::CaseInsensitiveOption
            );
        QRegularExpressionMatchIterator it = re.globalMatch(createSql);
        bool foundJoin = false;
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString childCol  = match.captured(1);
            QString parentTable = match.captured(2);
            QString parentCol   = match.captured(3);
            // Solo agrega JOIN si tabla padre está en las seleccionadas
            if (tables.contains(parentTable)) {
                joins << QString("INNER JOIN `%1` ON `%2`.`%3` = `%1`.`%4`")
                .arg(parentTable).arg(childTable)
                    .arg(childCol).arg(parentCol);
                foundJoin = true;
            }
        }
        // Si no se encontró FK automática, hacer CROSS JOIN
        if (!foundJoin)
            joins << QString("CROSS JOIN `%1`").arg(childTable);
    }
    return joins.join("\n  ");
}

QString dbms_view_creation::buildWhereClause()
{
    QStringList conditions;
    for (int row = 0; row < ui->twCriteria->rowCount(); row++) {
        QComboBox* tableBox  = qobject_cast<QComboBox*>(ui->twCriteria->cellWidget(row, 0));
        QComboBox* fieldBox  = qobject_cast<QComboBox*>(ui->twCriteria->cellWidget(row, 1));
        QComboBox* opBox     = qobject_cast<QComboBox*>(ui->twCriteria->cellWidget(row, 2));
        QTableWidgetItem* valItem = ui->twCriteria->item(row, 3);

        if (!tableBox || !fieldBox || !opBox || !valItem) continue;

        QString table = tableBox->currentText();
        QString field = fieldBox->currentText();
        QString op    = opBox->currentText();
        QString val   = valItem->text().trimmed();

        if (field.isEmpty() || val.isEmpty()) continue;

        // Formateo de valor (comillas)
        bool isNumeric;
        val.toDouble(&isNumeric);
        if (!isNumeric && val.toUpper() != "NULL") val = QString("'%1'").arg(val);
        conditions << QString("`%1`.`%2` %3 %4").arg(table).arg(field).arg(op).arg(val);
    }
    return conditions.join(" AND ");
}


QString dbms_view_creation::buildCreateViewSQL()
{
    QString viewName = ui->leViewName->text().trimmed();
    if (viewName.isEmpty()) return "-- Ingresa un nombre para la vista";
    QStringList tables  = getSelectedTables();
    QStringList columns = getSelectedColumns();
    if (tables.isEmpty())  return "-- Selecciona al menos una tabla";
    if (columns.isEmpty()) return "-- Selecciona al menos una columna";

    QString colList   = columns.join(",\n  ");
    QString fromClause = QString("`%1`.`%2`").arg(handler->getDbName()).arg(tables[0]);
    QString joinClause = buildJoinClause(tables);
    QString whereClause = buildWhereClause();
    QString sql = QString("CREATE VIEW `%1`.`%2` AS\nSELECT\n  %3\nFROM %4")
                      .arg(handler->getDbName())
                      .arg(viewName)
                      .arg(colList)
                      .arg(fromClause);

    if (!joinClause.isEmpty())
        sql += "\n  " + joinClause;
    QString where = buildWhereClause();
    if (!where.isEmpty()) {
        sql += "\n WHERE " + where;
    }
    sql += ";";
    return sql;
}

void dbms_view_creation::onTableSelectionChanged()
{
    loadColumnsForSelectedTables();
    updateSqlPreview();
}

void dbms_view_creation::updateSqlPreview()
{
    ui->ptePreviewSql->setPlainText(buildCreateViewSQL());
}

void dbms_view_creation::updateColumnCombo(int row, const QString& tableName)
{
    QComboBox* colBox = qobject_cast<QComboBox*>(ui->twCriteria->cellWidget(row, 1));
    if (!colBox) return;
    colBox->clear();
    if (tableColumnsMap.contains(tableName))
        colBox->addItems(tableColumnsMap[tableName]);
    else {
        // Si el mapa está vacío, intenta obtenerlas de la DB
        QSqlDatabase db = QSqlDatabase::database(handler->getConnId());
        QSqlQuery q(db);
        if (q.exec(QString("SHOW COLUMNS FROM `%1`").arg(tableName))) {
            QStringList columns;
            while (q.next()) columns << q.value(0).toString();
            tableColumnsMap[tableName] = columns;
            colBox->addItems(columns);
        }
    }
}

void dbms_view_creation::on_btnAddColumn_clicked()
{
    int row = ui->twCriteria->rowCount();
    ui->twCriteria->insertRow(row);
    // ComboBox para la Tabla (Col 0)
    QComboBox* tableBox = new QComboBox();
    tableBox->addItems(getSelectedTables());
    ui->twCriteria->setCellWidget(row, 0, tableBox);
    // ComboBox para el Campo (Col 1)
    QComboBox* columnBox = new QComboBox();
    ui->twCriteria->setCellWidget(row, 1, columnBox);
    // ComboBox para el Operador (Col 2)
    QComboBox* opBox = new QComboBox();
    opBox->addItems({"=", "!=", ">", "<", ">=", "<=", "LIKE"});

    ui->twCriteria->setCellWidget(row, 2, opBox);
    QTableWidgetItem* valItem = new QTableWidgetItem("");
    ui->twCriteria->setItem(row, 3, valItem);
    connect(tableBox, &QComboBox::currentTextChanged, [this, row](const QString& tName){
        this->updateColumnCombo(row, tName);
        this->updateSqlPreview();
    });

    connect(columnBox, &QComboBox::currentTextChanged, this, &dbms_view_creation::updateSqlPreview);
    connect(opBox, &QComboBox::currentTextChanged, this, &dbms_view_creation::updateSqlPreview);
    updateColumnCombo(row, tableBox->currentText());
    updateSqlPreview();
}
void dbms_view_creation::on_btnRemoveColumn_clicked()
{
    int row = ui->twCriteria->currentRow();
    if (row >= 0) {
        ui->twCriteria->removeRow(row);
        updateSqlPreview();
    }
}

void dbms_view_creation::on_btnCreateTable_clicked()
{
    QString sql = buildCreateViewSQL();
    if (sql.startsWith("--")) {
        ui->lMessage->setText(sql);
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(handler->getConnId());
    if (!db.isOpen()) {
        ui->lMessage->setText("ERROR: La conexión no está activa.");
        return;
    }
    QSqlQuery query(db);
    if (query.exec(sql)) {
        QMessageBox::information(this, "Éxito",
                                 QString("Vista '%1' creada correctamente.").arg(ui->leViewName->text().trimmed()));
        this->accept();
    } else
        ui->lMessage->setText("ERROR: " + query.lastError().text());
}

void dbms_view_creation::on_btnCancel_clicked()
{
    this->reject();
}
