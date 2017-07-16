/* --------------------------------------------------------------------------------
 * Example tests for QtObjectPropertyEditor.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QtObjectPropertyEditor.h"

int main(int argc, char **argv)
{
    QtObjectPropertyEditor::testQtObjectPropertyEditor(argc, argv);
    QtObjectPropertyEditor::testQtObjectListPropertyEditor(argc, argv);
    QtObjectPropertyEditor::testQtObjectTreePropertyEditor(argc, argv);
    return 0;
}
