#pragma once

#include "vstnode.h"
#include "mixernode.h"
#include "node.h"
#include <QObject>

namespace En
{

class NodeContextMenuHelper : public QObject
{
    Q_OBJECT

public:

    NodeContextMenuHelper(Node* node) : m_node(node) {}

    virtual ~NodeContextMenuHelper() {}

public slots:

    void deleteNode();

protected:

    Node* m_node;

};

class VstNodeContextMenuHelper : public NodeContextMenuHelper
{
    Q_OBJECT

public:

    VstNodeContextMenuHelper(Node* node) : NodeContextMenuHelper(node) {}

public slots:

    void setInput(QObject* inputNodeObject);

    void unsetInput();

private:

    VstNode* vstNode() const
    {
        return static_cast<VstNode*>(m_node);
    }

};

class MixerNodeContextMenuHelper : public NodeContextMenuHelper
{
    Q_OBJECT

public:

    MixerNodeContextMenuHelper(Node* node) : NodeContextMenuHelper(node) {}

public slots:

    void addInput(QObject* inputNodeObject)
    {
        //qDebug() << "adding input";
        Node* inputNode = qobject_cast<Node*>(inputNodeObject);
        // TODO: mutex
        mixerNode()->addInput(inputNode);
    }

    void removeInput(QObject* inputNodeObject)
    {
        //qDebug() << "removing input";
        Node* inputNode = qobject_cast<Node*>(inputNodeObject);
        // TODO: mutex
        mixerNode()->removeInput(inputNode);
    }

private:

    MixerNode* mixerNode() const
    {
        return static_cast<MixerNode*>(m_node);
    }
};


} // namespace En
