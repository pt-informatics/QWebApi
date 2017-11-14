#ifndef WEBSOCKETAPI_H
#define WEBSOCKETAPI_H

#include <QObject>
#include <QMap>
#include <QHostAddress>

#include "abstractapi.h"

class QWebSocketServer;
class QWebSocket;

class WebSocketApi : public AbstractApi //public QObject
{
    Q_OBJECT
public:
    enum JsonRpcError {
        PARSE_ERROR=-32700,
        INVALID_REQUEST=-32600,
        METHOD_NOT_FOUND=-32601,
        INVALID_PARAMS=-32602,
        INTERNAL_ERROR=-32603,
        SERVER_ERROR=-32000
    };
    Q_ENUMS(JsonRpcError)

    const QMap<int,QString> JsonRpcErrorStr{
        {PARSE_ERROR, "Invalid JSON was received by the server or "},
        {INVALID_REQUEST, "The JSON sent is not a valid Request object."},
        {METHOD_NOT_FOUND, "The method does not exist / is not available."},
        {INVALID_PARAMS, "Invalid method parameter(s)."},
        {INTERNAL_ERROR, "Internal JSON-RPC error."},
    };

    WebSocketApi(QObject *parent=0);
    WebSocketApi(QHostAddress address, qint16 port, QObject *parent=0);
    ~WebSocketApi();

signals:
    void closed();

private slots:
    void _newConnection();
    void _processText(QString message);
    void _processBinary(QByteArray message);
    void _sendSignal(QString methodName, QVariant value);
    void _disconnected();

private:
    QWebSocketServer *_socketServer;
    QList<QWebSocket*> _clients;

    QString _parseMessage(QString message);
    QString _toError(JsonRpcError error, int id=-1);
    QString _toResponse(QVariant result, int id);
    QString _toNotification(QString method, QVariant params=0);
};

#endif // WEBSOCKETAPI_H
