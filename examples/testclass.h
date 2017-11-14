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
