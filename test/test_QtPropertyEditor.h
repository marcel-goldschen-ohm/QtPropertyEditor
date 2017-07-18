/* --------------------------------------------------------------------------------
 * QObject property editor UI.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __test_QtPropertyEditor_H__
#define __test_QtPropertyEditor_H__

#include <QDateTime>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>

/* --------------------------------------------------------------------------------
 * QObject derived class with some properties.
 * -------------------------------------------------------------------------------- */
class TestObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MyEnum myEnum READ myEnum WRITE setMyEnum)
    Q_PROPERTY(MyEnum myReadOnlyEnum READ myEnum)
    Q_PROPERTY(bool myBool READ myBool WRITE setMyBool)
    Q_PROPERTY(bool myReadOnlyBool READ myBool)
    Q_PROPERTY(int myInt READ myInt WRITE setMyInt)
    Q_PROPERTY(int myReadOnlyInt READ myInt)
    Q_PROPERTY(float myFloat READ myFloat WRITE setMyFloat)
    Q_PROPERTY(float myReadOnlyFloat READ myFloat)
    Q_PROPERTY(double myDouble READ myDouble WRITE setMyDouble)
    Q_PROPERTY(double myReadOnlyDouble READ myDouble)
    Q_PROPERTY(QString myString READ myString WRITE setMyString)
    Q_PROPERTY(QString myReadOnlyString READ myString)
    Q_PROPERTY(QDateTime myDateTime READ myDateTime WRITE setMyDateTime)
    Q_PROPERTY(QDateTime myReadOnlyDateTime READ myDateTime)
    Q_PROPERTY(QSize mySize READ mySize WRITE setMySize)
    Q_PROPERTY(QSizeF mySizeF READ mySizeF WRITE setMySizeF)
    Q_PROPERTY(QPoint myPoint READ myPoint WRITE setMyPoint)
    Q_PROPERTY(QPointF myPointF READ myPointF WRITE setMyPointF)
    Q_PROPERTY(QRect myRect READ myRect WRITE setMyRect)
    Q_PROPERTY(QRectF myRectF READ myRectF WRITE setMyRectF)
    
public:
    // Custom enum will be editable via a QComboBox so long as we tell Qt about it with Q_ENUMS().
    enum  MyEnum { A, B, C };
    Q_ENUMS(MyEnum)
    
    // Init.
    TestObject(const QString &name = "", QObject *parent = 0, bool hasChild = true) : QObject(parent), _myEnum(B), _myBool(true), _myInt(82), _myFloat(3.14), _myDouble(3.14e-12), _myString("Hi-ya!"), _myDateTime(QDateTime::currentDateTime()), _mySize(2, 4), _mySizeF(3.1, 4.9), _myPoint(0, 1), _myPointF(0.05, 1.03), _myRect(0, 0, 3, 3), _myRectF(0.5, 0.5, 1.3, 3.1)
    {
        setObjectName(name);
        // Child object.
        if(hasChild)
            new TestObject("child", this, false);
    }
    
    // Property getters.
    MyEnum myEnum() const { return _myEnum; }
    bool myBool() const { return _myBool; }
    int myInt() const { return _myInt; }
    float myFloat() const { return _myFloat; }
    double myDouble() const { return _myDouble; }
    QString myString() const { return _myString; }
    QDateTime myDateTime() const { return _myDateTime; }
    QSize mySize() const { return _mySize; }
    QSizeF mySizeF() const { return _mySizeF; }
    QPoint myPoint() const { return _myPoint; }
    QPointF myPointF() const { return _myPointF; }
    QRect myRect() const { return _myRect; }
    QRectF myRectF() const { return _myRectF; }
    
    // Property setters.
    void setMyEnum(MyEnum myEnum) { _myEnum = myEnum; }
    void setMyBool(bool myBool) { _myBool = myBool; }
    void setMyInt(int myInt) { _myInt = myInt; }
    void setMyFloat(float myFloat) { _myFloat = myFloat; }
    void setMyDouble(double myDouble) { _myDouble = myDouble; }
    void setMyString(QString myString) { _myString = myString; }
    void setMyDateTime(QDateTime myDateTime) { _myDateTime = myDateTime; }
    void setMySize(QSize mySize) { _mySize = mySize; }
    void setMySizeF(QSizeF mySizeF) { _mySizeF = mySizeF; }
    void setMyPoint(QPoint myPoint) { _myPoint = myPoint; }
    void setMyPointF(QPointF myPointF) { _myPointF = myPointF; }
    void setMyRect(QRect myRect) { _myRect = myRect; }
    void setMyRectF(QRectF myRectF) { _myRectF = myRectF; }
    
protected:
    MyEnum _myEnum;
    bool _myBool;
    int _myInt;
    double _myFloat;
    double _myDouble;
    QString _myString;
    QDateTime _myDateTime;
    QSize _mySize;
    QSizeF _mySizeF;
    QPoint _myPoint;
    QPointF _myPointF;
    QRect _myRect;
    QRectF _myRectF;
};

#endif // __test_QtPropertyEditor_H__
