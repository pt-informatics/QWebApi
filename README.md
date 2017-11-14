# QWebApi
A library to expose the properties of any [QObject](http://doc.qt.io/qt-5/qobject.html) derived C++ class via a [REST](https://en.wikipedia.org/wiki/Representational_state_transfer) [API](https://en.wikipedia.org/wiki/Application_programming_interface), a WebSocket [JSONRPC](http://www.jsonrpc.org/specification) [API](https://en.wikipedia.org/wiki/Application_programming_interface) or both.

## Dependencies
This example (obviously) depends on Qt and nothing else.

It has been tested with the following configurations:

### macOS 10.12.6 Sierra

* Apple LLVM verion 9.0.0 (clang-900.0.37)
* Qt 5.9.2

## Implementation
There is a minimal example project in the 'examples' folder which covers the following:

1. Add `include (/path/to/qwebapi/src/qwebapi.pri)` to you .pro file
2. Ensure the class you wish to expose extends `Q_OBJECT` and uses `Q_PROPERTY` to decorate the desired properties. For example:

**testclass.h**

```c++
#ifndef TESTCLASS_H
#define TESTCLASS_H

#include <QObject>

class TestClass : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("Version", "0.1")
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit TestClass(QObject *parent = 0);
    int value();

signals:
    void valueChanged(int);

public slots:
    void setValue(int value);

    void valueChangedSlot(int value);

private:
    int _value;
};

#endif // TESTCLASS_H
```

**testclass.cpp**
```c++
#include "testclass.h"

#include <QDebug>

TestClass::TestClass(QObject *parent)
    : QObject(parent), _value(42)
{
    connect(this, SIGNAL(valueChanged(int)), SLOT(valueChangedSlot(int)));
}

int TestClass::value(){ return _value; }

void TestClass::setValue(int value){
    if(_value==value) return;
    _value=value;
    emit valueChanged(_value);
}

void TestClass::valueChangedSlot(int value){
    qDebug() << "Value Changed:" << value;
}
```

3. Create an instance of either `RestApi()` and/or `WebSocketApi()` and add the object you wish to expose as follows:

**main.cpp**
```c++
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
```

## Usage
### REST
A URI is created for each property with the following format: `/ClassName/PropertyName`. So, for our example above the URI `/TestClass/value` would expose the `value` property. Because we specified both a setter (READ) and getter (WRITE) method, it ispossible to both get and set the property using this URI. If we wanted a read-only property then we could simply omit setter in the `Q_PROPERTY` specification.

Using the generated URI's is simple, for example using cURL from the command line:

```sh
$ curl http://localhost:<port>/TestClass/value                       # Get value
$ curl -X PUT -d "new value" http://localhost:<port>/TestClass/value # Set value
```

### WebSocket
The WebSocket API makes use of the JSON RPC standard message formats. 

To get the value of a property:

```json
{
  "jsonrpc": "2.0",
  "method": "TestClass.value",
  "id": 1
}
```

The response will be in the following form:

```json
{
  "jsonrpc": "2.0", 
  "id": 1,
  "result": 42
}
```

If a property defined a NOTIFY signal, then a message will be sent whenever this signal is emitted as follows:

```json
{
    "jsonrpc": "2.0",
    "method": "TestClass.valueChanged",
    "params": 1
}
```

An TypeScript RPC library is included in the 'clients/browser/typescript' folder along with an example HTML page.
