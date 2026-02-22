#ifndef DBMS_CONNHANDLER_H
#define DBMS_CONNHANDLER_H

#include <QDialog>
#include "dbhandler.h"

namespace Ui { class dbms_connHandler; }

class dbms_connHandler : public QDialog
{
    Q_OBJECT
public:
    enum Mode {
        ModeNewConnection,  // Todos los campos editables
        ModeEditConnection, // Ttodos los campos editables
        ModeValidate        // Solo contraseña editable
    };
    explicit dbms_connHandler(QWidget *parent = nullptr, Mode mode = ModeNewConnection);
    ~dbms_connHandler();
    dbHandler* getHandler() const { return handler; }
    void prefillData(const QString& server, const QString& db, const QString& user,   const QString& password);

private slots:
    void on_btnCancel_clicked();
    void on_btnConnect_clicked();

private:
    Ui::dbms_connHandler *ui;
    dbHandler* handler;
    Mode currentMode;
    void applyMode();
};

#endif // DBMS_CONNHANDLER_H
