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
#include <QAction>
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
    QList<QByteArray> getNoninheritedPropertyNames(QObject *object);
    
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
        
        QList<QByteArray> propertyNames;
        QHash<QByteArray, QString> propertyHeaders;
        void setProperties(const QString &str);
        void addProperty(const QString &str);
        
        virtual QObject* objectAtIndex(const QModelIndex &index) const = 0;
        virtual QByteArray propertyNameAtIndex(const QModelIndex &index) const = 0;
        const QMetaProperty metaPropertyAtIndex(const QModelIndex &index) const;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        virtual Qt::ItemFlags flags(const QModelIndex &index) const;
        
    protected:
        // Called after a property was successfully changed via setData().
        // Derived models emit dataChanged() for all cells that may depend on the changed value
        // (e.g. read-only properties that mirror a writable one).
        virtual void refreshAfterChange(const QModelIndex &/* index */) {}
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
            Node *parent = NULL;
            QList<Node*> children;
            
            // Node data.
            QObject *object = NULL;
            QByteArray propertyName;
            
            Node(Node *parent = NULL) : parent(parent) {}
            ~Node() { qDeleteAll(children); }
            
            void setObject(QObject *object, int maxChildDepth = -1, const QList<QByteArray> &propertyNames = QList<QByteArray>());
        };
        
        QtPropertyTreeModel(QObject *parent = NULL) : QtAbstractPropertyModel(parent) {}
        
        // Getters.
        QObject* object() const { return _root.object; }
        int maxDepth() const { return _maxTreeDepth; }
        
        // Setters.
        void setObject(QObject *object) { beginResetModel(); _root.setObject(object, _maxTreeDepth, propertyNames); endResetModel(); }
        void setMaxDepth(int i) { _maxTreeDepth = i; reset(); }
        void setProperties(const QString &str) { QtAbstractPropertyModel::setProperties(str); reset(); }
        void addProperty(const QString &str) { QtAbstractPropertyModel::addProperty(str); reset(); }
        
        // Model interface.
        Node* nodeAtIndex(const QModelIndex &index) const;
        QObject* objectAtIndex(const QModelIndex &index) const override;
        QByteArray propertyNameAtIndex(const QModelIndex &index) const override;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &index) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        
    public slots:
        void reset() { beginResetModel(); _root.setObject(object(), _maxTreeDepth, propertyNames); endResetModel(); }
        
    protected:
        void refreshAfterChange(const QModelIndex &index) override;
        
        Node _root;
        int _maxTreeDepth = -1;
    };
    
    /* --------------------------------------------------------------------------------
     * Property table model for a list of QObjects (rows are objects, columns are properties).
     * -------------------------------------------------------------------------------- */
    class QtPropertyTableModel : public QtAbstractPropertyModel
    {
        Q_OBJECT
        
    public:
        typedef std::function<QObject*()> ObjectCreatorFunction;
        
        QtPropertyTableModel(QObject *parent = NULL) : QtAbstractPropertyModel(parent) {}
        
        // Getters.
        QObjectList objects() const { return _objects; }
        ObjectCreatorFunction objectCreator() const { return _objectCreator; }
        
        // Setters.
        void setObjects(const QObjectList &objects) { beginResetModel(); _objects = objects; endResetModel(); }
        template <class T>
        void setObjects(const QList<T*> &objects);
        template <class T>
        void setChildObjects(QObject *parent);
        void setObjectCreator(ObjectCreatorFunction creator) { _objectCreator = creator; }
        void setProperties(const QString &str) { beginResetModel(); QtAbstractPropertyModel::setProperties(str); endResetModel(); }
        void addProperty(const QString &str) { beginResetModel(); QtAbstractPropertyModel::addProperty(str); endResetModel(); }
        
        // Model interface.
        QObject* objectAtIndex(const QModelIndex &index) const override;
        QByteArray propertyNameAtIndex(const QModelIndex &index) const override;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &index) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
        bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
        bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow) override;
        void reorderChildObjectsToMatchRowOrder(int firstRow = 0);
        
        // Default creator functions for convenience.
        // Requires template class T to implement a default constructor T().
        template <class T>
        static QObject* defaultCreator() { return new T(); }
        template <class T>
        static QObject* defaultChildCreator(QObject *parent) { T *object = new T(); object->setParent(parent); return object; }
        
    signals:
        void rowCountChanged();
        void rowOrderChanged();
        
    protected:
        void refreshAfterChange(const QModelIndex &index) override;
        
        QObjectList _objects;
        ObjectCreatorFunction _objectCreator = NULL;
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
    
    template <class T>
    void QtPropertyTableModel::setChildObjects(QObject *parent)
    {
        beginResetModel();
        _objects.clear();
        foreach(T *derivedObject, parent->findChildren<T*>(QString(), Qt::FindDirectChildrenOnly)) {
            if(QObject *object = qobject_cast<QObject*>(derivedObject))
                _objects.append(object);
        }
        _objectCreator = std::bind(&QtPropertyTableModel::defaultChildCreator<T>, parent);
        endResetModel();
    }
    
    /* --------------------------------------------------------------------------------
     * Property editor delegate.
     * Editors commit their value to the model on every change (live update), not only
     * when the editor is closed.
     * -------------------------------------------------------------------------------- */
    class QtPropertyDelegate: public QStyledItemDelegate
    {
    public:
        QtPropertyDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}
        
        QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
        void setEditorData(QWidget *editor, const QModelIndex &index) const override;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
        QString displayText(const QVariant &value, const QLocale &locale) const override;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    
    protected:
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
        
        // Connect the editor's "value changed by user" signal to commitData() so that the model is
        // updated immediately instead of when the editor closes.
        void connectLiveCommit(QWidget *editor) const;
    };
    
    /* --------------------------------------------------------------------------------
     * User types for QVariant that will be handled by QtPropertyDelegate.
     * User types need to be declared via Q_DECLARE_METATYPE (see below outside of namespace)
     *   and also registered via qRegisterMetaType (see static instantiation in .cpp file)
     * -------------------------------------------------------------------------------- */
    
    // For static registration of user types (see static instantiation in QtPropertyEditor.cpp).
    template <typename Type> class MetaTypeRegistration
    {
    public:
        inline MetaTypeRegistration()
        {
            qRegisterMetaType<Type>();
        }
    };
    
    // For push buttons.
    // See Q_DECLARE_METATYPE below and qRegisterMetaType in .cpp file.
    class QtPushButtonActionWrapper
    {
    public:
        QtPushButtonActionWrapper(QAction *action = NULL) : action(action) {}
        QtPushButtonActionWrapper(const QtPushButtonActionWrapper &other) { action = other.action; }
        ~QtPushButtonActionWrapper() {}
        QAction *action = NULL;
    };
    
    /* --------------------------------------------------------------------------------
     * Tree editor for properties in a QObject tree.
     * -------------------------------------------------------------------------------- */
    class QtPropertyTreeEditor : public QTreeView
    {
        Q_OBJECT
        
    public:
        QtPropertyTreeEditor(QWidget *parent = NULL);
        
        // Owns its own tree model for convenience. This means model will be deleted along with editor.
        // However, you're not forced to use this model.
        QtPropertyTreeModel treeModel;
        
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
        QtPropertyTableEditor(QWidget *parent = NULL);
        
        // Owns its own table model for convenience. This means model will be deleted along with editor.
        // However, you're not forced to use this model.
        QtPropertyTableModel tableModel;
        
        bool isDynamic() const { return _isDynamic; }
        void setIsDynamic(bool b);
        
        QSize sizeHint() const override { return getTableSize(this); }
        
    public slots:
        void horizontalHeaderContextMenu(QPoint pos);
        void verticalHeaderContextMenu(QPoint pos);
        void appendRow();
        void insertSelectedRows();
        void removeSelectedRows();
        void handleSectionMove(int logicalIndex, int oldVisualIndex, int newVisualIndex);
        
    protected:
        QtPropertyDelegate _delegate;
        bool _isDynamic = true;
        
        void keyPressEvent(QKeyEvent *event) override;
        bool eventFilter(QObject* o, QEvent* e) override;
    };
    
} // QtPropertyEditor

Q_DECLARE_METATYPE(QtPropertyEditor::QtPushButtonActionWrapper);

#endif // __QtPropertyEditor_H__
