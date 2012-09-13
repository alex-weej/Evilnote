#include "host.h"

#include "node.h"
#include "vstnode.h"
#include "mixernode.h"

// bits for hi-res timing on Mac
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

namespace En
{

Host::Host(Node* outputNode)
    : m_ioDevice(0)
    , m_audioOutput(0)
    , m_timer(0)
    , m_globalSampleIndex(0)
    , m_outputNode(outputNode)
    , m_lastProfiledTime(0)
    , m_busyAccumulatedTime(0)
{
    init();
}

Host::~Host()
{
    delete m_audioOutput;
}

void Host::init()
{
    m_format.setFrequency(44100);
    m_format.setChannels(2);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    m_audioOutput = new QAudioOutput(m_format, this);

    const int bufferSizeSamples = 512;
    m_audioOutput->setBufferSize(bufferSizeSamples * m_format.channels() * m_format.sampleSize() / 8);

    connect(m_audioOutput, SIGNAL(notify()), SLOT(notified()));
    connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));

    m_ioDevice = m_audioOutput->start();

    qDebug() << m_audioOutput->error() << QAudio::NoError;

    qDebug() << "buffer size (bytes)" << m_audioOutput->bufferSize();
    qDebug() << "period size (bytes)" << m_audioOutput->periodSize();

    m_blockSizeSamples = m_audioOutput->periodSize() / m_format.channelCount() / (m_format.sampleSize() / 8);
    qDebug() << "block size (samples)" << m_blockSizeSamples;


    m_nullInputBuffer.resize(m_blockSizeSamples);
    Node::s_nullInputBuffer = nullInputBuffer();

    //for (int i = 0; i < 4; ++i)
    //    m_floatBlock[i].resize(m_blockSizeSamples);

    m_buffer.resize(m_audioOutput->periodSize());

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(0);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(writeData()));
    m_timer->start();

}

void Host::stop()
{
    m_timer->stop();
}

