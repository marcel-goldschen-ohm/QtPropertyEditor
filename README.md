# QtObjectPropertyEditor

UI property editors for a QObject or a QObjectList.

* Editor for a single QObject is a column list of properties.
    * <img src="images/QtObjectPropertyEditor.png" width="200" />
* Editor for a list of QObjects is a table where rows are objects and columns are properties.
    * <img src="images/QtObjectListPropertyEditor.png" width="600" />
    * Rows (objects) can be rearranged by dragging the row header with the mouse.
    * Allows dynamic insertion/deletion of objects (rows) via a context menu obtainable by right clicking on the row headers (similar to Excel).
* Default delegates for editing common value types (these are in addition to the default delegates already in Qt):
    * bool: QCheckBox
    * QEnum: QComboBox
    * double: QLineEdit that can handle scientific notation
    * QSize/QSizeF: QLineEdit for text format *(w x h)*
    * QPoint/QPointF: QLineEdit for text format *(x, y)*
    * QRect/QRectF: QLineEdit for text format *[(x, y) w x h]*
* Default is a flat editor for an object's properties excluding properties of child objects.
    * Specific child object properties can be made available in the editor via a *"path.to.child.property"* string. In this case, *path*, *to* and *child* are the object names of the child object tree, and *property* is a property name for *child*.

**Author**: Marcel Goldschen-Ohm  
**Email**:  <marcel.goldschen@gmail.com>  
**License**: MIT  
Copyright (c) 2017 Marcel Goldschen-Ohm 

## INSTALL

Everything is in:

* `QtObjectPropertyEditor.h`
* `QtObjectPropertyEditor.cpp`

### Requires:

* [Qt](http://www.qt.io)

## Edit properties of a single QObject.

See `QtObjectPropertyEditor::testQtObjectPropertyEditor` for an example. Note, this is part of `test/test.cpp`.

For this basic example, we need a QApplication, same as always.

```cpp
QApplication app(...);
```

Any object derived from QObject whose properties will be exposed in the editor.

```cpp
QtObjectPropertyEditor::TestObject object;
```

**[Note]** Properties of child object's are by default NOT exposed in the editor unless we specify them directly (see below).

```cpp
QtObjectPropertyEditor::TestObject *child = 
    new QtObjectPropertyEditor::TestObject("MyChild");
child->setParent(&object);
```

The model interface to our object's properties.

```cpp
QtObjectPropertyEditor::QtObjectPropertyModel model;
model.setObject(&object);
```

**[Optional]** You can define which properties to expose in the editor (default includes all properties including dynamic properties). For example, if we only wanted to show the "objectName" and "myInt" properties of our object as well as the "myDouble" property of the child object named "MyChild":
    
```cpp
QList<QByteArray> propertyNames;
propertyNames << "objectName" << "myInt" << "MyChild.myDouble";
model.setPropertyNames(propertyNames);
```

**[Optional]** You can define the property column headers that will be displayed (otherwise they default to the property names). For example, if we wanted to show "Name" in the column header instead of the default "objectName":

```cpp
QHash<QByteArray, QString> propertyHeaders;
propertyHeaders["objectName"] = "Name";
model.setPropertyHeaders(propertyHeaders);
```

The table view UI editor linked to our object's model interface.

```cpp
QtObjectPropertyEditor::QtObjectPropertyEditor editor;
editor.setModel(&model);
```

Show the editor and run the application.

```cpp
editor.show();
app.exec();
```

## Edit properties for each object in a QObjectList.

See `QtObjectPropertyEditor::testQtObjectListPropertyEditor` for an example. Note, this is part of `test/test.cpp`.

For this basic example, we need a QApplication, same as always.

```cpp
QApplication app(...);
```

A list of objects derived from QObject whose properties will be exposed in the editor. Although it is NOT required, for this example we'll make the objects in our list children of a single parent object. :warning: **All of this only makes sense if all of the objects to be exposed in the editor have the same properties (i.e. they are all the same type of object).** 

```cpp
QObject parent;
for(int i = 0; i < 5; ++i) {
    QObject *object = new QtObjectPropertyEditor::TestObject(
        "My Obj " + QString::number(i));
    object->setParent(&parent);
}
QObjectList objects = parent->children();
```

The model interface to the properties in our list of objects.

```cpp
QtObjectPropertyEditor::QtObjectListPropertyModel model;
model.setObjects(objects);
```

**[Optional]** For dynamic object insertion in the list, you need to supply an object creator function of type QtObjectListPropertyModel::ObjectCreatorFunction. For convenience, QtObjectListPropertyModel defines defaultObjectCreator() templated on the derived type, but you are free to use your own creator function as well. Furthermore, if you want the newly created objects to be children of a particular parent object, you need to tell the model about the parent.
    
```cpp
model.setObjectCreator(QtObjectListPropertyModel::defaultObjectCreator<TestObject>);
model.setParentOfObjects(&parent);
```

**[Optional]** Exposed properties and their column headers can be specified exactly the same as shown in the example above for the editor of a single QObject.

The table view UI editor linked to the model interface for our list of objects.

```cpp
QtObjectPropertyEditor::QtObjectListPropertyEditor editor;
editor.setModel(&model);
```

Show the editor and run the application.

```cpp
editor.show();
app.exec();
```
