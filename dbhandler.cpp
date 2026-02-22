#include "dbhandler.h"
#include <QUuid>

dbHandler::dbHandler() {
    this->dbErrorMsg = "";
    this->connId = "";
}

dbHandler::~dbHandler() {
    disconnectSession();
}

bool dbHandler::startSession(QString server, QString dbName, QString dbUsername, QString dbPassword, QString port)
{
    this->serverName = server;
    this->dbName = dbName;
    this->dbUsername = dbUsername;
    this->dbPassword   = dbPassword;
    dbErrorMsg = "";

    if(connId.isEmpty())
        setConnId(QUuid::createUuid().toString());
    if(QSqlDatabase::contains(connId))
        QSqlDatabase::removeDatabase(connId);

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", getConnId());
    QString connString = QString("DRIVER={MariaDB Unicode};SERVER=%1;DATABASE=%2;UID=%3;PWD=%4;OPTION=3;")
                             .arg(server)
                             .arg(dbName)
                             .arg(dbUsername)
                             .arg(dbPassword);
    db.setDatabaseName(connString);
    if (!db.open()) {
        dbErrorMsg = db.lastError().text();
        return false;
    }
    return true;
}

void dbHandler::disconnectSession()
{
    if(!connId.isEmpty() && QSqlDatabase::contains(connId)) {
        {
            QSqlDatabase db = QSqlDatabase::database(connId);
            if(db.isOpen())
                db.close();
        }
        QSqlDatabase::removeDatabase(connId);
    }
}
