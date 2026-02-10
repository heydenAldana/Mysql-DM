#ifndef DBMS_CONNHANDLER_H
#define DBMS_CONNHANDLER_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

namespace Ui {
class dbms_connHandler;
}

class dbms_connHandler : public QDialog
{
    Q_OBJECT

public:
    explicit dbms_connHandler(QWidget *parent = nullptr);
    ~dbms_connHandler();

private slots:
    void on_btnCancel_clicked();

    void on_btnConnect_clicked();

private:
    Ui::dbms_connHandler *ui;

};

#endif // DBMS_CONNHANDLER_H
