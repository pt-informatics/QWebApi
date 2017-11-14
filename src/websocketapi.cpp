#include "websocketapi.h"

#include <QWebSocketServer>
#include <QWebSocket>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

WebSocketApi::WebSocketApi(QObject *parent)
    : AbstractApi(parent),
      _socketServer(new QWebSocketServer("WebSocketApi", QWebSocketServer::NonSecureMode, this))
{
    if(!_socketServer->listen()){
        qCritical() << "Failed to start listening";
        return;
    }

    qDebug() << _socketServer->serverAddress() << _socketServer->serverPort();

    connect(_socketServer, SIGNAL(newConnection()), SLOT(_newConnection()));
    connect(_socketServer, SIGNAL(closed()), SIGNAL(closed()));
    connect(this, SIGNAL(_signalEmitted(QString,QVariant)), SLOT(_sendSignal(QString,QVariant)));
}

WebSocketApi::~WebSocketApi(){
    _socketServer->close();
    qDeleteAll(_clients.begin(), _clients.end());
}

void WebSocketApi::_newConnection(){
    qDebug() << "New Connection";
    QWebSocket *socket=_socketServer->nextPendingConnection();
    connect(socket, SIGNAL(textMessageReceived(QString)), SLOT(_processText(QString)));
    connect(socket, SIGNAL(binaryMessageReceived(QByteArray)), SLOT(_processBinary(QByteArray)));
    connect(socket, SIGNAL(disconnected()), SLOT(_disconnected()));
    _clients << socket;
}

void WebSocketApi::_processText(QString message){
    QWebSocket *socket=dynamic_cast<QWebSocket*>(sender());
    qDebug() << "Text Message:" << message;
    socket->sendTextMessage(_parseMessage(message));
}

QString WebSocketApi::_parseMessage(QString message){
    QJsonDocument jdoc=QJsonDocument::fromJson(message.toLatin1());
    if(!jdoc.isObject()) return _toError(PARSE_ERROR);

    QJsonObject jobj=jdoc.object();
    if(!(jobj.contains("jsonrpc") && jobj.contains("id") && jobj.contains("method")))
        return _toError(INVALID_REQUEST);

    QJsonValue jsonrpc=jobj["jsonrpc"];
    if(!(jsonrpc.isString() && jsonrpc.toString()=="2.0"))
        return _toError(INVALID_REQUEST);

    QJsonValue jid=jobj["id"];
    if(!jid.isDouble() && jid.toDouble()>0) return _toError(INVALID_REQUEST);
    int id=jid.toInt();

    QJsonValue jmethod=jobj["method"];
    if(!jmethod.isString()) return _toError(INVALID_REQUEST);
    QString method=jmethod.toString();

    QVariant arg;
    QVariantList args;
    if(jobj.contains("params")){
        QJsonValue jparams=jobj["params"];
        if(jparams.isArray()) args=jparams.toArray().toVariantList();
        else arg=jparams.toVariant();
    }

    auto mbits=method.split(".");
    if(mbits.count()!=2) return _toError(METHOD_NOT_FOUND, id);

    auto clazz=mbits[0], prop=mbits[1];
    if(_apiInfo.contains(clazz)){
        ApiInfo info=_apiInfo[clazz];
        if(info.properties.contains(prop)){
            auto mprop=info.properties[prop].prop;
            auto obj=info.obj.value<QObject*>();

            if(arg.isNull()&&args.isEmpty()){
                if(mprop.isReadable()) return _toResponse(mprop.read(obj), id);
                else return _toError(METHOD_NOT_FOUND, id);
            } else {
                if(args.count()>1) return _toError(INVALID_PARAMS, id);
                else if(!args.isEmpty()) arg=args[0];
                arg.convert(mprop.type());
                bool ok=mprop.write(obj,arg);
                if(!ok) return _toError(INTERNAL_ERROR, id);
                return _toResponse("OK",id);
            }
        }
    }
    return _toError(METHOD_NOT_FOUND, id);
}

QString WebSocketApi::_toError(JsonRpcError error, int id){
    QJsonObject jresponse;
    jresponse["jsonrpc"]=2.0;
    if(id>0) jresponse["id"]=id;

    QJsonObject jdata;
    jdata["code"]=error;
    jdata["message"]=JsonRpcErrorStr[error];
    jresponse["error"]=jdata;

    return QJsonDocument(jresponse).toJson();
}

QString WebSocketApi::_toResponse(QVariant result, int id){
    QJsonObject jresponse;
    jresponse["jsonrpc"]=2.0;
    jresponse["id"]=id;

    if(result.type()==QVariant::String) jresponse["result"]=result.toString();
    else if(result.type()==QVariant::Int) jresponse["result"]=result.toInt();
    else if(result.type()==QVariant::Double) jresponse["result"]=result.toDouble();
    else jresponse["result"]="Invalid";

    return QJsonDocument(jresponse).toJson();
}

QString WebSocketApi::_toNotification(QString method, QVariant params){
    QJsonObject jnot;
    jnot["jsonrpc"]="2.0";
    jnot["method"]=method;

    if(!params.isNull()){
        if(params.type()==QVariant::List) jnot["params"]=QJsonArray::fromVariantList(params.toList());
        else if(params.type()==QVariant::String) jnot["params"]=params.toString();
        else if(params.type()==QVariant::Int) jnot["params"]=params.toInt();
        else if(params.type()==QVariant::Double) jnot["params"]=params.toDouble();
        else jnot["params"]="Invalid";
    }

    return QJsonDocument(jnot).toJson();
}

void WebSocketApi::_processBinary(QByteArray message){
    QWebSocket *socket=dynamic_cast<QWebSocket*>(sender());
    Q_UNUSED(socket);
    qDebug() << "Binary Message:" << message;
}

void WebSocketApi::_sendSignal(QString methodName, QVariant value){
    // Notification style...
    QString message=_toNotification(methodName, value);
    foreach(QWebSocket* client, _clients){
        client->sendTextMessage(message);
    }
}

void WebSocketApi::_disconnected(){
    QWebSocket *socket=dynamic_cast<QWebSocket*>(sender());
    qDebug() << "Socket disconnected:" << socket;
    if(!socket) return;
    _clients.removeAll(socket);
    socket->deleteLater();
}
