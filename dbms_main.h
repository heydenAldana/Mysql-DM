#ifndef DBMS_MAIN_H
#define DBMS_MAIN_H

#include <QMainWindow>

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

private:
    Ui::dbms_main *ui;
};
#endif // DBMS_MAIN_H
