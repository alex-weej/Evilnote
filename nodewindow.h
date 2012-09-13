#pragma once

#include <QtCore>
#include <QtGui>

namespace Ui
{
class NodeWindow;
}

namespace En
{

class VstNode;

class NodeWindow : public QMainWindow
{
    Q_OBJECT

public:

    NodeWindow(VstNode* vstNode, QWidget *parent = 0);

    ~NodeWindow();

protected:

    void keyPressEvent(QKeyEvent* event);

    void keyReleaseEvent(QKeyEvent* event);

private slots:

    void on_testNoteButton_pressed();

    void on_testNoteButton_released();

    void on_showEditorButton_toggled(bool checked);

private:

    Ui::NodeWindow *ui;
    VstNode* m_vstNode; // not owned
    QWidget* m_vstEditorWidget;
    int m_note;
    QVector<int> m_funkyTown;
    unsigned m_funkyTownPos;
    int m_lastNote;
};

}
