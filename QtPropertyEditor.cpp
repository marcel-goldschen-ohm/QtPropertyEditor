/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QtPropertyEditor.h"

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaType>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStylePainter>
#include <QToolButton>

namespace QtPropertyEditor
{
    static MetaTypeRegistration<QtPushButtonActionWrapper> thisInstantiationRegistersQtPushButtonActionWrapperWithQt;

    static bool isEnumValue(const QVariant &value)
    {
        return value.isValid() && value.metaType().flags().testFlag(QMetaType::IsEnumeration);
    }

    QList<QByteArray> getPropertyNames(QObject *object)
    {
        QList<QByteArray> propertyNames = getMetaPropertyNames(*object->metaObject());
        foreach(const QByteArray &dynamicPropertyName, object->dynamicPropertyNames()) {
            propertyNames << dynamicPropertyName;
        }
        return propertyNames;
    }
    
    QList<QByteArray> getMetaPropertyNames(const QMetaObject &metaObject)
    {
        QList<QByteArray> propertyNames;
        int numProperties = metaObject.propertyCount();
        for(int i = 0; i < numProperties; ++i) {
            const QMetaProperty metaProperty = metaObject.property(i);
            propertyNames << QByteArray(metaProperty.name());
        }
        return propertyNames;
    }
    
    QList<QByteArray> getNoninheritedPropertyNames(QObject *object)
    {
        QList<QByteArray> propertyNames = getPropertyNames(object);
        QList<QByteArray> superPropertyNames = getMetaPropertyNames(*object->metaObject()->superClass());
        foreach(const QByteArray &superPropertyName, superPropertyNames) {
            propertyNames.removeOne(superPropertyName);
        }
        return propertyNames;
    }
    
    QObject* descendant(QObject *object, const QByteArray &pathToDescendantObject)
    {
        // Get descendent object specified by "path.to.descendant", where "path", "to" and "descendant"
        // are the object names of objects with the parent->child relationship object->path->to->descendant.
        if(!object || pathToDescendantObject.isEmpty())
            return 0;
        if(pathToDescendantObject.contains('.')) {
            QList<QByteArray> descendantObjectNames = pathToDescendantObject.split('.');
            foreach(QByteArray name, descendantObjectNames) {
                object = object->findChild<QObject*>(QString(name));
                if(!object)
                    return 0; // Invalid path to descendant object.
            }
            return object;
        }
        return object->findChild<QObject*>(QString(pathToDescendantObject));
    }
    
    QSize getTableSize(const QTableView *table)
    {
        int w = table->verticalHeader()->width() + 4; // +4 seems to be needed
        int h = table->horizontalHeader()->height() + 4;
        for(int i = 0; i < table->model()->columnCount(); i++)
            w += table->columnWidth(i);
        for(int i = 0; i < table->model()->rowCount(); i++)
            h += table->rowHeight(i);
        return QSize(w, h);
    }
    
    void QtAbstractPropertyModel::setProperties(const QString &str)
    {
        // str = "name0: header0, name1, name2, name3: header3 ..."
        propertyNames.clear();
        propertyHeaders.clear();
        QStringList fields = str.split(",", Qt::SkipEmptyParts);
        foreach(const QString &field, fields) {
            if(!field.trimmed().isEmpty())
                addProperty(field);
        }
    }
    
    void QtAbstractPropertyModel::addProperty(const QString &str)
    {
        // "name" OR "name: header"
        if(str.contains(":")) {
            int pos = str.indexOf(":");
            QByteArray propertyName = str.left(pos).trimmed().toUtf8();
            QString propertyHeader = str.mid(pos+1).trimmed();
            propertyNames.push_back(propertyName);
            propertyHeaders[propertyName] = propertyHeader;
        } else {
            QByteArray propertyName = str.trimmed().toUtf8();
            propertyNames.push_back(propertyName);
        }
    }
    
    const QMetaProperty QtAbstractPropertyModel::metaPropertyAtIndex(const QModelIndex &index) const
    {
        QObject *object = objectAtIndex(index);
        if(!object)
            return QMetaProperty();
        QByteArray propertyName = propertyNameAtIndex(index);
        if(propertyName.isEmpty())
            return QMetaProperty();
        // Return metaObject with same name.
        const QMetaObject *metaObject = object->metaObject();
        int numProperties = metaObject->propertyCount();
        for(int i = 0; i < numProperties; ++i) {
            const QMetaProperty metaProperty = metaObject->property(i);
            if(QByteArray(metaProperty.name()) == propertyName)
                return metaProperty;
        }
        return QMetaProperty();
    }
    
    QVariant QtAbstractPropertyModel::data(const QModelIndex &index, int role) const
    {
        if(!index.isValid())
            return QVariant();
        if(role == Qt::DisplayRole || role == Qt::EditRole) {
            QObject *object = objectAtIndex(index);
            if(!object)
                return QVariant();
            QByteArray propertyName = propertyNameAtIndex(index);
            if(propertyName.isEmpty())
                return QVariant();
            return object->property(propertyName.constData());
        }
        return QVariant();
    }
    
