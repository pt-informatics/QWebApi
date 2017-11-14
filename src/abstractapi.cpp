#include"abstractapi.h"

#include <QString>

AbstractApi::AbstractApi(QObject *parent): QObject(parent){}

void AbstractApi::_connect(QObject *obj, int index){
    QMetaObject::connect(obj, index, this, metaObject()->indexOfMethod("_changedSignal()"));
}

void AbstractApi::_changedSignal(){
    QObject *obj=sender();
    const QMetaObject *mobj=obj->metaObject();
    int signalId=senderSignalIndex();
    QString className=mobj->className();
    QString signalName=mobj->method(signalId).name();
    QString methodName=QString("%1.%2").arg(className).arg(signalName);

    if(!_apiInfo.contains(className)) return;
    ApiInfo info=_apiInfo[className];

    if(!info.sig2Prop.contains(signalName)) return;
    QString propName=info.sig2Prop[signalName];
    QMetaProperty prop=info.properties[propName].prop;
    if(!prop.isReadable()) return;
    QVariant value=prop.read(obj);

    if(!value.isValid()) return;
    emit _signalEmitted(methodName, value);
}
