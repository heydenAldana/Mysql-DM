#include "dbms_create_table.h"
#include "ui_dbms_create_table.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QLayout>

dbms_create_table::dbms_create_table(dbHandler* handler, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::dbms_create_table)
    , handler(handler)
{
    ui->setupUi(this);
    setWindowTitle("Crear Tabla");
    setupColumnGrid();
    addColumnRow();
}

dbms_create_table::~dbms_create_table()
{
    delete ui;
}

void dbms_create_table::setupColumnGrid()
{
    QStringList headers = {"Nombre", "Tipo", "Longitud", "PK", "Not Null", "AI"};
    ui->twEditColumns->setColumnCount(headers.size());
    ui->twEditColumns->setHorizontalHeaderLabels(headers);
    ui->twEditColumns->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->twEditColumns->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->twEditColumns->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->twEditColumns->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->twEditColumns->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->twEditColumns->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->twEditColumns->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void dbms_create_table::addColumnRow()
{
    int row = ui->twEditColumns->rowCount();
    ui->twEditColumns->insertRow(row);
    // Col 0: Nombre (editable)
    ui->twEditColumns->setItem(row, 0, new QTableWidgetItem(""));
    // Col 1: Combobox para tipos
    QComboBox* typeBox = new QComboBox();
    typeBox->addItems({"INT", "VARCHAR", "CHAR", "TEXT", "TINYTEXT",
                       "DECIMAL", "FLOAT", "DOUBLE", "BOOLEAN",
                       "DATE", "DATETIME", "TIMESTAMP", "TIME",
                       "BIGINT", "SMALLINT", "TINYINT", "BLOB"});
    ui->twEditColumns->setCellWidget(row, 1, typeBox);
    // Col 2: Longitud
    ui->twEditColumns->setItem(row, 2, new QTableWidgetItem(""));
    // Cols 3, 4, 5: Checkboxes
    auto makeCheckBox = [&](int col) {
        QWidget* cell = new QWidget();
        QCheckBox* cb = new QCheckBox();
        QHBoxLayout* layout = new QHBoxLayout(cell);
        layout->addWidget(cb);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        ui->twEditColumns->setCellWidget(row, col, cell);
    };
    makeCheckBox(3); // PK
    makeCheckBox(4); // Not Null
    makeCheckBox(5); // AI
}

QString dbms_create_table::buildCreateTableSQL()
{
    QString tableName = ui->leTableName->text().trimmed();
    if (tableName.isEmpty()) return "";
    QStringList columnDefs;
    QStringList pkCols;
    for (int row = 0; row < ui->twEditColumns->rowCount(); row++) {
        QString colName = ui->twEditColumns->item(row, 0)->text().trimmed();
        if (colName.isEmpty()) continue;
        QComboBox* typeBox = qobject_cast<QComboBox*>(ui->twEditColumns->cellWidget(row, 1));
        QString type = typeBox ? typeBox->currentText() : "VARCHAR";
        QString length = ui->twEditColumns->item(row, 2) ? ui->twEditColumns->item(row, 2)->text().trimmed() : "";
        // Recuperar checkboxes desde su QWidget contenedor
        auto getCheck = [&](int col) -> bool {
            QWidget* cell = ui->twEditColumns->cellWidget(row, col);
            if (!cell) return false;
            QCheckBox* cb = cell->findChild<QCheckBox*>();
            return cb ? cb->isChecked() : false;
        };
        bool isPK    = getCheck(3);
        bool notNull = getCheck(4);
        bool isAI    = getCheck(5);
        // Tipos que requieren longitud
        QString fullType = type;
        if (!length.isEmpty() &&
            (type == "VARCHAR" || type == "CHAR" ||
             type == "DECIMAL" || type == "FLOAT" || type == "DOUBLE")) {
            fullType = QString("%1(%2)").arg(type).arg(length);
        }
        QString colDef = QString("`%1` %2").arg(colName).arg(fullType);
        if (notNull || isPK) colDef += " NOT NULL";
        if (isAI)            colDef += " AUTO_INCREMENT";
        if (isPK)            pkCols << "`" + colName + "`";
        columnDefs << colDef;
    }
    if (columnDefs.isEmpty()) return "";
    if (!pkCols.isEmpty())
        columnDefs << QString("PRIMARY KEY (%1)").arg(pkCols.join(", "));
    return QString("CREATE TABLE `%1`.`%2` (\n  %3\n) ENGINE=InnoDB;")
        .arg(handler->getDbName())
        .arg(tableName)
        .arg(columnDefs.join(",\n  "));
}

void dbms_create_table::on_btnCreateTable_clicked()
{
    QString sql = buildCreateTableSQL();
    if (sql.isEmpty()) {
        QMessageBox::warning(this, "Error", "Verifica que el nombre de la tabla y al menos una columna estén definidos.");
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(handler->getConnId());
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Error", "La conexión no está activa.");
        return;
    }
    QSqlQuery query(db);
    if (query.exec(sql)) {
        QMessageBox::information(this, "Éxito", QString("Tabla '%1' creada correctamente.").arg(ui->leTableName->text().trimmed()));
        this->accept();
    } else
        QMessageBox::critical(this, "Error al crear tabla", query.lastError().text());
}

void dbms_create_table::on_btnAddColumn_clicked()
{
    addColumnRow();
}

void dbms_create_table::on_btnRemoveColumn_clicked()
{
    ui->btnRemoveColumn->setEnabled(1);
    int row = ui->twEditColumns->currentRow();
    if (row >= 0)
        ui->twEditColumns->removeRow(row);
}

void dbms_create_table::on_btnCancel_clicked()
{
    this->reject();
}

