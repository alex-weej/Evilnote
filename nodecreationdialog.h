#pragma once

#include <QtWidgets>

// for Node::Factory
#include "node.h"

namespace En
{

class NodeGroup;

class NodeCreationWidget : public QLineEdit
{
    Q_OBJECT

    NodeGroup* m_nodeGroup;
    typedef QMap<QString, Node::Factory*> FactoryMap;
    FactoryMap m_factories;
    QPointF m_pos;

public:

    NodeCreationWidget(NodeGroup* nodeGroup, QPointF pos, QWidget* parent = 0);

    ~NodeCreationWidget();

signals:

    void nodeCreated(Node*);

private slots:

    void createNode();

};

class NodeCreationDialog : public QDialog
{
    Q_OBJECT

    NodeGroup* m_nodeGroup;
    Node* m_createdNode;

public:

    NodeCreationDialog(NodeGroup* nodeGroup, QPointF pos, QWidget* parent = 0);

    Node* createdNode() const
    {
        return m_createdNode;
    }

private:

    void initCocoa();

private slots:

    void nodeCreated(Node* newNode);

};

}
