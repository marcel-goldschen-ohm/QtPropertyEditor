/* --------------------------------------------------------------------------------
 * Example tests for QtObjectPropertyEditor.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QtPropertyEditorTest.h"

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
    
    // Model.
    QtPropertyEditor::QtPropertyTreeModel model;
    model.setObject(&object);
    
    // Property names.
    QList<QByteArray> propertyNames = QtPropertyEditor::getObjectPropertyNames(&object);
    propertyNames.append("child.myInt");
    model.setPropertyNames(propertyNames);
    
    // Property headers.
    QHash<QByteArray, QString> propertyHeaders;
    propertyHeaders["objectName"] = "Name";
    model.setPropertyHeaders(propertyHeaders);
    
    // Editor UI.
    QtPropertyEditor::QtPropertyTreeEditor editor;
    editor.setModel(&model);
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
    
    // Model.
    QtPropertyEditor::QtPropertyTableModel model;
    model.setObjects(objects);
    model.setObjectCreator(std::bind(newTestObject, &parent));
    
    // Property names.
    QList<QByteArray> propertyNames = QtPropertyEditor::getMetaObjectPropertyNames(TestObject::staticMetaObject);
    propertyNames.append("child.myInt");
    model.setPropertyNames(propertyNames);
    
    // Property headers.
    QHash<QByteArray, QString> propertyHeaders;
    propertyHeaders["objectName"] = "Name";
    model.setPropertyHeaders(propertyHeaders);
    
    // Editor UI.
    QtPropertyEditor::QtPropertyTableEditor editor;
    editor.setModel(&model);
    editor.show();
    editor.resizeColumnsToContents();
    
    int status = app.exec();
    
    // Check child object order.
    foreach(QObject *object, parent.findChildren<QObject*>(QString(), Qt::FindDirectChildrenOnly))
    qDebug() << object->objectName();
    
    return status;
}

int main(int argc, char **argv)
{
    testQtPropertyTreeEditor(argc, argv);
    testQtPropertyTableEditor(argc, argv);
    return 0;
}