    bool QtAbstractPropertyModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if(!index.isValid())
            return false;
        if(role == Qt::EditRole) {
            QObject *object = objectAtIndex(index);
            if(!object)
                return false;
            QByteArray propertyName = propertyNameAtIndex(index);
            if(propertyName.isEmpty())
                return false;
            bool result = object->setProperty(propertyName.constData(), value);
            // Result will be FALSE for dynamic properties, which causes the tree view to lag.
            // So make sure we still return TRUE in this case.
            if(!result && object->dynamicPropertyNames().contains(propertyName))
                result = true;
            // Let the model refresh any cells that depend on the changed value (e.g. read-only mirrors).
            if(result)
                refreshAfterChange(index);
            return result;
        }
        return false;
    }
    
    Qt::ItemFlags QtAbstractPropertyModel::flags(const QModelIndex &index) const
    {
        Qt::ItemFlags flags = QAbstractItemModel::flags(index);
        if(!index.isValid())
            return flags;
        QObject *object = objectAtIndex(index);
        if(!object)
            return flags;
        flags |= Qt::ItemIsEnabled;
        flags |= Qt::ItemIsSelectable;
        QByteArray propertyName = propertyNameAtIndex(index);
        const QMetaProperty metaProperty = metaPropertyAtIndex(index);
        if(metaProperty.isWritable() || object->dynamicPropertyNames().contains(propertyName))
            flags |= Qt::ItemIsEditable;
        return flags;
    }
    
    void QtPropertyTreeModel::Node::setObject(QObject *object, int maxChildDepth, const QList<QByteArray> &propertyNames)
    {
        this->object = object;
        propertyName.clear();
        qDeleteAll(children);
        children.clear();
        if(!object) return;
        
        // Compiled properties (but exclude objectName as this is displayed for the object node itself).
        const QMetaObject *metaObject = object->metaObject();
        int numProperties = metaObject->propertyCount();
        for(int i = 0; i < numProperties; ++i) {
            const QMetaProperty metaProperty = metaObject->property(i);
            QByteArray propertyName = QByteArray(metaProperty.name());
            if(propertyNames.isEmpty() || propertyNames.contains(propertyName)) {
                Node *node = new Node(this);
                node->propertyName = propertyName;
                children.append(node);
            }
        }
        // Dynamic properties.
        QList<QByteArray> dynamicPropertyNames = object->dynamicPropertyNames();
        foreach(const QByteArray &propertyName, dynamicPropertyNames) {
            if(propertyNames.isEmpty() || propertyNames.contains(propertyName)) {
                Node *node = new Node(this);
                node->propertyName = propertyName;
                children.append(node);
            }
        }
        // Child objects.
        if(maxChildDepth > 0 || maxChildDepth == -1) {
            if(maxChildDepth > 0)
                --maxChildDepth;
            QMap<QByteArray, QObjectList> childMap;
            foreach(QObject *child, object->children()) {
                childMap[QByteArray(child->metaObject()->className())].append(child);
            }
            for(auto it = childMap.begin(); it != childMap.end(); ++it) {
                foreach(QObject *child, it.value()) {
                    Node *node = new Node(this);
                    node->setObject(child, maxChildDepth, propertyNames);
                    children.append(node);
                }
            }
        }
    }
    
    QtPropertyTreeModel::Node* QtPropertyTreeModel::nodeAtIndex(const QModelIndex &index) const
    {
        try {
            return static_cast<Node*>(index.internalPointer());
        } catch(...) {
            return NULL;
        }
    }
    
    QObject* QtPropertyTreeModel::objectAtIndex(const QModelIndex &index) const
    {
        // If node is an object, return the node's object.
        // Else if node is a property, return the parent node's object.
        Node *node = nodeAtIndex(index);
        if(!node) return NULL;
        if(node->object) return node->object;
        if(node->parent) return node->parent->object;
        return NULL;
    }
    
    QByteArray QtPropertyTreeModel::propertyNameAtIndex(const QModelIndex &index) const
    {
        // If node is a property, return the node's property name.
        // Else if node is an object, return "objectName".
        Node *node = nodeAtIndex(index);
        if(!node) return QByteArray();
        if(!node->propertyName.isEmpty()) return node->propertyName;
        return QByteArray();
    }
    
    QModelIndex QtPropertyTreeModel::index(int row, int column, const QModelIndex &parent) const
    {
        // Return a model index whose internal pointer references the appropriate tree node.
        if(column < 0 || column >= 2 || !hasIndex(row, column, parent))
            return QModelIndex();
        const Node *parentNode = parent.isValid() ? nodeAtIndex(parent) : &_root;
        if(!parentNode || row < 0 || row >= parentNode->children.size())
            return QModelIndex();
        Node *node = parentNode->children.at(row);
        return node ? createIndex(row, column, node) : QModelIndex();
    }
    
    QModelIndex QtPropertyTreeModel::parent(const QModelIndex &index) const
    {
        // Return a model index for parent node (column must be 0).
        if(!index.isValid())
            return QModelIndex();
        Node *node = nodeAtIndex(index);
        if(!node)
            return QModelIndex();
        Node *parentNode = node->parent;
        if(!parentNode || parentNode == &_root)
            return QModelIndex();
        int row = 0;
        Node *grandparentNode = parentNode->parent;
        if(grandparentNode)
            row = grandparentNode->children.indexOf(parentNode);
        return createIndex(row, 0, parentNode);
    }
    
    int QtPropertyTreeModel::rowCount(const QModelIndex &parent) const
    {
        // Return number of child nodes.
        const Node *parentNode = parent.isValid() ? nodeAtIndex(parent) : &_root;
        return parentNode ? parentNode->children.size() : 0;
    }
    
    int QtPropertyTreeModel::columnCount(const QModelIndex &parent) const
    {
        // Return 2 for name/value columns.
        const Node *parentNode = parent.isValid() ? nodeAtIndex(parent) : &_root;
        return (parentNode ? 2 : 0);
    }
    
    QVariant QtPropertyTreeModel::data(const QModelIndex &index, int role) const
    {
        if(!index.isValid())
            return QVariant();
        if(role == Qt::DisplayRole || role == Qt::EditRole) {
            QObject *object = objectAtIndex(index);
            if(!object)
                return QVariant();
            QByteArray propertyName = propertyNameAtIndex(index);
            if(index.column() == 0) {
                // Object's class name or else the property name.
                if(propertyName.isEmpty())
                    return QVariant(object->metaObject()->className());
                else if(propertyHeaders.contains(propertyName))
                    return QVariant(propertyHeaders[propertyName]);
                else
                    return QVariant(propertyName);
            } else if(index.column() == 1) {
                // Object's objectName or else the property value.
                if(propertyName.isEmpty())
                    return QVariant(object->objectName());
                else
                    return object->property(propertyName.constData());
            }
        }
        return QVariant();
    }
    
    bool QtPropertyTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if(!index.isValid())
            return false;
        if(role == Qt::EditRole) {
            QObject *object = objectAtIndex(index);
            if(!object)
                return false;
            QByteArray propertyName = propertyNameAtIndex(index);
            if(index.column() == 0) {
                // Object class name or property name.
                return false;
            } else if(index.column() == 1) {
                // Object's objectName or else the property value.
                if(propertyName.isEmpty()) {
                    object->setObjectName(value.toString());
                    refreshAfterChange(index);
                    return true;
                } else {
                    bool result = object->setProperty(propertyName.constData(), value);
                    // Result will be FALSE for dynamic properties, which causes the tree view to lag.
                    // So make sure we still return TRUE in this case.
                    if(!result && object->dynamicPropertyNames().contains(propertyName))
                        result = true;
                    // Refresh sibling properties (e.g. read-only mirrors of the changed property).
                    if(result)
                        refreshAfterChange(index);
                    return result;
                }
            }
        }
        return false;
    }
    
    void QtPropertyTreeModel::refreshAfterChange(const QModelIndex &index)
    {
        // All properties of an object are siblings under the same parent node, so refresh them all.
        QModelIndex parentIndex = index.parent();
        int rows = rowCount(parentIndex);
        int cols = columnCount(parentIndex);
        if(rows > 0 && cols > 0)
            emit dataChanged(this->index(0, 0, parentIndex), this->index(rows - 1, cols - 1, parentIndex));
    }
    
    Qt::ItemFlags QtPropertyTreeModel::flags(const QModelIndex &index) const
    {
        Qt::ItemFlags flags = QAbstractItemModel::flags(index);
        if(!index.isValid())
            return flags;
        QObject *object = objectAtIndex(index);
        if(!object)
            return flags;
        flags |= Qt::ItemIsEnabled;
        flags |= Qt::ItemIsSelectable;
        if(index.column() == 1) {
            QByteArray propertyName = propertyNameAtIndex(index);
            const QMetaProperty metaProperty = metaPropertyAtIndex(index);
            if(metaProperty.isWritable() || object->dynamicPropertyNames().contains(propertyName))
                flags |= Qt::ItemIsEditable;
        }
        return flags;
    }
    
    QVariant QtPropertyTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if(role == Qt::DisplayRole) {
            if(orientation == Qt::Horizontal) {
                if(section == 0)
                    return QVariant("Name");
                else if(section == 1)
                    return QVariant("Value");
            }
        }
        return QVariant();
    }
    
    QObject* QtPropertyTableModel::objectAtIndex(const QModelIndex &index) const
    {
        if(_objects.size() <= index.row())
            return 0;
        QObject *object = _objects.at(index.row());
        // If property names are specified, check if name at column is a path to a child object property.
        if(!propertyNames.isEmpty()) {
            if(propertyNames.size() > index.column()) {
                QByteArray propertyName = propertyNames.at(index.column());
                if(propertyName.contains('.')) {
                    int pos = propertyName.lastIndexOf('.');
                    return descendant(object, propertyName.left(pos));
                }
            }
        }
        return object;
    }
    
    QByteArray QtPropertyTableModel::propertyNameAtIndex(const QModelIndex &index) const
    {
        // If property names are specified, return the name at column.
        if(!propertyNames.isEmpty()) {
            if(propertyNames.size() > index.column()) {
                QByteArray propertyName = propertyNames.at(index.column());
                if(propertyName.contains('.')) {
                    int pos = propertyName.lastIndexOf('.');
                    return propertyName.mid(pos + 1);
                }
                return propertyName;
            }
            return QByteArray();
        }
        // If property names are NOT specified, return the metaObject's property name at column.
        QObject *object = objectAtIndex(index);
        if(!object)
            return QByteArray();
        const QMetaObject *metaObject = object->metaObject();
        int numProperties = metaObject->propertyCount();
        if(numProperties > index.column())
            return QByteArray(metaObject->property(index.column()).name());
        // If column is greater than the number of metaObject properties, check for dynamic properties.
        const QList<QByteArray> &dynamicPropertyNames = object->dynamicPropertyNames();
        if(numProperties + dynamicPropertyNames.size() > index.column())
            return dynamicPropertyNames.at(index.column() - numProperties);
        return QByteArray();
    }
    
    QModelIndex QtPropertyTableModel::index(int row, int column, const QModelIndex &/* parent */) const
    {
        return createIndex(row, column);
    }
    
    QModelIndex QtPropertyTableModel::parent(const QModelIndex &/* index */) const
    {
        return QModelIndex();
    }
    
    int QtPropertyTableModel::rowCount(const QModelIndex &/* parent */) const
    {
        return _objects.size();
    }
    
    int QtPropertyTableModel::columnCount(const QModelIndex &/* parent */) const
    {
        // Number of properties.
        if(!propertyNames.isEmpty())
            return propertyNames.size();
        if(_objects.isEmpty())
            return 0;
        QObject *object = _objects.at(0);
        const QMetaObject *metaObject = object->metaObject();
        return metaObject->propertyCount() + object->dynamicPropertyNames().size();
    }
    
    QVariant QtPropertyTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if(role == Qt::DisplayRole) {
            if(orientation == Qt::Vertical) {
                return QVariant(section);
            } else if(orientation == Qt::Horizontal) {
                QByteArray propertyName = propertyNameAtIndex(createIndex(0, section));
                QByteArray childPath;
                if(propertyNames.size() > section) {
                    QByteArray pathToPropertyName = propertyNames.at(section);
                    if(pathToPropertyName.contains('.')) {
                        int pos = pathToPropertyName.lastIndexOf('.');
                        childPath = pathToPropertyName.left(pos + 1);
                    }
                }
                if(propertyHeaders.contains(propertyName))
                    return QVariant(childPath + propertyHeaders.value(propertyName));
                return QVariant(childPath + propertyName);
            }
        }
        return QVariant();
    }
    
    void QtPropertyTableModel::refreshAfterChange(const QModelIndex &index)
    {
        // All properties of an object are the columns of one row, so refresh the whole row.
        int cols = columnCount();
        if(index.row() >= 0 && cols > 0)
            emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), cols - 1));
    }
    
    bool QtPropertyTableModel::insertRows(int row, int count, const QModelIndex &parent)
    {
        // Only valid if we have an object creator method.
        if(!_objectCreator)
            return false;
        bool columnCountWillAlsoChange = _objects.isEmpty() && propertyNames.isEmpty();
        beginInsertRows(parent, row, row + count - 1);
        for(int i = row; i < row + count; ++i) {
            QObject *object = _objectCreator();
            _objects.insert(i, object);
        }
        endInsertRows();
        if(row + count < _objects.size())
            reorderChildObjectsToMatchRowOrder(row + count);
        if(columnCountWillAlsoChange) {
            beginResetModel();
            endResetModel();
        }
        emit rowCountChanged();
        return true;
    }
    
    bool QtPropertyTableModel::removeRows(int row, int count, const QModelIndex &parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        for(int i = row; i < row + count; ++i)
            delete _objects.at(i);
        QObjectList::iterator begin = _objects.begin() + row;
        _objects.erase(begin, begin + count);
        endRemoveRows();
        emit rowCountChanged();
        return true;
    }
    
    bool QtPropertyTableModel::moveRows(const QModelIndex &/*sourceParent*/, int sourceRow, int count, const QModelIndex &/*destinationParent*/, int destinationRow)
    {
        beginResetModel();
        QObjectList objectsToMove;
        for(int i = sourceRow; i < sourceRow + count; ++i)
            objectsToMove.append(_objects.takeAt(sourceRow));
        for(int i = 0; i < objectsToMove.size(); ++i) {
            if(destinationRow + i >= _objects.size())
                _objects.append(objectsToMove.at(i));
            else
                _objects.insert(destinationRow + i, objectsToMove.at(i));
        }
        endResetModel();
        reorderChildObjectsToMatchRowOrder(sourceRow <= destinationRow ? sourceRow : destinationRow);
        emit rowOrderChanged();
        return true;
    }
    
    void QtPropertyTableModel::reorderChildObjectsToMatchRowOrder(int firstRow)
    {
        for(int i = firstRow; i < rowCount(); ++i) {
            QObject *object = objectAtIndex(createIndex(i, 0));
            if(object) {
                QObject *parent = object->parent();
                if(parent) {
                    object->setParent(NULL);
                    object->setParent(parent);
                }
            }
        }
    }

    // True if the editor (or one of its child widgets, e.g. the line edit inside a spin box)
    // currently has the keyboard focus, i.e. a change is coming from the user.
    static bool editorHasFocus(QWidget *editor)
    {
        QWidget *fw = QApplication::focusWidget();
        return editor && fw && (editor == fw || editor->isAncestorOf(fw));
    }
    
    void QtPropertyDelegate::connectLiveCommit(QWidget *editor) const
    {
        if(!editor)
            return;
        // commitData() is a signal, which we need to emit from this const method.
        QtPropertyDelegate *self = const_cast<QtPropertyDelegate*>(this);
        
        if(QLineEdit *e = qobject_cast<QLineEdit*>(editor)) {
            // textEdited is only emitted for user edits (not for programmatic setText()).
            connect(e, &QLineEdit::textEdited, self, [self, e]() { emit self->commitData(e); });
        } else if(QComboBox *e = qobject_cast<QComboBox*>(editor)) {
            connect(e, QOverload<int>::of(&QComboBox::activated), self, [self, e]() { emit self->commitData(e); });
        } else if(QSpinBox *e = qobject_cast<QSpinBox*>(editor)) {
            e->setKeyboardTracking(true);
            connect(e, QOverload<int>::of(&QSpinBox::valueChanged), self, [self, e]() {
                if(editorHasFocus(e)) emit self->commitData(e);
            });
        } else if(QDoubleSpinBox *e = qobject_cast<QDoubleSpinBox*>(editor)) {
            e->setKeyboardTracking(true);
            connect(e, QOverload<double>::of(&QDoubleSpinBox::valueChanged), self, [self, e]() {
                if(editorHasFocus(e)) emit self->commitData(e);
            });
        } else if(QDateTimeEdit *e = qobject_cast<QDateTimeEdit*>(editor)) {
            // Also covers QDateEdit and QTimeEdit.
            connect(e, &QDateTimeEdit::dateTimeChanged, self, [self, e]() {
                if(editorHasFocus(e)) emit self->commitData(e);
            });
        }
    }

    QWidget* QtPropertyDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QVariant value = index.data(Qt::DisplayRole);
        if(value.isValid()) {
            // If the value is an enum, create a QComboBox with the enum's keys as items.
            if(isEnumValue(value)) {
                const QtAbstractPropertyModel *propertyModel = qobject_cast<const QtAbstractPropertyModel*>(index.model());
                if(propertyModel) {
                    const QMetaProperty metaProperty = propertyModel->metaPropertyAtIndex(index);
                    if(metaProperty.isValid() && metaProperty.isEnumType()) {
                        const QMetaEnum metaEnum = metaProperty.enumerator();
                        QComboBox *editor = new QComboBox(parent);
                        for(int j = 0; j < metaEnum.keyCount(); ++j)
                            editor->addItem(QString::fromLatin1(metaEnum.key(j)), metaEnum.value(j));
                        editor->setCurrentIndex(editor->findData(value.toInt()));
                        // Commit selected value immediately on selection.
                        connectLiveCommit(editor);
                        return editor;
                    }
                }
            } else if(value.typeId() == QMetaType::Bool) {
                // We want a check box, but instead of creating an editor widget we'll just directly
                // draw the check box in paint() and handle mouse clicks in editorEvent().
                // Here, we'll just return NULL to make sure that no editor is created when this cell is double clicked.
                return NULL;
            } else if(value.typeId() == QMetaType::Double) {
                // Return a QLineEdit to enter double values with arbitrary precision and scientific notation.
                QLineEdit *editor = new QLineEdit(parent);
                editor->setText(value.toString());
                connectLiveCommit(editor);
                return editor;
            } else if(value.typeId() == QMetaType::QSize ||
                       value.typeId() == QMetaType::QSizeF ||
                      value.typeId() == QMetaType::QPoint ||
                       value.typeId() == QMetaType::QPointF ||
                      value.typeId() == QMetaType::QRect ||
                       value.typeId() == QMetaType::QRectF) {
                // Return a QLineEdit. Parsing will be done in displayText() and setEditorData().
                QLineEdit *editor = new QLineEdit(parent);
                editor->setText(displayText(value, QLocale()));
                connectLiveCommit(editor);
                return editor;
            } else if(value.canConvert<QtPushButtonActionWrapper>()) {
                // We want a push button, but instead of creating an editor widget we'll just directly
                // draw the button in paint() and handle mouse clicks in editorEvent().
                // Here, we'll just return NULL to make sure that no editor is created when this cell is double clicked.
                return NULL;
            }
        }
        // Default editors (QLineEdit for strings, QSpinBox for ints, QDateTimeEdit for date/times, ...).
        QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
        connectLiveCommit(editor);
        return editor;
    }
    
    void QtPropertyDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        // With live commits, every edit makes the model emit dataChanged(), which makes the view call
        // setEditorData() on the editor that is currently being typed in. Re-populating it would reset
        // the text/cursor position, so skip editors that have (or contain) the keyboard focus.
        // The editor already holds the newest value anyway, since it is the source of the change.
        // On creation the editor does not have focus yet, so it is still populated initially.
        QWidget *focusWidget = QApplication::focusWidget();
        if(editor && focusWidget && (editor == focusWidget || editor->isAncestorOf(focusWidget)))
            return;
        QStyledItemDelegate::setEditorData(editor, index);
    }
    
    void QtPropertyDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        QVariant value = index.data(Qt::DisplayRole);
        if(value.isValid()) {
            // If the value is an enum, set the model's data to the enum value corresponding to the selected key in the QComboBox editor.
            if(isEnumValue(value)) {
                QComboBox *comboBoxEditor = qobject_cast<QComboBox*>(editor);
                const QtAbstractPropertyModel *propertyModel = qobject_cast<const QtAbstractPropertyModel*>(model);
                if(comboBoxEditor && propertyModel) {
                    const QMetaProperty metaProperty = propertyModel->metaPropertyAtIndex(index);
                    if(metaProperty.isValid() && metaProperty.isEnumType()) {
                        bool ok = false;
                        const int selectedValue = metaProperty.enumerator().keyToValue(comboBoxEditor->currentText().toLatin1().constData(), &ok);
                        if(ok)
                            model->setData(index, QVariant(metaProperty.metaType(), &selectedValue), Qt::EditRole);
                        return;
                    }
                }
            } else if(value.typeId() == QMetaType::Double) {
                // Set model's double value data to numeric representation in QLineEdit editor.
                // Conversion from text to number handled by QVariant.
                // Half-typed text such as "1e-" does not parse and is simply ignored until it becomes valid.
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    QVariant value = QVariant(lineEditor->text());
                    bool ok;
                    double dval = value.toDouble(&ok);
                    if(ok)
                        model->setData(index, QVariant(dval), Qt::EditRole);
                    return;
                }
            } else if(value.typeId() == QMetaType::QSize) {
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    // Parse formats: (w x h) or (w,h) or (w h) <== () are optional
                    QRegularExpression regex("\\s*\\(?\\s*(\\d+)\\s*[x,\\s]\\s*(\\d+)\\s*\\)?\\s*");
                    QRegularExpressionMatch match = regex.match(lineEditor->text().trimmed());
                    if(match.hasMatch() && match.capturedTexts().size() == 3) {
                        bool wok, hok;
                        int w = match.captured(1).toInt(&wok);
                        int h = match.captured(2).toInt(&hok);
                        if(wok && hok)
                            model->setData(index, QVariant(QSize(w, h)), Qt::EditRole);
                    }
                    return;
                }
            } else if(value.typeId() == QMetaType::QSizeF) {
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    // Parse formats: (w x h) or (w,h) or (w h) <== () are optional
                    QRegularExpression regex("\\s*\\(?\\s*([0-9\\+\\-\\.eE]+)\\s*[x,\\s]\\s*([0-9\\+\\-\\.eE]+)\\s*\\)?\\s*");
                    QRegularExpressionMatch match = regex.match(lineEditor->text().trimmed());
                    if(match.hasMatch() && match.capturedTexts().size() == 3) {
                        bool wok, hok;
                        double w = match.captured(1).toDouble(&wok);
                        double h = match.captured(2).toDouble(&hok);
                        if(wok && hok)
                            model->setData(index, QVariant(QSizeF(w, h)), Qt::EditRole);
                    }
                    return;
                }
            } else if(value.typeId() == QMetaType::QPoint) {
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    // Parse formats: (x,y) or (x y) <== () are optional
                    QRegularExpression regex("\\s*\\(?\\s*(\\d+)\\s*[x,\\s]\\s*(\\d+)\\s*\\)?\\s*");
                    QRegularExpressionMatch match = regex.match(lineEditor->text().trimmed());
                    if(match.hasMatch() && match.capturedTexts().size() == 3) {
                        bool xok, yok;
                        int x = match.captured(1).toInt(&xok);
                        int y = match.captured(2).toInt(&yok);
                        if(xok && yok)
                            model->setData(index, QVariant(QPoint(x, y)), Qt::EditRole);
                    }
                    return;
                }
            } else if(value.typeId() == QMetaType::QPointF) {
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    // Parse formats: (x,y) or (x y) <== () are optional
                    QRegularExpression regex("\\s*\\(?\\s*([0-9\\+\\-\\.eE]+)\\s*[x,\\s]\\s*([0-9\\+\\-\\.eE]+)\\s*\\)?\\s*");
                    QRegularExpressionMatch match = regex.match(lineEditor->text().trimmed());
                    if(match.hasMatch() && match.capturedTexts().size() == 3) {
                        bool xok, yok;
                        double x = match.captured(1).toDouble(&xok);
                        double y = match.captured(2).toDouble(&yok);
                        if(xok && yok)
                            model->setData(index, QVariant(QPointF(x, y)), Qt::EditRole);
                    }
                    return;
                }
            } else if(value.typeId() == QMetaType::QRect) {
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    // Parse formats: [Point,Size] or [Point Size] <== [] are optional
                    // Point formats: (x,y) or (x y) <== () are optional
                    // Size formats: (w x h) or (w,h) or (w h) <== () are optional
                    QRegularExpression regex("\\s*\\[?"
                                             "\\s*\\(?\\s*(\\d+)\\s*[,\\s]\\s*(\\d+)\\s*\\)?\\s*"
                                             "[,\\s]"
                                             "\\s*\\(?\\s*(\\d+)\\s*[x,\\s]\\s*(\\d+)\\s*\\)?\\s*"
                                             "\\]?\\s*");
                    QRegularExpressionMatch match = regex.match(lineEditor->text().trimmed());
                    if(match.hasMatch() && match.capturedTexts().size() == 5) {
                        bool xok, yok, wok, hok;
                        int x = match.captured(1).toInt(&xok);
                        int y = match.captured(2).toInt(&yok);
                        int w = match.captured(3).toInt(&wok);
                        int h = match.captured(4).toInt(&hok);
                        if(xok && yok && wok && hok)
                            model->setData(index, QVariant(QRect(x, y, w, h)), Qt::EditRole);
                    }
                    return;
                }
            } else if(value.typeId() == QMetaType::QRectF) {
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    // Parse formats: [Point,Size] or [Point Size] <== [] are optional
                    // Point formats: (x,y) or (x y) <== () are optional
                    // Size formats: (w x h) or (w,h) or (w h) <== () are optional
                    QRegularExpression regex("\\s*\\[?"
                                             "\\s*\\(?\\s*([0-9\\+\\-\\.eE]+)\\s*[,\\s]\\s*([0-9\\+\\-\\.eE]+)\\s*\\)?\\s*"
                                             "[,\\s]"
                                             "\\s*\\(?\\s*([0-9\\+\\-\\.eE]+)\\s*[x,\\s]\\s*([0-9\\+\\-\\.eE]+)\\s*\\)?\\s*"
                                             "\\]?\\s*");
                    QRegularExpressionMatch match = regex.match(lineEditor->text().trimmed());
                    if(match.hasMatch() && match.capturedTexts().size() == 5) {
                        bool xok, yok, wok, hok;
                        double x = match.captured(1).toDouble(&xok);
                        double y = match.captured(2).toDouble(&yok);
                        double w = match.captured(3).toDouble(&wok);
                        double h = match.captured(4).toDouble(&hok);
                        if(xok && yok && wok && hok)
                            model->setData(index, QVariant(QRectF(x, y, w, h)), Qt::EditRole);
                    }
                    return;
                }
    //        } else if(value.type() == QVariant::Color) {
    //            QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
    //            if(lineEditor) {
    //                // Parse formats: (r,g,b) or (r g b) or (r,g,b,a) or (r g b a) <== () are optional
    //                QRegularExpression regex("\\s*\\(?"
    //                                         "\\s*(\\d+)\\s*"
    //                                         "[,\\s]\\s*(\\d+)\\s*"
    //                                         "[,\\s]\\s*(\\d+)\\s*"
    //                                         "([,\\s]\\s*(\\d+)\\s*)?"
    //                                         "\\)?\\s*");
    //                QRegularExpressionMatch match = regex.match(lineEditor->text().trimmed());
    //                if(match.hasMatch() && (match.capturedTexts().size() == 4 || match.capturedTexts().size() == 5)) {
    //                    bool rok, gok, bok, aok;
    //                    int r = match.captured(1).toInt(&rok);
    //                    int g = match.captured(2).toInt(&gok);
    //                    int b = match.captured(3).toInt(&bok);
    //                    if(match.capturedTexts().size() == 4) {
    //                        if(rok && gok && bok)
    //                            model->setData(index, QColor(r, g, b), Qt::EditRole);
    //                    } else if(match.capturedTexts().size() == 5) {
    //                        int a = match.captured(4).toInt(&aok);
    //                        if(rok && gok && bok && aok)
    //                            model->setData(index, QColor(r, g, b, a), Qt::EditRole);
    //                    }
    //                }
    //            }
            }
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }
    
    QString QtPropertyDelegate::displayText(const QVariant &value, const QLocale &locale) const
    {
        if(value.isValid()) {
            if(value.typeId() == QMetaType::QSize) {
                // w x h
                QSize size = value.toSize();
                return QString::number(size.width()) + QString(" x ") + QString::number(size.height());
            } else if(value.typeId() == QMetaType::QSizeF) {
                // w x h
                QSizeF size = value.toSizeF();
                return QString::number(size.width()) + QString(" x ") + QString::number(size.height());
            } else if(value.typeId() == QMetaType::QPoint) {
                // (x, y)
                QPoint point = value.toPoint();
                return QString("(")
                + QString::number(point.x()) + QString(", ") + QString::number(point.y())
                + QString(")");
            } else if(value.typeId() == QMetaType::QPointF) {
                // (x, y)
                QPointF point = value.toPointF();
                return QString("(")
                + QString::number(point.x()) + QString(", ") + QString::number(point.y())
                + QString(")");
            } else if(value.typeId() == QMetaType::QRect) {
                // [(x, y), w x h]
                QRect rect = value.toRect();
                return QString("[(")
                + QString::number(rect.x()) + QString(", ") + QString::number(rect.y())
                + QString("), ")
                + QString::number(rect.width()) + QString(" x ") + QString::number(rect.height())
                + QString("]");
            } else if(value.typeId() == QMetaType::QRectF) {
                // [(x, y), w x h]
                QRectF rect = value.toRectF();
                return QString("[(")
                + QString::number(rect.x()) + QString(", ") + QString::number(rect.y())
                + QString("), ")
                + QString::number(rect.width()) + QString(" x ") + QString::number(rect.height())
                + QString("]");
    //        } else if(value.type() == QVariant::Color) {
    //            // (r, g, b, a)
    //            QColor color = value.value<QColor>();
    //            return QString("(")
    //                    + QString::number(color.red()) + QString(", ") + QString::number(color.green()) + QString(", ")
    //                    + QString::number(color.blue()) + QString(", ") + QString::number(color.alpha())
    //                    + QString(")");
            }
        }
        return QStyledItemDelegate::displayText(value, locale);
    }
    
    void QtPropertyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QVariant value = index.data(Qt::DisplayRole);
        if(value.isValid()) {
            // If the value is an enum, draw the enum's key instead of the numeric value.
            if(isEnumValue(value)) {
                const QtAbstractPropertyModel *propertyModel = qobject_cast<const QtAbstractPropertyModel*>(index.model());
                if(propertyModel) {
                    const QMetaProperty metaProperty = propertyModel->metaPropertyAtIndex(index);
                    if(metaProperty.isValid() && metaProperty.isEnumType()) {
                        QStyleOptionViewItem itemOption(option);
                        initStyleOption(&itemOption, index);
                        itemOption.text = QString::fromLatin1(metaProperty.enumerator().valueToKey(value.toInt()));
                        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter);
                        return;
                    }
                }
            } else if(value.typeId() == QMetaType::Bool) {
                bool checked = value.toBool();
                QStyleOptionButton buttonOption;
                buttonOption.state |= QStyle::State_Active; // Required!
                buttonOption.state |= ((index.flags() & Qt::ItemIsEditable) ? QStyle::State_Enabled : QStyle::State_ReadOnly);
                buttonOption.state |= (checked ? QStyle::State_On : QStyle::State_Off);
                QRect checkBoxRect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &buttonOption); // Only used to get size of native checkbox widget.
                buttonOption.rect = QStyle::alignedRect(option.direction, Qt::AlignLeft, checkBoxRect.size(), option.rect); // Our checkbox rect.
                QApplication::style()->drawControl(QStyle::CE_CheckBox, &buttonOption, painter);
                return;
            } else if(value.canConvert<QtPushButtonActionWrapper>()) {
                QAction *action = value.value<QtPushButtonActionWrapper>().action;
                QStyleOptionButton buttonOption;
                buttonOption.state = QStyle::State_Active | QStyle::State_Raised;
                //buttonOption.features = QStyleOptionButton::DefaultButton;
                if(action) buttonOption.text = action->text();
                buttonOption.rect = option.rect;
                //buttonOption.rect = QRect(option.rect.x() + 5, option.rect.y() + 5, option.rect.width() - 10, option.rect.height() - 10);
                QApplication::style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter);
                return;
            }
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
    
    bool QtPropertyDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
    {
        QVariant value = index.data(Qt::DisplayRole);
        if(value.isValid()) {
            if(value.typeId() == QMetaType::Bool) {
                if(event->type() == QEvent::MouseButtonDblClick)
                    return false;
                if(event->type() != QEvent::MouseButtonRelease)
                    return false;
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if(mouseEvent->button() != Qt::LeftButton)
                    return false;
                // option.rect ==> cell
                // Here, we choose to allow clicks anywhere in the cell to toggle the checkbox.
                if(!option.rect.contains(mouseEvent->pos()))
                    return false;
                bool checked = value.toBool();
                QVariant newValue(!checked); // Toggle model's bool value.
                // The model refreshes all dependent cells (e.g. read-only mirrors) in setData().
                return model->setData(index, newValue, Qt::EditRole);
            } else if(value.canConvert<QtPushButtonActionWrapper>()) {
                if(event->type() != QEvent::MouseButtonRelease)
                    return false;
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if(mouseEvent->button() != Qt::LeftButton)
                    return false;
                if(!option.rect.contains(mouseEvent->pos()))
                    return false;
                QAction *action = value.value<QtPushButtonActionWrapper>().action;
                if(action) action->trigger();
                return true;
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
    
    QtPropertyTreeEditor::QtPropertyTreeEditor(QWidget *parent) : QTreeView(parent)
    {
        setItemDelegate(&_delegate);
        setAlternatingRowColors(true);
        setModel(&treeModel);
        setEditTriggers(QAbstractItemView::AllEditTriggers);
    }
    
    void QtPropertyTreeEditor::resizeColumnsToContents()
    {
        resizeColumnToContents(0);
        resizeColumnToContents(1);
    }
    
    QtPropertyTableEditor::QtPropertyTableEditor(QWidget *parent) : QTableView(parent)
    {
        setItemDelegate(&_delegate);
        setAlternatingRowColors(true);
        setModel(&tableModel);
        setEditTriggers(QAbstractItemView::AllEditTriggers);
        verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        setIsDynamic(_isDynamic);
        
        // Draggable rows.
        verticalHeader()->setSectionsMovable(_isDynamic);
        connect(verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(handleSectionMove(int, int, int)));
        
        // Header context menus.
        horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(horizontalHeaderContextMenu(QPoint)));
        connect(verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(verticalHeaderContextMenu(QPoint)));
        
        // Custom corner button.
        if(QAbstractButton *cornerButton = findChild<QAbstractButton*>()) {
            cornerButton->installEventFilter(this);
        }
    }
    
    void QtPropertyTableEditor::setIsDynamic(bool b)
    {
        _isDynamic = b;
        
        // Dragging rows.
        verticalHeader()->setSectionsMovable(_isDynamic);
        
        // Corner button.
        if(QAbstractButton *cornerButton = findChild<QAbstractButton*>()) {
            if(_isDynamic) {
                cornerButton->disconnect(SIGNAL(clicked()));
                connect(cornerButton, SIGNAL(clicked()), this, SLOT(appendRow()));
                cornerButton->setText("+");
                cornerButton->setToolTip("Append row");
            } else {
                cornerButton->disconnect(SIGNAL(clicked()));
                connect(cornerButton, SIGNAL(clicked()), this, SLOT(selectAll()));
                cornerButton->setText("");
                cornerButton->setToolTip("Select all");
            }
            // adjust the width of the vertical header to match the preferred corner button width
            // (unfortunately QAbstractButton doesn't implement any size hinting functionality)
            QStyleOptionHeader opt;
            opt.text = cornerButton->text();
            //opt.icon = cornerButton->icon();

            QSize s = cornerButton->style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize(), cornerButton).expandedTo(QSize(10, 10));

            if(s.isValid()) {
                verticalHeader()->setMinimumWidth(s.width());
            }
        }
    }
    
    void QtPropertyTableEditor::horizontalHeaderContextMenu(QPoint pos)
    {
        QModelIndexList indexes = selectionModel()->selectedColumns();
        QMenu *menu = new QMenu;
        menu->addAction("Resize Columns To Contents", this, SLOT(resizeColumnsToContents()));
        menu->popup(horizontalHeader()->viewport()->mapToGlobal(pos));
    }
    
    void QtPropertyTableEditor::verticalHeaderContextMenu(QPoint pos)
    {
        QModelIndexList indexes = selectionModel()->selectedRows();
        QMenu *menu = new QMenu;
        if(_isDynamic) {
            QtPropertyTableModel *propertyTableModel = qobject_cast<QtPropertyTableModel*>(model());
            if(propertyTableModel->objectCreator()) {
                menu->addAction("Append Row", this, SLOT(appendRow()));
            }
            if(indexes.size()) {
                if(propertyTableModel->objectCreator()) {
                    menu->addSeparator();
                    menu->addAction("Insert Rows", this, SLOT(insertSelectedRows()));
                    menu->addSeparator();
                }
                menu->addAction("Delete Rows", this, SLOT(removeSelectedRows()));
            }
        }
        menu->popup(verticalHeader()->viewport()->mapToGlobal(pos));
    }
    
    void QtPropertyTableEditor::appendRow()
    {
        if(!_isDynamic)
            return;
        QtPropertyTableModel *propertyTableModel = qobject_cast<QtPropertyTableModel*>(model());
        if(!propertyTableModel || !propertyTableModel->objectCreator())
            return;
        model()->insertRows(model()->rowCount(), 1);
    }
    
    void QtPropertyTableEditor::insertSelectedRows()
    {
        if(!_isDynamic)
            return;
        QtPropertyTableModel *propertyTableModel = qobject_cast<QtPropertyTableModel*>(model());
        if(!propertyTableModel || !propertyTableModel->objectCreator())
            return;
        QModelIndexList indexes = selectionModel()->selectedRows();
        if(indexes.size() == 0)
            return;
        QList<int> rows;
        foreach(const QModelIndex &index, indexes) {
            rows.append(index.row());
        }

        QVector<int> vec = rows.toVector();
        std::sort(vec.begin(), vec.end());
        rows = QList<int>::fromVector(vec);

        model()->insertRows(rows.at(0), rows.size());
    }
    
    void QtPropertyTableEditor::removeSelectedRows()
    {
        if(!_isDynamic)
            return;
        QModelIndexList indexes = selectionModel()->selectedRows();
        if(indexes.size() == 0)
            return;
        QList<int> rows;
        foreach(const QModelIndex &index, indexes) {
            rows.append(index.row());
        }

        QVector<int> vec = rows.toVector();
        std::sort(vec.begin(), vec.end());
        rows = QList<int>::fromVector(vec);


        for(int i = rows.size() - 1; i >= 0; --i) {
            model()->removeRows(rows.at(i), 1);
        }
    }
    
    void QtPropertyTableEditor::handleSectionMove(int /* logicalIndex */, int oldVisualIndex, int newVisualIndex)
    {
        if(!_isDynamic)
            return;
        QtPropertyTableModel *propertyTableModel = qobject_cast<QtPropertyTableModel*>(model());
        if(!propertyTableModel)
            return;
        // Move objects in the model, and then move the sections back to maintain logicalIndex order.
        propertyTableModel->moveRows(QModelIndex(), oldVisualIndex, 1, QModelIndex(), newVisualIndex);
        disconnect(verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(handleSectionMove(int, int, int)));
        verticalHeader()->moveSection(newVisualIndex, oldVisualIndex);
        connect(verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(handleSectionMove(int, int, int)));
    }
    
    void QtPropertyTableEditor::keyPressEvent(QKeyEvent *event)
    {
        switch(event->key()) {
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
                if(_isDynamic && QMessageBox::question(this, "Delete Rows?", "Delete selected rows?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    removeSelectedRows();
                }
                break;
                
            case Qt::Key_Plus:
                appendRow();
                break;
                
            default:
                break;
        }
    }
    
    bool QtPropertyTableEditor::eventFilter(QObject* o, QEvent* e)
    {
        if (e->type() == QEvent::Paint) {
            if(QAbstractButton *btn = qobject_cast<QAbstractButton*>(o)) {
                // paint by hand (borrowed from QTableCornerButton)
                QStyleOptionHeader opt;

                opt.initFrom(btn);

                QStyle::State styleState = QStyle::State_None;
                if (btn->isEnabled())
                    styleState |= QStyle::State_Enabled;
                if (btn->isActiveWindow())
                    styleState |= QStyle::State_Active;
                if (btn->isDown())
                    styleState |= QStyle::State_Sunken;
                opt.state = styleState;
                opt.rect = btn->rect();
                opt.text = btn->text(); // this line is the only difference to QTableCornerButton
                //opt.icon = btn->icon(); // this line is the only difference to QTableCornerButton
                opt.position = QStyleOptionHeader::OnlyOneSection;
                QStylePainter painter(btn);
                painter.drawControl(QStyle::CE_Header, opt);
                return true; // eat event
            }
        }
        return false;
    }
    
} // QtPropertyEditor
