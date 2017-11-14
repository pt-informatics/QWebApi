#ifndef WEBAPI_H
#define WEBAPI_H

#include <QObject>
#include <QHostAddress>

#include "abstractapi.h"

class QTcpServer;
class QNetworkSession;

/**
 * @brief The RestApi class exposes a REST API corresponding to a QObjects properties as defined by the use of Q_PROPERTY.
 */
class RestApi : public AbstractApi
{
    Q_OBJECT
public:   
    /**
     * @brief Constructs a RestApi object.
     * @details This constructor will create an object with address QHostAddress::Any and the next available port.
     * @param parent A parent object.
     */
    RestApi(QObject *parent=0);

    /**
     * @brief Constructs a RestApi object.
     * @details This constructor will create an object that will listen for incoming connections on address and port.
     * @param address The server will listen for incoming connections on this address.
     * @param port The server will listen for incoming connections on this port.
     * @param parent A parent object.
     */
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
