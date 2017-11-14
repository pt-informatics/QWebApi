#ifndef ABSTRACTAPI_H
#define ABSTRACTAPI_H

#include <QObject>

#include <QMetaObject>
#include <QMetaClassInfo>
#include <QMetaProperty>
#include <QDebug>

class AbstractApi : public QObject
{
    Q_OBJECT
public:
    typedef struct ApiProp {
        QMetaProperty prop;
    } ApiProp;

    typedef struct ApiInfo {
        QString version;
        QHash<QString,ApiProp> properties;
        QHash<QString,QString> sig2Prop;
        QVariant obj;
    } ApiInfo;

    explicit AbstractApi(QObject *parent=0);

    template<class T> void addObject(T obj){
        const QMetaObject *mobj=obj->metaObject();
        ApiInfo info;
        info.obj=QVariant::fromValue(obj);

        info.version="unknown";
        for(int i=0; i<mobj->classInfoCount(); i++){
            if(QString(mobj->classInfo(i).name()).toLower()=="version"){
                info.version=mobj->classInfo(i).value();
                break;
            }
        }

        QString className=mobj->className();
        for(int i=mobj->propertyOffset(); i<mobj->propertyCount(); i++){
            ApiProp prop;
            prop.prop=mobj->property(i);
            info.properties[mobj->property(i).name()]=prop;
            if(prop.prop.hasNotifySignal()){
                info.sig2Prop[prop.prop.notifySignal().name()]=prop.prop.name();
                _connect(obj, prop.prop.notifySignalIndex());
            }
        }
        _apiInfo[mobj->className()]=info;
    }

signals:
    void _signalEmitted(QString methodName, QVariant value);

private slots:
    void _changedSignal();

private:
    void _connect(QObject* obj, int index);

protected:
    QHash<QString, ApiInfo> _apiInfo;
};

#endif // ABSTRACTAPI_H


