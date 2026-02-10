#ifndef DBMS_CONNHANDLER_H
#define DBMS_CONNHANDLER_H

#include <QDialog>
#include "dbhandler.h"


namespace Ui {
class dbms_connHandler;
}

class dbms_connHandler : public QDialog
{
    Q_OBJECT

public:
    explicit dbms_connHandler(QWidget *parent = nullptr);
    ~dbms_connHandler();
    dbHandler* getHandler() const { return handler; }

private slots:
    void on_btnCancel_clicked();
    void on_btnConnect_clicked();

private:
    Ui::dbms_connHandler *ui;
    dbHandler *handler;
};

#endif // DBMS_CONNHANDLER_H
