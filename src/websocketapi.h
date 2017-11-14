#ifndef WEBSOCKETAPI_H
#define WEBSOCKETAPI_H

#include <QObject>
#include <QMap>
#include <QHostAddress>

#include "abstractapi.h"

class QWebSocketServer;
class QWebSocket;

/**
 * @brief The WebSocketApi class exposes a JSON RPC API via a WebSocket corresponding to a QObjects properties as defined by the use of Q_PROPERTY.
 */
class WebSocketApi : public AbstractApi //public QObject
{
    Q_OBJECT
public:
    /**
     * @brief JSON RPC Error Codes
     * @details This enum contains the error codes (specified by the JSON RPC 2.0 Specification) to be returned
     * in the event of an error occuring during the processing of a request.
     */
    enum JsonRpcError {
        PARSE_ERROR=-32700,
        INVALID_REQUEST=-32600,
        METHOD_NOT_FOUND=-32601,
        INVALID_PARAMS=-32602,
        INTERNAL_ERROR=-32603,
        SERVER_ERROR=-32000
    };
    Q_ENUMS(JsonRpcError)

    /**
     * @brief Construct a WebSocketApi object.
     * @details This constructor will create an object with address QHostAddress::Any and the next available port.
     * @param parent A parent object.
     */
    WebSocketApi(QObject *parent=0);

    /**
     * @brief Construct a WebSocketApi object.
     * @details This constructor will create an object that will listen for incoming connections on address and port.
     * @param address The server will listen for incoming connections on this address.
     * @param port The server will listen for incoming connections on this port.
     * @param parent A parent object.
     */
    WebSocketApi(QHostAddress address, qint16 port, QObject *parent=0);
    ~WebSocketApi();

private slots:
    void _newConnection();
    void _processText(QString message);
    void _processBinary(QByteArray message);
    void _sendSignal(QString methodName, QVariant value);
    void _disconnected();

private:
    QWebSocketServer *_socketServer;
    QList<QWebSocket*> _clients;

    const QMap<int,QString> JsonRpcErrorStr{
        {PARSE_ERROR, "Invalid JSON was received by the server."},
        {INVALID_REQUEST, "The JSON sent is not a valid Request object."},
        {METHOD_NOT_FOUND, "The method does not exist / is not available."},
        {INVALID_PARAMS, "Invalid method parameter(s)."},
        {INTERNAL_ERROR, "Internal JSON-RPC error."},
    };

    QString _parseMessage(QString message);
    QString _toError(JsonRpcError error, int id=-1);
    QString _toResponse(QVariant result, int id);
    QString _toNotification(QString method, QVariant params=0);
};

#endif // WEBSOCKETAPI_H
