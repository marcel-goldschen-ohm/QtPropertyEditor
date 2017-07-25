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
#endif

namespace QtPropertyEditor
{
    // List all object property names.
    QList<QByteArray> getPropertyNames(QObject *object);
    QList<QByteArray> getMetaPropertyNames(const QMetaObject &metaObject);
    
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
    
} // QtPropertyEditor

#endif // __QtPropertyEditor_H__
