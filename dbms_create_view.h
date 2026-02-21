#ifndef DBMS_CREATE_VIEW_H
#define DBMS_CREATE_VIEW_H

#include <QWidget>

namespace Ui {
class dbms_view_creation;
}

class dbms_view_creation : public QWidget
{
    Q_OBJECT

public:
    explicit dbms_view_creation(QWidget *parent = nullptr);
    ~dbms_view_creation();

private:
    Ui::dbms_view_creation *ui;
};

#endif // DBMS_CREATE_VIEW_H
