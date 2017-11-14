#include "restapi.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkSession>
#include <QDateTime>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "picohttpparser.h"

RestApi::RestApi(QObject *parent)
    : AbstractApi(parent), _tcpServer(Q_NULLPTR), _networkSession(0)
{
    _tcpServer=new QTcpServer(this);
    if(!_tcpServer->listen()){
        qCritical() << "Failed to start listening!";
        return;
    }

    QString ipAddress;
    QList<QHostAddress> ipList=QNetworkInterface::allAddresses();
    QHostAddress ip;
    foreach(ip, ipList){
        if(ip==QHostAddress::LocalHost) continue;
        if(!ip.toIPv4Address()) continue;
        ipAddress=ip.toString();
    }
    if(ipAddress.isEmpty()) ipAddress=QHostAddress(QHostAddress::LocalHost).toString();
    qDebug() << "IP Address:" << ipAddress << _tcpServer->serverPort();

    connect(_tcpServer, SIGNAL(newConnection()), SLOT(_newConnection()));
}

void RestApi::_newConnection(){
    QTcpSocket *socket=_tcpServer->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
    connect(socket, SIGNAL(readyRead()), SLOT(_readyRead()));
}

void RestApi::_readyRead(){
    QTcpSocket *socket=dynamic_cast<QTcpSocket*>(sender());
    QString request=socket->readAll();

    int responseCode=200;
    QString method, path, content, responseText="OK";
    bool ok=_parse(request, &method, &path, &content);
    if(!ok) { responseCode=400; responseText="Bad request"; }
    else {
        if(method.toLower()!="get" && method.toLower()!="put"){
            responseCode=405; responseText="Method not allowed";
        } else {
            if(path.startsWith("/")) path=path.mid(1);
            auto pathBits=path.split("/");
            if(pathBits.count()<2){
                responseCode=404; responseText="Not found";
            } else {
                auto clazz=pathBits[0], prop=pathBits[1];
                if(_apiInfo.contains(clazz)){
                    ApiInfo info=_apiInfo[clazz];
                    if(info.properties.contains(prop)){
                        auto mprop=info.properties[prop].prop;
                        auto obj=info.obj.value<QObject*>();
                        if(method.toLower()=="get"){
                            if(mprop.isReadable())
                                responseText=mprop.read(obj).toString();
                        }
                        else {
                            if(mprop.isWritable()){
                                auto value=QVariant(content);
                                value.convert(mprop.type());
                                ok=mprop.write(obj, value);
                                if(mprop.hasNotifySignal()) emit mprop.notifySignal();
                            }
                        }
                    } else { responseCode=404; responseText="Not found"; }
                } else { responseCode=404; responseText="Not found"; }
            }
        }
    }

    auto now=QDateTime::currentDateTime().toString("ddd, dd MMM yyyy HH:mm:ss t");
    auto response=QString(
        "HTTP/1.1 %1 OK\r\n"
        "Server: RestApi/0.1\r\n"
        "Date: %2\r\n"
        "Connection: close\r\n"
        "content-type: text/plain;charset=UTF-8\r\n"
        "Content-Length: %3\r\n\r\n"
        "%4"
    ).arg(responseCode).arg(now).arg(responseText.length()).arg(responseText);

    socket->write(response.toLocal8Bit());
    socket->waitForBytesWritten();
    socket->disconnectFromHost();

    return;
}

bool RestApi::_parse(QString http, QString *method, QString *path, QString *content){
    const char *_method, *_path;
    int minorVersion;
    size_t methodLen, pathLen;
    struct phr_header headers[100];

    size_t numHeaders=sizeof(headers)/sizeof(headers[0]);

    QByteArray bHttp=http.toLocal8Bit();
    char *data=new char[bHttp.size()+1];
    strcpy(data, bHttp.data());

    int ret=phr_parse_request(data, strlen(data), &_method, &methodLen, &_path, &pathLen,
                              &minorVersion, headers, &numHeaders, 0);

    if(ret<=0) return false;

    *method=QString(_method);
    method->truncate(methodLen);
    *path=QString(_path);
    path->truncate(pathLen);
    *content=QString(http).mid(ret);
    return true;
}
