#include "dbms_create_table.h"
#include "ui_dbms_create_table.h"

dbms_create_table::dbms_create_table(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::dbms_create_table)
{
    ui->setupUi(this);
}

dbms_create_table::~dbms_create_table()
{
    delete ui;
}
