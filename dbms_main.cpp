#include "dbms_main.h"
#include "./ui_dbms_main.h"
#include "dbms_connhandler.h"


dbms_main::dbms_main(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::dbms_main)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
}

dbms_main::~dbms_main()
{
    delete ui;
}

void dbms_main::on_btnAddConn_clicked()
{
    dbms_connHandler uiConnConfig(this);
    uiConnConfig.exec();
}

