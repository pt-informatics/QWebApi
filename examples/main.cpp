#include <QCoreApplication>

#include "testclass.h"
#include "restapi.h"
#include "websocketapi.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    TestClass *test=new TestClass;
    test->setObjectName("TestClass");

    RestApi restApi;
    restApi.addObject<TestClass*>(test);

    WebSocketApi socketApi;
    socketApi.addObject<TestClass*>(test);

    return a.exec();
}