void Host::writeData()
{
    //QElapsedTimer elTimer;
    //elTimer.start();

    if (m_lastProfiledTime != 0) {
        uint64_t thisTime = mach_absolute_time();
        uint64_t elapsed = thisTime - m_lastProfiledTime;
        Nanoseconds nanoNano = AbsoluteToNanoseconds(*(AbsoluteTime*)&elapsed);
        uint64_t elapsedNano = *(uint64_t*)&nanoNano;
        // once every second...
        if (elapsedNano > 1e9) {
            nanoNano = AbsoluteToNanoseconds(*(AbsoluteTime*)&m_busyAccumulatedTime);
            uint64_t busyNano = *(uint64_t*)&nanoNano;
            float utilisationAmount = (double)busyNano / elapsedNano;
            emit utilisation(utilisationAmount);
            m_lastProfiledTime = thisTime;
            m_busyAccumulatedTime = 0;
        }
    } else {
        m_lastProfiledTime = mach_absolute_time();
    }



    if (!m_audioOutput || m_audioOutput->state() == QAudio::StoppedState) {
        return;
    }


    //qDebug() << "checking in writeData";

    //qDebug() << "bytesFree" << m_audioOutput->bytesFree();
    //qDebug() << "periodSize" << m_audioOutput->periodSize();

    int chunks = m_audioOutput->bytesFree() / m_audioOutput->periodSize();


    //qDebug() << "chunks" << chunks;
    if (chunks > 0) {

        uint64_t busyTimeStart = mach_absolute_time();

        for (int i = 0; i < chunks; ++i) {
            // generate a buffer and write it



            // Temporary ppqPos implementation. This should be ultimately be coming from the sequencer.
            s_vstTimeInfo.ppqPos = s_vstTimeInfo.samplePos / 44100 * s_vstTimeInfo.tempo / 60;
            //qDebug() << "sample pos" << s_vstTimeInfo.samplePos;
            //qDebug() << "ppq pos" << s_vstTimeInfo.ppqPos;


            Node* outputNode = m_outputNode;

            //qDebug() << "checkpoint before dep calc" << elTimer.restart();

            struct DependencyVisitor: public Node::Visitor {

                typedef QMap<Node*, QSet<Node*> > NodeRelationMap;

                // any need to have these separate? could be a QPair?
                NodeRelationMap dependencyMap;
                NodeRelationMap dependentMap;

                void visitRecurse(Node* node) {
                    //qDebug() << "VisitRecurse Node*" << node->displayLabel();
                    if (dependencyMap.contains(node)) {
                        return;
                    }
                    dependencyMap[node];
                    dependentMap[node];
                    node->accept(*this);
                }

                void visit(VstNode* vstNode) {
                    //qDebug() << "Visiting VstNode*" << vstNode->displayLabel();
                    Node* input = vstNode->input();
                    if (input) {
                        dependencyMap[static_cast<Node*>(vstNode)] << input;
                        dependentMap[input] << static_cast<Node*>(vstNode);
                        visitRecurse(input);
                    }
                }

                void visit(MixerNode* mixerNode) {
                    //qDebug() << "Visiting MixerNode*" << mixerNode->displayLabel();
                    for (unsigned i = 0; i < mixerNode->numInputs(); ++i) {
                        Node* input = mixerNode->input(i);
                        dependencyMap[static_cast<Node*>(mixerNode)] << input;
                        dependentMap[input] << static_cast<Node*>(mixerNode);
                        visitRecurse(input);
                    }
                }

            } dependencyVisitor;

            Q_FOREACH (Node* node, outputNode->nodeGroup()->childNodes()) {
                dependencyVisitor.visitRecurse(node);
            }


            //qDebug() << "checkpoint before executor" << elTimer.restart();

            //outputNode->accept(dependencyVisitor);
            // TODO: fix the above so it only happens when the graph changes. no need to recalculate dependencies on every single buffer generation

            struct SerialNodeExecutor: public Node::Visitor {

                DependencyVisitor& dependencyVisitor;
                QSet<Node*> visited;

                SerialNodeExecutor(DependencyVisitor& d) : dependencyVisitor(d) {}

                void execute() {

                    //qDebug() << "------------------------";

                    //Q_FOREACH (DependencyVisitor::NodeRelationMap::const_iterator it, dependencyVisitor.dependencyMap) {

                    QQueue<Node*> nodeQueue;

                    DependencyVisitor::NodeRelationMap::const_iterator it = dependencyVisitor.dependencyMap.constBegin();
                    while (it != dependencyVisitor.dependencyMap.constEnd()) {
                        Node* node = it.key();
                        //qDebug() << "Dependencies of" << node->displayLabel();
                        //bool hasDeps = false;
                        //Q_FOREACH (Node* depNode, it.value()) {
                        //qDebug() << " -" << depNode->displayLabel();
                        //hasDeps = true;
                        //}
                        bool hasDeps = !it.value().isEmpty();
                        if (!hasDeps) {
                            nodeQueue.append(node);
                        }
                        ++it;
                    }

                    //qDebug() << "------------------------";

                    Q_FOREACH (Node* node, nodeQueue) {
                        dispatch(node);
                    }
                }

                void dispatch(Node* node) {
                    if (visited.contains(node)) {
                        return;
                    }
                    visited << node;
                    node->accept(*this);
                }

                virtual void visit(Node* node)
                {
                    node->processAudio();

                    Q_FOREACH (Node* dependentNode, dependencyVisitor.dependentMap[node]) {
                        dispatch(dependentNode);
                    }
                }

            } executor(dependencyVisitor);

            executor.execute();

            //qDebug() << "checkpoint before buffer write" << elTimer.restart();

            // FIXME: what if output node does not have 2 outputs? deal with it.

            // convert float data buffers to our audio format

            qint16* ptr = (qint16*)m_buffer.data();

            for (unsigned sampleIndex = 0; sampleIndex < m_blockSizeSamples; sampleIndex++) {
                for (int channel = 0; channel < m_format.channelCount(); ++channel) {
                    float clamped = std::min(std::max(outputNode->outputChannelBufferConst(channel)[sampleIndex], -1.f), 1.f);
                    *ptr++ = qint16(clamped * 32767);
                }
            }


            //qDebug("writing buffer");
            qint64 written = m_ioDevice->write(m_buffer.data(), m_buffer.size());


            //qDebug() << "checkpoint finish" << elTimer.restart();

            //m_ioDevice->waitForBytesWritten(-1);
            //qDebug("%lld bytes written!", written);

            //qDebug() << "error state" << m_audioOutput->error() << QAudio::NoError;

            // update vst time info struct
            s_vstTimeInfo.samplePos += m_blockSizeSamples;
        }

        //quint64 elapsedBusy = m_profileTimer.restart();
        //qDebug() << "Host was busy for" << elapsedBusy << "ms";
        m_busyAccumulatedTime += (mach_absolute_time() - busyTimeStart);

    } else {
        // do nothing.
    }


    //qDebug() << "bytesFree = " << m_audioOutput->bytesFree();


    m_timer->start();
}

void Host::stateChanged(QAudio::State state)
{
    qDebug() << "State changed" << state;
}

} // namespace En;
