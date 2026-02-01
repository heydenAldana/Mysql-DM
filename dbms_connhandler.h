#ifndef DBMS_CONNHANDLER_H
#define DBMS_CONNHANDLER_H

#include <QDialog>

namespace Ui {
class dbms_connHandler;
}

class dbms_connHandler : public QDialog
{
    Q_OBJECT

public:
    explicit dbms_connHandler(QWidget *parent = nullptr);
    ~dbms_connHandler();

private:
    Ui::dbms_connHandler *ui;
};

#endif // DBMS_CONNHANDLER_H
