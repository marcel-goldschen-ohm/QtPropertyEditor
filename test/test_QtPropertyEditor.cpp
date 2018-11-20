/* --------------------------------------------------------------------------------
 * Example tests for QtObjectPropertyEditor.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "test_QtPropertyEditor.h"

#include <QApplication>

#include "QtPropertyEditor.h"

int testQtPropertyTreeEditor(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // Object.
    TestObject object("My Obj");
    
    // Dynamic properties.
    object.setProperty("myDynamicBool", false);
    object.setProperty("myDynamicInt", 3);
    object.setProperty("myDynamicDouble", 3.0);
    object.setProperty("myDynamicString", "3 amigos");
    object.setProperty("myDynamicDateTime", QDateTime::currentDateTime());
    
    // UI.
    QtPropertyEditor::QtPropertyTreeEditor editor;
    editor.treeModel.propertyNames = QtPropertyEditor::getPropertyNames(&object);
    editor.treeModel.addProperty("child.myInt");
    editor.treeModel.propertyHeaders["objectName"] = "Name";
    editor.treeModel.setObject(&object);
    editor.show();
    editor.resizeColumnsToContents();
    
    int status = app.exec();
    
    object.dumpObjectInfo();
    
    return status;
}

QObject* newTestObject(QObject *parent) { return new TestObject("", parent); }

int testQtPropertyTableEditor(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    // Objects.
    QObject parent;
    QObjectList objects;
    for(int i = 0; i < 5; ++i) {
        QObject *object = new TestObject("My Obj " + QString::number(i), &parent);
        // Dynamic properties.
        object->setProperty("myDynamicBool", false);
        object->setProperty("myDynamicInt", 3);
        object->setProperty("myDynamicDouble", 3.0);
        object->setProperty("myDynamicString", "3 amigos");
        object->setProperty("myDynamicDateTime", QDateTime::currentDateTime());
        objects.append(object);
    }
    
    // UI.
    QtPropertyEditor::QtPropertyTableEditor editor;
    editor.tableModel.propertyNames = QtPropertyEditor::getMetaPropertyNames(TestObject::staticMetaObject);
    editor.tableModel.addProperty("child.myInt");
    editor.tableModel.propertyHeaders["objectName"] = "Name";
    editor.tableModel.setChildObjects<TestObject>(&parent);
    editor.show();
    editor.resizeColumnsToContents();
    
    int status = app.exec();
    
    // Check child object order.
    foreach(QObject *object, parent.findChildren<QObject*>(QString(), Qt::FindDirectChildrenOnly)) {
        qDebug() << object->objectName();
    }
    
    return status;
}

int main(int argc, char **argv)
{
    testQtPropertyTreeEditor(argc, argv);
    testQtPropertyTableEditor(argc, argv);
    return 0;
}
