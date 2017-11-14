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
