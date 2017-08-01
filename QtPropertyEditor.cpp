/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QtPropertyEditor.h"

#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMetaObject>
#include <QMetaType>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QScrollBar>

namespace QtPropertyEditor
{
    QList<QByteArray> getPropertyNames(QObject *object)
    {
        QList<QByteArray> propertyNames = getMetaPropertyNames(*object->metaObject());
        foreach(const QByteArray &dynamicPropertyName, object->dynamicPropertyNames())
            propertyNames << dynamicPropertyName;
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
    
    QList<QByteArray> getDerivedPropertyNames(QObject *object)
    {
        QList<QByteArray> propertyNames = getPropertyNames(object);
        QList<QByteArray> superPropertyNames = getMetaPropertyNames(*object->metaObject()->superClass());
        foreach(const QByteArray &superPropertyName, superPropertyNames)
            propertyNames.removeOne(superPropertyName);
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
                return true;
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
            return 0;
        }
    }
    
    QObject* QtPropertyTreeModel::objectAtIndex(const QModelIndex &index) const
    {
        // If node is an object, return the node's object.
        // Else if node is a property, return the parent node's object.
        Node *node = nodeAtIndex(index);
        if(!node) return 0;
        if(node->object) return node->object;
        if(node->parent) return node->parent->object;
        return 0;
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
                else if(_propertyHeaders.contains(propertyName))
                    return QVariant(_propertyHeaders[propertyName]);
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
                    return true;
                } else {
                    bool result = object->setProperty(propertyName.constData(), value);
                    // Result will be FALSE for dynamic properties, which causes the tree view to lag.
                    // So make sure we still return TRUE in this case.
                    if(!result && object->dynamicPropertyNames().contains(propertyName))
                        return true;
                    return result;
                }
            }
        }
        return false;
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
        if(!_propertyNames.isEmpty()) {
            if(_propertyNames.size() > index.column()) {
                QByteArray propertyName = _propertyNames.at(index.column());
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
        if(!_propertyNames.isEmpty()) {
            if(_propertyNames.size() > index.column()) {
                QByteArray propertyName = _propertyNames.at(index.column());
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
        if(!_propertyNames.isEmpty())
            return _propertyNames.size();
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
                if(_propertyNames.size() > section) {
                    QByteArray pathToPropertyName = _propertyNames.at(section);
                    if(pathToPropertyName.contains('.')) {
                        int pos = pathToPropertyName.lastIndexOf('.');
                        childPath = pathToPropertyName.left(pos + 1);
                    }
                }
                if(_propertyHeaders.contains(propertyName))
                    return QVariant(childPath + _propertyHeaders.value(propertyName));
                return QVariant(childPath + propertyName);
            }
        }
        return QVariant();
    }
    
    bool QtPropertyTableModel::insertRows(int row, int count, const QModelIndex &parent)
    {
        // Only valid if we have an object creator method.
        if(!_objectCreator)
            return false;
        bool columnCountWillAlsoChange = _objects.isEmpty() && _propertyNames.isEmpty();
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
                    object->setParent(0);
                    object->setParent(parent);
                }
            }
        }
    }

    QWidget* QtPropertyDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QVariant value = index.data(Qt::DisplayRole);
        if(value.isValid()) {
            if(value.type() == QVariant::Bool) {
                // We want a check box, but instead of creating an editor widget we'll just directly
                // draw the check box in paint() and handle mouse clicks in editorEvent().
                // Here, we'll just return 0 to make sure that no editor is created when this cell is double clicked.
                return 0;
            } else if(value.type() == QVariant::Double) {
                // Return a QLineEdit to enter double values with arbitrary precision and scientific notation.
                QLineEdit *editor = new QLineEdit(parent);
                editor->setText(value.toString());
                return editor;
            } else if(value.type() == QVariant::Int) {
                // We don't need to do anything special for an integer, we'll just use the default QSpinBox.
                // However, we do need to check if it is an enum. If so, we'll use a QComboBox editor.
                const QtAbstractPropertyModel *propertyModel = qobject_cast<const QtAbstractPropertyModel*>(index.model());
                if(propertyModel) {
                    const QMetaProperty metaProperty = propertyModel->metaPropertyAtIndex(index);
                    if(metaProperty.isValid() && metaProperty.isEnumType()) {
                        const QMetaEnum metaEnum = metaProperty.enumerator();
                        int numKeys = metaEnum.keyCount();
                        if(numKeys > 0) {
                            QComboBox *editor = new QComboBox(parent);
                            for(int j = 0; j < numKeys; ++j) {
                                QByteArray key = QByteArray(metaEnum.key(j));
                                editor->addItem(QString(key));
                            }
                            QByteArray currentKey = QByteArray(metaEnum.valueToKey(value.toInt()));
                            editor->setCurrentText(QString(currentKey));
                            return editor;
                        }
                    }
                }
            } else if(value.type() == QVariant::Size || value.type() == QVariant::SizeF ||
                      value.type() == QVariant::Point || value.type() == QVariant::PointF ||
                      value.type() == QVariant::Rect || value.type() == QVariant::RectF) {
                // Return a QLineEdit. Parsing will be done in displayText() and setEditorData().
                QLineEdit *editor = new QLineEdit(parent);
                editor->setText(displayText(value, QLocale()));
                return editor;
            }
        }
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    
    void QtPropertyDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
    
    void QtPropertyDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        QVariant value = index.data(Qt::DisplayRole);
        if(value.isValid()) {
            if(value.type() == QVariant::Double) {
                // Set model's double value data to numeric representation in QLineEdit editor.
                // Conversion from text to number handled by QVariant.
                QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
                if(lineEditor) {
                    QVariant value = QVariant(lineEditor->text());
                    bool ok;
                    double dval = value.toDouble(&ok);
                    if(ok)
                        model->setData(index, QVariant(dval), Qt::EditRole);
                    return;
                }
            } else if(value.type() == QVariant::Int) {
                // We don't need to do anything special for an integer.
                // However, if it's an enum we'll set the data based on the QComboBox editor.
                QComboBox *comboBoxEditor = qobject_cast<QComboBox*>(editor);
                if(comboBoxEditor) {
                    QString selectedKey = comboBoxEditor->currentText();
                    const QtAbstractPropertyModel *propertyModel = qobject_cast<const QtAbstractPropertyModel*>(model);
                    if(propertyModel) {
                        const QMetaProperty metaProperty = propertyModel->metaPropertyAtIndex(index);
                        if(metaProperty.isValid() && metaProperty.isEnumType()) {
                            const QMetaEnum metaEnum = metaProperty.enumerator();
                            bool ok;
                            int selectedValue = metaEnum.keyToValue(selectedKey.toLatin1().constData(), &ok);
                            if(ok)
                                model->setData(index, QVariant(selectedValue), Qt::EditRole);
                            return;
                        }
                    }
                    // If we got here, we have a QComboBox editor but the property at index is not an enum.
                }
            } else if(value.type() == QVariant::Size) {
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
                }
            } else if(value.type() == QVariant::SizeF) {
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
                }
            } else if(value.type() == QVariant::Point) {
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
                }
            } else if(value.type() == QVariant::PointF) {
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
                }
            } else if(value.type() == QVariant::Rect) {
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
                }
            } else if(value.type() == QVariant::RectF) {
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
            if(value.type() == QVariant::Size) {
                // w x h
                QSize size = value.toSize();
                return QString::number(size.width()) + QString(" x ") + QString::number(size.height());
            } else if(value.type() == QVariant::SizeF) {
                // w x h
                QSizeF size = value.toSizeF();
                return QString::number(size.width()) + QString(" x ") + QString::number(size.height());
            } else if(value.type() == QVariant::Point) {
                // (x, y)
                QPoint point = value.toPoint();
                return QString("(")
                + QString::number(point.x()) + QString(", ") + QString::number(point.y())
                + QString(")");
            } else if(value.type() == QVariant::PointF) {
                // (x, y)
                QPointF point = value.toPointF();
                return QString("(")
                + QString::number(point.x()) + QString(", ") + QString::number(point.y())
                + QString(")");
            } else if(value.type() == QVariant::Rect) {
                // [(x, y), w x h]
                QRect rect = value.toRect();
                return QString("[(")
                + QString::number(rect.x()) + QString(", ") + QString::number(rect.y())
                + QString("), ")
                + QString::number(rect.width()) + QString(" x ") + QString::number(rect.height())
                + QString("]");
            } else if(value.type() == QVariant::RectF) {
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
            if(value.type() == QVariant::Bool) {
                bool checked = value.toBool();
                QStyleOptionButton buttonOption;
                buttonOption.state |= QStyle::State_Active; // Required!
                buttonOption.state |= ((index.flags() & Qt::ItemIsEditable) ? QStyle::State_Enabled : QStyle::State_ReadOnly);
                buttonOption.state |= (checked ? QStyle::State_On : QStyle::State_Off);
                QRect checkBoxRect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &buttonOption); // Only used to get size of native checkbox widget.
                buttonOption.rect = QStyle::alignedRect(option.direction, Qt::AlignLeft, checkBoxRect.size(), option.rect); // Our checkbox rect.
                QApplication::style()->drawControl(QStyle::CE_CheckBox, &buttonOption, painter);
                return;
            } else if(value.type() == QVariant::Int) {
                // We don't need to do anything special for an integer.
                // However, if it's an enum want to render the key name instead of the value.
                // This cannot be done in displayText() because we need the model index to get the key name.
                const QtAbstractPropertyModel *propertyModel = qobject_cast<const QtAbstractPropertyModel*>(index.model());
                if(propertyModel) {
                    const QMetaProperty metaProperty = propertyModel->metaPropertyAtIndex(index);
                    if(metaProperty.isValid() && metaProperty.isEnumType()) {
                        const QMetaEnum metaEnum = metaProperty.enumerator();
                        QByteArray currentKey = QByteArray(metaEnum.valueToKey(value.toInt()));
                        QStyleOptionViewItem itemOption(option);
                        initStyleOption(&itemOption, index);
                        itemOption.text = QString(currentKey);
                        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter);
                        return;
                    }
                }
            }
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
    
    bool QtPropertyDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
    {
        QVariant value = index.data(Qt::DisplayRole);
        if(value.isValid()) {
            if(value.type() == QVariant::Bool) {
                if(event->type() == QEvent::MouseButtonDblClick)
                    return false;
                if(event->type() != QEvent::MouseButtonRelease)
                    return false;
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if(mouseEvent->button() != Qt::LeftButton)
                    return false;
                //QStyleOptionButton buttonOption;
                //QRect checkBoxRect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &buttonOption); // Only used to get size of native checkbox widget.
                //buttonOption.rect = QStyle::alignedRect(option.direction, Qt::AlignLeft, checkBoxRect.size(), option.rect); // Our checkbox rect.
                // option.rect ==> cell
                // buttonOption.rect ==> check box
                // Here, we choose to allow clicks anywhere in the cell to toggle the checkbox.
                if(!option.rect.contains(mouseEvent->pos()))
                    return false;
                bool checked = value.toBool();
                QVariant newValue(!checked); // Toggle model's bool value.
                bool success = model->setData(index, newValue, Qt::EditRole);
                // Update entire table row just in case some other cell also refers to the same bool value.
                // Otherwise, that other cell will not reflect the current state of the bool set via this cell.
                if(success)
                    model->dataChanged(index.sibling(index.row(), 0), index.sibling(index.row(), model->columnCount()));
                return success;
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
    
//    QtObjectPropertyEditor::QtObjectPropertyEditor(QWidget *parent) : QTableView(parent)
//    {
//        setItemDelegate(&_delegate);
//        setAlternatingRowColors(true);
//        verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
//    }
    
    QtPropertyTreeEditor::QtPropertyTreeEditor(QWidget *parent) : QTreeView(parent)
    {
        setItemDelegate(&_delegate);
        setAlternatingRowColors(true);
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
        verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        
        // Draggable rows.
        verticalHeader()->setSectionsMovable(true);
        connect(verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(handleSectionMove(int, int, int)));
        
        // Header context menus.
        horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(horizontalHeaderContextMenu(QPoint)));
        connect(verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(verticalHeaderContextMenu(QPoint)));
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
        menu->addAction("Append Row", this, SLOT(appendRow()));
        if(indexes.size()) {
            menu->addSeparator();
            menu->addAction("Insert Rows", this, SLOT(insertSelectedRows()));
            menu->addSeparator();
            menu->addAction("Delete Rows", this, SLOT(removeSelectedRows()));
        }
        menu->popup(verticalHeader()->viewport()->mapToGlobal(pos));
    }
    
    void QtPropertyTableEditor::appendRow()
    {
        model()->insertRows(model()->rowCount(), 1);
    }
    
    void QtPropertyTableEditor::insertSelectedRows()
    {
        QModelIndexList indexes = selectionModel()->selectedRows();
        if(indexes.size() == 0)
            return;
        QList<int> rows;
        foreach(const QModelIndex &index, indexes)
            rows.append(index.row());
        qSort(rows);
        model()->insertRows(rows.at(0), rows.size());
    }
    
    void QtPropertyTableEditor::removeSelectedRows()
    {
        QModelIndexList indexes = selectionModel()->selectedRows();
        if(indexes.size() == 0)
            return;
        QList<int> rows;
        foreach(const QModelIndex &index, indexes)
            rows.append(index.row());
        qSort(rows);
        for(int i = rows.size() - 1; i >= 0; --i)
            model()->removeRows(rows.at(i), 1);
    }
    
    void QtPropertyTableEditor::handleSectionMove(int /* logicalIndex */, int oldVisualIndex, int newVisualIndex)
    {
        if(QtPropertyTableModel *propertyModel = qobject_cast<QtPropertyTableModel*>(model())) {
            // Move objects in the model, and then move the sections back to maintain logicalIndex order.
            propertyModel->moveRows(QModelIndex(), oldVisualIndex, 1, QModelIndex(), newVisualIndex);
            disconnect(verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(handleSectionMove(int, int, int)));
            verticalHeader()->moveSection(newVisualIndex, oldVisualIndex);
            connect(verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(handleSectionMove(int, int, int)));
        }
    }
    
    void QtPropertyTableEditor::keyPressEvent(QKeyEvent *event)
    {
        if(event->key() == Qt::Key_Plus)
            appendRow();
    }
    
} // QtPropertyEditor
