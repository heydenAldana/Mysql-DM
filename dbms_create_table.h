#ifndef DBMS_CREATE_TABLE_H
#define DBMS_CREATE_TABLE_H

#include <QWidget>

namespace Ui {
class dbms_create_table;
}

class dbms_create_table : public QWidget
{
    Q_OBJECT

public:
    explicit dbms_create_table(QWidget *parent = nullptr);
    ~dbms_create_table();

private:
    Ui::dbms_create_table *ui;
};

#endif // DBMS_CREATE_TABLE_H
