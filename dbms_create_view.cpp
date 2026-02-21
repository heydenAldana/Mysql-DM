#include "dbms_create_view.h"
#include "ui_dbms_create_view.h"

dbms_view_creation::dbms_view_creation(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::dbms_view_creation)
{
    ui->setupUi(this);
}

dbms_view_creation::~dbms_view_creation()
{
    delete ui;
}
