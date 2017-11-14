#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>

/// @private
class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = 0);

signals:

public slots:
};

#endif // HTTPSERVER_H
