#ifndef CONNFILE_H
#define CONNFILE_H

#include <QString>
#include <QList>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include "dbhandler.h"

class connFile
{
public:
    static QString filePath() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/connections.json";
    }

    static QString encrypt(const QString& plain) {
        const quint8 key = 0x5A;
        QByteArray bytes = plain.toUtf8();
        for (int i = 0; i < bytes.size(); i++)
            bytes[i] = bytes[i] ^ key;
        return QString::fromLatin1(bytes.toBase64());
    }

    static QString decrypt(const QString& encoded) {
        const quint8 key = 0x5A;
        QByteArray bytes = QByteArray::fromBase64(encoded.toLatin1());
        for (int i = 0; i < bytes.size(); i++)
            bytes[i] = bytes[i] ^ key;
        return QString::fromUtf8(bytes);
    }

    static bool saveConnections(const QList<dbHandler*>& list) {
        QJsonArray arr;
        for (dbHandler* conn : list) {
            QJsonObject obj;
            obj["server"]   = conn->getServerName();
            obj["database"] = conn->getDbName();
            obj["username"] = conn->getDbUsername();
            obj["password"] = encrypt(conn->getDbPassword()); // cifrado
            arr.append(obj);
        }

        QJsonDocument doc(arr);
        // Crea directorio si no existe
        QFileInfo fi(filePath());
        QDir().mkpath(fi.absolutePath());

        QFile file(filePath());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    struct ConnData {
        QString server, database, username, password;
    };

    static QList<ConnData> loadConnections() {
        QList<ConnData> result;
        QFile file(filePath());
        if (!file.open(QIODevice::ReadOnly)) return result;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (!doc.isArray()) return result;

        for (const QJsonValue& val : doc.array()) {
            QJsonObject obj = val.toObject();
            ConnData data;
            data.server   = obj["server"].toString();
            data.database = obj["database"].toString();
            data.username = obj["username"].toString();
            data.password = decrypt(obj["password"].toString());
            result << data;
        }
        return result;
    }
};

#endif // CONNFILE_H
