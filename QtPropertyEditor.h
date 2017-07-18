/* --------------------------------------------------------------------------------
 * QObject property editor UI.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __QtPropertyEditor_H__
#define __QtPropertyEditor_H__

#include <functional>

#include <QAbstractItemModel>
#include <QByteArray>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHash>
#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTreeView>
#include <QVariant>
#include <QVBoxLayout>

#ifdef DEBUG
#include <iostream>
#include <QDebug>
#include <QDateTime>
#endif

namespace QtPropertyEditor
{
    // List all object property names.
    QList<QByteArray> getObjectPropertyNames(QObject *object);
    QList<QByteArray> getMetaObjectPropertyNames(const QMetaObject &metaObject);
    
    // Handle descendant properties such as "child.grandchild.property".
    QObject* descendant(QObject *object, const QByteArray &pathToDescendantObject);
    
    // Get the size of a QTableView widget.
    QSize getTableSize(const QTableView *table);
    
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
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    };
    
    /* --------------------------------------------------------------------------------
     * Property tree model for a QObject tree.
     * Max tree depth can be specified (i.e. depth = 0 --> single object only).
     * -------------------------------------------------------------------------------- */
    class QtPropertyTreeModel : public QtAbstractPropertyModel
    {
        Q_OBJECT
        
    public:
        // Internal tree node.
        struct Node
        {
            // Node traversal.
            Node *parent;
            QList<Node*> children;
            
            // Node data.
            QObject *object;
            QByteArray propertyName;
            
            Node(Node *parent = 0) : parent(parent), object(0) {}
            ~Node() { qDeleteAll(children); }
            
            void setObject(QObject *object, int maxChildDepth = -1, const QList<QByteArray> &propertyNames = QList<QByteArray>());
        };
        
        QtPropertyTreeModel(QObject *parent = 0) : QtAbstractPropertyModel(parent), _maxTreeDepth(-1) {}
        
        // Getters.
        QObject* object() const { return _root.object; }
        int maxDepth() const { return _maxTreeDepth; }
        QList<QByteArray> propertyNames() const { return _propertyNames; }
        QHash<QByteArray, QString> propertyHeaders() const { return _propertyHeaders; }
        
        // Setters.
        void setObject(QObject *object) { beginResetModel(); _root.setObject(object, _maxTreeDepth, _propertyNames); endResetModel(); }
        void setMaxDepth(int i) { _maxTreeDepth = i; }
        void setPropertyNames(const QList<QByteArray> &names) { beginResetModel(); _propertyNames = names; setObject(object()); endResetModel(); }
        void setPropertyHeaders(const QHash<QByteArray, QString> &headers) { beginResetModel(); _propertyHeaders = headers; endResetModel(); }
        
        // Model interface.
        Node* nodeAtIndex(const QModelIndex &index) const;
        QObject* objectAtIndex(const QModelIndex &index) const;
        QByteArray propertyNameAtIndex(const QModelIndex &index) const;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &index) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        Qt::ItemFlags flags(const QModelIndex &index) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        
    protected:
        Node _root;
        int _maxTreeDepth;
        QList<QByteArray> _propertyNames;
        QHash<QByteArray, QString> _propertyHeaders;
    };
    
    /* --------------------------------------------------------------------------------
     * Property table model for a list of QObjects (rows are objects, columns are properties).
     * -------------------------------------------------------------------------------- */
    class QtPropertyTableModel : public QtAbstractPropertyModel
    {
        Q_OBJECT
        
    public:
        typedef std::function<QObject*()> ObjectCreatorFunction;
        
        QtPropertyTableModel(QObject *parent = 0) : QtAbstractPropertyModel(parent), _objectCreator(0) {}
        
        // Getters.
        QObjectList objects() const { return _objects; }
        QList<QByteArray> propertyNames() const { return _propertyNames; }
        QHash<QByteArray, QString> propertyHeaders() const { return _propertyHeaders; }
        ObjectCreatorFunction objectCreator() const { return _objectCreator; }
        
        // Setters.
        void setObjects(const QObjectList &objects) { beginResetModel(); _objects = objects; endResetModel(); }
        template <class T>
        void setObjects(const QList<T*> &objects);
        void setPropertyNames(const QList<QByteArray> &names) { beginResetModel(); _propertyNames = names; endResetModel(); }
        void setPropertyHeaders(const QHash<QByteArray, QString> &headers) { beginResetModel(); _propertyHeaders = headers; endResetModel(); }
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
        ObjectCreatorFunction _objectCreator;
    };
    
    template <class T>
    void QtPropertyTableModel::setObjects(const QList<T*> &objects)
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
    class QtPropertyDelegate: public QStyledItemDelegate
    {
    public:
        QtPropertyDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}
        
        QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
        void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE;
        QString displayText(const QVariant &value, const QLocale &locale) const Q_DECL_OVERRIDE;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    
    protected:
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) Q_DECL_OVERRIDE;
    };
    
    /* --------------------------------------------------------------------------------
     * Tree editor for properties in a QObject tree.
     * -------------------------------------------------------------------------------- */
    class QtPropertyTreeEditor : public QTreeView
    {
        Q_OBJECT
        
    public:
        QtPropertyTreeEditor(QWidget *parent = 0);
        
    public slots:
        void resizeColumnsToContents();
        
    protected:
        QtPropertyDelegate _delegate;
    };
    
    
    /* --------------------------------------------------------------------------------
     * Table editor for properties in a list of QObjects.
     * -------------------------------------------------------------------------------- */
    class QtPropertyTableEditor : public QTableView
    {
        Q_OBJECT
        
    public:
        QtPropertyTableEditor(QWidget *parent = 0);
        
        QSize sizeHint() const Q_DECL_OVERRIDE { return getTableSize(this); }
        
    public slots:
        void horizontalHeaderContextMenu(QPoint pos);
        void verticalHeaderContextMenu(QPoint pos);
        void appendRow();
        void insertSelectedRows();
        void removeSelectedRows();
        void handleSectionMove(int logicalIndex, int oldVisualIndex, int newVisualIndex);
        
    protected:
        QtPropertyDelegate _delegate;
        
        void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
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
    
    /* --------------------------------------------------------------------------------
     * Unit tests.
     * 
     * Example:
     * #include "QtObjectPropertyEditor"
     * int main(int argc, char **argv) {
     *   return QtPropertyEditor::testQtPropertyTreeEditor(argc, argv);
     * }
     * -------------------------------------------------------------------------------- */
    int testQtPropertyTreeEditor(int argc, char **argv);
    int testQtPropertyTableEditor(int argc, char **argv);
    
#endif // DEBUG
    
} // QtPropertyEditor

#endif // __QtPropertyEditor_H__
