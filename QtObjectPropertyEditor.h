/* --------------------------------------------------------------------------------
 * QObject property editor UI.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __QtObjectPropertyEditor_H__
#define __QtObjectPropertyEditor_H__

#include <functional>

#include <QAbstractItemModel>
#include <QByteArray>
#include <QDialog>
#include <QHash>
#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVariant>
#include <QVBoxLayout>

#ifdef DEBUG
#include <iostream>
#include <QDebug>
#include <QDateTime>
#endif

namespace QtObjectPropertyEditor
{
    // List all object property names.
    QList<QByteArray> getObjectPropertyNames(QObject *object);
    QList<QByteArray> getMetaObjectPropertyNames(const QMetaObject &metaObject);
    
    // Handle descendant properties such as "child.grandchild.property".
    QObject* descendant(QObject *object, const QByteArray &pathToDescendantObject);
    
    // Get the size of a QTableView widget.
    QSize getTableSize(QTableView *table);
    
    /* --------------------------------------------------------------------------------
     * Things that all QObject property models should be able to do.
     * -------------------------------------------------------------------------------- */
    class QtAbstractPropertyModel : public QAbstractItemModel
    {
        Q_OBJECT
        
    public:
        QtAbstractPropertyModel(QObject *parent = 0) : QAbstractItemModel(parent) {}
        
        virtual QObject* objectAtIndex(const QModelIndex &index) const = 0;
        virtual QByteArray propertyNameAtIndex(const QModelIndex &index) const = 0;
        const QMetaProperty metaPropertyAtIndex(const QModelIndex &index) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        Qt::ItemFlags flags(const QModelIndex &index) const;
    };
    
    /* --------------------------------------------------------------------------------
     * Property model for a single QObject.
     * -------------------------------------------------------------------------------- */
    class QtObjectPropertyModel : public QtAbstractPropertyModel
    {
        Q_OBJECT
        
    public:
        QtObjectPropertyModel(QObject *parent = 0) : QtAbstractPropertyModel(parent), _object(0) {}
        
        // Property getters.
        QObject* object() const { return _object; }
        QList<QByteArray> propertyNames() const { return _propertyNames; }
        QHash<QByteArray, QString> propertyHeaders() const { return _propertyHeaders; }
        
        // Property setters.
        void setObject(QObject *obj) { beginResetModel(); _object = obj; endResetModel(); }
        void setPropertyNames(const QList<QByteArray> &names) { beginResetModel(); _propertyNames = names; endResetModel(); }
        void setPropertyHeaders(const QHash<QByteArray, QString> &headers) { beginResetModel(); _propertyHeaders = headers; endResetModel(); }
        
        // Model interface.
        QObject* objectAtIndex(const QModelIndex &index) const;
        QByteArray propertyNameAtIndex(const QModelIndex &index) const;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &index) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        
    protected:
        QObject *_object;
        QList<QByteArray> _propertyNames;
        QHash<QByteArray, QString> _propertyHeaders;
    };
    
    /* --------------------------------------------------------------------------------
     * Property model for a list of QObjects (same properties for each).
     * -------------------------------------------------------------------------------- */
    class QtObjectListPropertyModel : public QtAbstractPropertyModel
    {
        Q_OBJECT
        
    public:
        typedef std::function<QObject*()> ObjectCreatorFunction;
        
        QtObjectListPropertyModel(QObject *parent = 0) : QtAbstractPropertyModel(parent), _parentOfObjects(0), _objectCreator(0) {}
        
        // Property getters.
        QObjectList objects() const { return _objects; }
        QList<QByteArray> propertyNames() const { return _propertyNames; }
        QHash<QByteArray, QString> propertyHeaders() const { return _propertyHeaders; }
        QObject* parentOfObjects() const { return _parentOfObjects; }
        ObjectCreatorFunction objectCreator() const { return _objectCreator; }
        
        // Property setters.
        void setObjects(const QObjectList &objects) { beginResetModel(); _objects = objects; endResetModel(); }
        template <class T>
        void setObjects(const QList<T*> &objects);
        void setPropertyNames(const QList<QByteArray> &names) { beginResetModel(); _propertyNames = names; endResetModel(); }
        void setPropertyHeaders(const QHash<QByteArray, QString> &headers) { beginResetModel(); _propertyHeaders = headers; endResetModel(); }
        void setParentOfObjects(QObject *parent) { _parentOfObjects = parent; }
        void setObjectCreator(ObjectCreatorFunction creator) { _objectCreator = creator; }
        
        // For convenience.
        template <class T>
        static QObject* defaultObjectCreator() { return new T(); }
        
        // Model interface.
        QObject* objectAtIndex(const QModelIndex &index) const;
        QByteArray propertyNameAtIndex(const QModelIndex &index) const;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &index) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
        bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
        bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow);
        void reorderChildObjectsToMatchRowOrder(int firstRow = 0);
        
    signals:
        void rowCountChanged();
        void rowOrderChanged();
        
    protected:
        QObjectList _objects;
        QList<QByteArray> _propertyNames;
        QHash<QByteArray, QString> _propertyHeaders;
        QObject *_parentOfObjects;
        ObjectCreatorFunction _objectCreator;
    };
    
    template <class T>
    void QtObjectListPropertyModel::setObjects(const QList<T*> &objects)
    {
        beginResetModel();
        _objects.clear();
        foreach(T *object, objects) {
            if(QObject *obj = qobject_cast<QObject*>(object))
                _objects.append(obj);
        }
        endResetModel();
    }
    
    /* --------------------------------------------------------------------------------
     * Property editor delegate.
     * -------------------------------------------------------------------------------- */
    class QtObjectPropertyDelegate: public QStyledItemDelegate
    {
    public:
        QtObjectPropertyDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}
        
        QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
        void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE;
        QString displayText(const QVariant &value, const QLocale &locale) const Q_DECL_OVERRIDE;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    
    protected:
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) Q_DECL_OVERRIDE;
    };
    
    /* --------------------------------------------------------------------------------
     * Editor for properties in a single QObject.
     * -------------------------------------------------------------------------------- */
    class QtObjectPropertyEditor : public QTableView
    {
        Q_OBJECT
        
    public:
        QtObjectPropertyEditor(QWidget *parent = 0);
        
        QSize sizeHint() { return getTableSize(this); }
    
    protected:
        QtObjectPropertyDelegate _delegate;
    };
    
    /* --------------------------------------------------------------------------------
     * Dialog for QtObjectPropertyEditor.
     * -------------------------------------------------------------------------------- */
    class QtObjectPropertyDialog : public QDialog
    {
        Q_OBJECT
        
    public:
        QtObjectPropertyModel model;
        
        QtObjectPropertyDialog(QObject *object, QWidget *parent = 0);
        
    protected:
        QtObjectPropertyEditor *_editor;
    };
    
    /* --------------------------------------------------------------------------------
     * Editor for properties in a list of QObjects.
     * -------------------------------------------------------------------------------- */
    class QtObjectListPropertyEditor : public QTableView
    {
        Q_OBJECT
        
    public:
        QtObjectListPropertyEditor(QWidget *parent = 0);
        
        QSize sizeHint() { return getTableSize(this); }
        
    public slots:
        void horizontalHeaderContextMenu(QPoint pos);
        void verticalHeaderContextMenu(QPoint pos);
        void appendRow();
        void insertSelectedRows();
        void removeSelectedRows();
        void handleSectionMove(int logicalIndex, int oldVisualIndex, int newVisualIndex);
        
    protected:
        QtObjectPropertyDelegate _delegate;
        
        void keyPressEvent(QKeyEvent *event);
    };
    
#ifdef DEBUG
    
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
        TestObject(const QString &name = "", QObject *parent = 0) :
        QObject(parent),
        _myEnum(B),
        _myBool(true),
        _myInt(82),
        _myFloat(3.14),
        _myDouble(3.14e-12),
        _myString("Hi-ya!"),
        _myDateTime(QDateTime::currentDateTime()),
        _mySize(2, 4),
        _mySizeF(3.1, 4.9),
        _myPoint(0, 1),
        _myPointF(0.05, 1.03),
        _myRect(0, 0, 3, 3),
        _myRectF(0.5, 0.5, 1.3, 3.1)
        {
            setObjectName(name);
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
    
    /* --------------------------------------------------------------------------------
     * Unit tests.
     * 
     * Example:
     * #include "QtObjectPropertyEditor"
     * int main(int argc, char **argv) {
     *   return QtObjectPropertyEditor::testQtObjectListPropertyEditor(argc, argv);
     * }
     * -------------------------------------------------------------------------------- */
    int testQtObjectPropertyEditor(int argc, char **argv);
    int testQtObjectListPropertyEditor(int argc, char **argv);
    
#endif // DEBUG
    
} // QtObjectPropertyEditor

#endif // __QtObjectPropertyEditor_H__
