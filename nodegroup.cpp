#include "nodegroup.h"
#include "node.h"

namespace En
{

NodeGroup::NodeGroup() :
    m_host(0)
{

}

NodeGroup::~NodeGroup()
{
    Q_FOREACH (Node* node, m_nodes) {
        delete node;
    }
}

void NodeGroup::addNode(Node *node)
{
    node->setNodeGroup(this);
    m_nodes.push_back(node);
    emit nodeAdded(node);
}

void NodeGroup::removeNode(Node *node)
{
    node->disconnectNode();
    emit nodePreRemoved(node);
    m_nodes.remove(m_nodes.indexOf(node));
    node->setNodeGroup(0);
    emit nodeRemoved(node);
}

void NodeGroup::deleteNode(Node *node)
{
    removeNode(node);
    emit nodePreDeleted(node);
    delete node;
}

QList<Node*> NodeGroup::dependentNodes(Node *node) const
{
    QList<Node*> deps;

    Q_FOREACH (Node* testNode, m_nodes) {
        Q_FOREACH (Node* input, testNode->inputs()) {
            if (input == node) {
                deps << testNode;
            }
        }
    }

    return deps;
}

}
