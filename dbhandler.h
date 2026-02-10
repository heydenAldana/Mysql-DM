#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

class dbHandler
{
private:
    QString serverName, dbName, dbUsername;
    QString dbErrorMsg, connId;
public:
    dbHandler();
    ~dbHandler();
    bool startSession(QString server="", QString dbName="", QString dbusername="", QString dbPassword="", QString port="3306");
    void disconnectSession();

    void setDbErrorMsg(const QString dbErrorMsg) { this->dbErrorMsg = dbErrorMsg; }
    QString getDbErrorMsg() const { return this->dbErrorMsg; }
    void setConnId(const QString connId) { this->connId = connId; }
    QString getConnId() const { return this->connId; }
    QString getServerName() const { return serverName; }
    QString getDbName() const { return dbName; }
    QString getDbUsername() const { return dbUsername; }
};

#endif // DBHANDLER_H
