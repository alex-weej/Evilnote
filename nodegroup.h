#pragma once

#include "node.h"

namespace En
{

class NodeGroup : public QObject
{
    Q_OBJECT

    // maybe change this to a QSet? can it be ordered?
    QVector<Node*> m_nodes;

public:

    virtual ~NodeGroup();

    void addNode(Node* node);

    void removeNode(Node* node);

    void deleteNode(Node* node);

    unsigned numChildNodes() const
    {
        return m_nodes.size();
    }

    Node* childNode(unsigned index) const
    {
        return m_nodes[index];
    }

    const QVector<Node*>& childNodes() const
    {
        return m_nodes;
    }

    /// Very slow way to get the node dependencies. TODO: cache this somehow? Or maintain node connections separately...
    QList<Node*> dependentNodes(Node* node) const;

    // is this necessary!?
    void emitNodeInputsChanged(Node* node)
    {
        emit nodeInputsChanged(node);
    }

signals:

    void nodeAdded(Node* node);
    void nodePreRemoved(Node* node);
    void nodeRemoved(Node* node);
    void nodePreDeleted(Node* node); // should probably move this to something more core? don't want to have to monitor every group for this
    void nodeInputsChanged(Node* node);

};

} // namespace En
