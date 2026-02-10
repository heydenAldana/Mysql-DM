#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

class dbHandler
{
private:
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
};

#endif // DBHANDLER_H
