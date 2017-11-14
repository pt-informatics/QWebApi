#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

#include "testclass.h"
#include "restapi.h"
#include "websocketapi.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    TestClass *test=new TestClass;
    test->setObjectName("TestClass");

    RestApi restApi(QHostAddress::LocalHost, 45678);
    restApi.addObject<TestClass*>(test);

    WebSocketApi socketApi(QHostAddress::LocalHost, 45679);
    socketApi.addObject<TestClass*>(test);

    QFileInfo clientPath("../../clients/browser/typescript/test_page.html");
    QDesktopServices::openUrl(QUrl::fromLocalFile(clientPath.absoluteFilePath()));

    return a.exec();
}
