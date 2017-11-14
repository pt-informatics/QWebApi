#ifndef WEBAPI_H
#define WEBAPI_H

#include <QObject>
#include <QHostAddress>

#include "abstractapi.h"

class QTcpServer;
class QNetworkSession;

class RestApi : public AbstractApi
{
    Q_OBJECT
public:
    RestApi(QObject *parent=0);
    RestApi(QHostAddress address, qint16 port, QObject* parent=0);

private slots:
    void _newConnection();
    void _readyRead();

private:
    bool _parse(QString http, QString *method, QString *path, QString *content);

    QTcpServer *_tcpServer;
    QNetworkSession *_networkSession;
};

#endif // WEBAPI_H
