#pragma once

#include <QThread>
#include "host.h"

namespace En
{

class HostThread : public QThread
{
    Q_OBJECT

    //QVector<VstNode*> m_vstNodes;
    NodeGroup* m_rootGroup;
    Host* m_host;

public:

    HostThread(NodeGroup* rootGroup)
        : m_rootGroup(rootGroup)
        , m_host(0)
    {}

    virtual ~HostThread() {
        m_host->stop();
    }

    void run() {
        m_host = new Host(m_rootGroup);
        connect(m_host, SIGNAL(utilisation(float)), SIGNAL(utilisation(float)));
        QThread::exec();
    }

signals:

    void utilisation(float);

};


} // namespace En
