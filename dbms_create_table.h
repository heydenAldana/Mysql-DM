#ifndef DBMS_CREATE_TABLE_H
#define DBMS_CREATE_TABLE_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidgetItem>
#include "dbhandler.h"

namespace Ui {
class dbms_create_table;
}

class dbms_create_table : public QDialog
{
    Q_OBJECT

public:
    explicit dbms_create_table(dbHandler* handler, QWidget *parent = nullptr);
    ~dbms_create_table();

private slots:
    void on_btnCreateTable_clicked();
    void on_btnCancel_clicked();
    void on_btnAddColumn_clicked();
    void on_btnRemoveColumn_clicked();

private:
    Ui::dbms_create_table *ui;
    dbHandler* handler;

    void setupColumnGrid();
    void addColumnRow();
    QString buildCreateTableSQL();
};

#endif // DBMS_CREATE_TABLE_H
