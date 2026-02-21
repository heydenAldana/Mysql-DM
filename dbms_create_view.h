#ifndef DBMS_CREATE_VIEW_H
#define DBMS_CREATE_VIEW_H

#include <QDialog>
#include <QListWidgetItem>
#include "dbhandler.h"

namespace Ui {
class dbms_view_creation;
}

class dbms_view_creation : public QDialog
{
    Q_OBJECT

public:
    explicit dbms_view_creation(dbHandler* handler, QWidget *parent = nullptr);
    ~dbms_view_creation();

private slots:
    void on_btnCreateTable_clicked();
    void on_btnCancel_clicked();
    void on_btnAddColumn_clicked();
    void on_btnRemoveColumn_clicked();
    void onTableSelectionChanged();
    void updateSqlPreview();

private:
    Ui::dbms_view_creation *ui;
    dbHandler* handler;
    QMap<QString, QStringList> tableColumnsMap;

    void updateColumnCombo(int row, const QString& tableName);
    void loadTables();
    void loadColumnsForSelectedTables();
    void setupCriteriaGrid();
    QStringList getSelectedTables();
    QStringList getSelectedColumns();
    QString buildJoinClause(const QStringList& tables);
    QString buildWhereClause();
    QString buildCreateViewSQL();
};

#endif // DBMS_CREATE_VIEW_H
