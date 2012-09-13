#include "nodecontextmenuhelper.h"

#include "en.h"

namespace En
{

void NodeContextMenuHelper::deleteNode()
{
    m_node->nodeGroup()->deleteNode(m_node);
}

void NodeContextMenuHelper::setNodeAsOutput()
{
    m_node->nodeGroup()->setOutputNode(m_node);
}

void VstNodeContextMenuHelper::setInput(QObject *inputNodeObject)
{
    //qDebug() << "setting input";
    Node* inputNode = qobject_cast<Node*>(inputNodeObject);
    // TODO: mutex
    vstNode()->setInput(inputNode);
}

void VstNodeContextMenuHelper::unsetInput()
{
    vstNode()->setInput(0);
}

} // namespace En
