#ifndef DBMS_MAIN_H
#define DBMS_MAIN_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QStandardItemModel>
#include "dbhandler.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class dbms_main;
}
QT_END_NAMESPACE

class dbms_main : public QMainWindow
{
    Q_OBJECT

public:
    dbms_main(QWidget *parent = nullptr);
    ~dbms_main();

private slots:
    void on_btnAddConn_clicked();
    void on_btnDeleteConn_clicked();

    void on_btnDeleteAllConn_clicked();

    void on_twDatabaseConn_itemClicked(QTreeWidgetItem *item, int column);

    void on_btnExecuteSql_clicked();

    void on_btnExportDDL_clicked();

    void onDataViewContextMenu(const QPoint& pos);

    void on_btnCleanSqlCommand_clicked();

    void on_twDataView_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_btnCreateTable_clicked();

    void on_btnCopySql_clicked();

    void on_btnCreateView_clicked();

    void on_btnEditConn_clicked();

private:
    Ui::dbms_main *ui;
    QList<dbHandler*> activeConnList;
    dbHandler* activeConn = nullptr;
    void updateConnTree();
    void showQueryResults(QSqlQuery& query);
    void changeToolsState(bool setActive);
    void refreshDbInfo(dbHandler* handler);
    void showStatusMessage(const QString& msg, bool isError);
    QString generateDDL(QTreeWidgetItem* item);
    void loadSavedConnections();
};
#endif // DBMS_MAIN_H
