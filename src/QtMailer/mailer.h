#ifndef MAILER_H
#define MAILER_H

#include <QObject>
#include <QList>
#include <QString>
#include <deque>
#include <QStringList>
#include <utility>
#include <QSslSocket>
#include <QHostInfo>
#include <QEventLoop>

#include "mail.h"

#define SMTPPORT 25
#define SMTPTIMEOUT 30000

#define ERROR_UNENCCONNECTIONNOTPOSSIBLE    "Could not connect to server"
#define ERROR_ENCCONNECTIONNOTPOSSIBLE      "Could not connect to server encrypted"

class Mailer : public QObject
{
    Q_OBJECT

    enum SMTP_States {
        Disconnected,
        Connected,
        EHLOsent,
        MAILFROMsent,
        TOsent,
        DATAsent,
        CONTENTsent,
        QUITsent,
        RSETsent,
        AUTH
    };



    enum SMTP_Login_State {
        PRELOGIN,
        AUTHLOGINsent,
        USERNAMEsent,
        PASSWORDsent
    };


public:
    enum ENCRYPTION{
        UNENCRYPTED,
        STARTTLS,
        SSL
    };

    enum SMTP_Auth_Method {
        LOGIN,
        NO_Auth
    };
    explicit Mailer(const QString &server, QObject *parent = 0);

    int                     sizeOfQueue() const;
    bool                    sendAllMails();
    void                    enqueueMail(const Mail& mail);
    QString                 getServer() const;
    void                    setServer(const QString &value);
    bool                    isBusy();
    void                    waitForProcessing();
    std::pair<int,int>      lastErrors() const;
    int                     getSmtpPort() const;
    void                    setSmtpPort(int value);
    int                     getSmtpTimeout() const;
    void                    setSmtpTimeout(int value);
    void                    setAUTHMethod(SMTP_Auth_Method);
    void                    setPassword(const QString &value);
    void                    setUsername(const QString &value);
    void                    setEncryptionUsed(const ENCRYPTION &value);

protected:
    QString             server;
    QSslSocket*         socket{nullptr};
    QTextStream         socketStream;
    bool                isConnected{false};
    SMTP_States         currentState{Disconnected};
    std::deque<Mail>    mailqueue;
    Mail*               processedMail{nullptr};
    int                 recepientsSent{0};
    int                 mailsProcessed{0};
    int                 mailsToSend{0};
    int                 tempErrors{0};
    int                 permErrors{0};
    int                 smtpPort{SMTPPORT};
    int                 smtpTimeout{SMTPTIMEOUT};
    SMTP_Auth_Method    authMethodToUse{NO_Auth};
    ENCRYPTION          encryptionUsed{UNENCRYPTED};
    SMTP_Login_State    loginState{PRELOGIN};
    QString             username;
    QString             password;

    bool                connectToServer();
    void                disconnectFromServer();
    void                sendAUTHLOGIN();
    void                sendAUTHLOGINuser();
    void                sendAUTHLOGINpassword();
    void                sendEHLO();
    void                sendMAILFROM();
    void                sendTO();
    void                sendDATA();
    void                sendMessagecontent();
    void                sendQUIT();
    void                sendRSET();
    void                sendNextMailOrQuit();
    void                mailProcessed();

signals:
    void finishedSending(bool queueEmpty);
    void errorSendingMails(int smtpErrorcode, QString smtpErrorstring);
    void mailsHaveBeenProcessedTillNow(int numberOfMailsProcessed);

public slots:
    void     cancelSending();

protected slots:
    void    dataReadyForReading();
    void    errorReceived(QAbstractSocket::SocketError);
    void    sslErrorsReceived(QList<QSslError>);


public slots:

};

#endif // MAILER_H