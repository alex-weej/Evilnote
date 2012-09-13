#pragma once

#include <QThread>

class Node;
#include "host.h"

namespace En
{

class HostThread : public QThread
{
    Q_OBJECT

    //QVector<VstNode*> m_vstNodes;
    Node* m_outputNode;
    Host* m_host;

public:

    HostThread(Node* outputNode)
        : m_outputNode(outputNode)
        , m_host(0)
    {}

    virtual ~HostThread() {
        m_host->stop();
    }

    void run() {
        m_host = new Host(m_outputNode);
        connect(m_host, SIGNAL(utilisation(float)), SIGNAL(utilisation(float)));
        QThread::exec();
    }

signals:

    void utilisation(float);

};


} // namespace En
