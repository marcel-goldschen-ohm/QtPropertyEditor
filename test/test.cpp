/* --------------------------------------------------------------------------------
 * Example tests for QtObjectPropertyEditor.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QtPropertyEditor.h"

int main(int argc, char **argv)
{
    QtPropertyEditor::testQtPropertyTreeEditor(argc, argv);
    QtPropertyEditor::testQtPropertyTableEditor(argc, argv);
    return 0;
}
