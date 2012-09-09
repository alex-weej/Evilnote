#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMultimedia>
#include <QAudioOutput>
#include <QMacCocoaViewContainer>
#include <QGLWidget>

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

#include "pluginterfaces/vst2.x/aeffectx.h"
#include <algorithm>

// bits for high res timing on Mach
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>


namespace Ui {
class MainWindow;
}




namespace En {

extern VstTimeInfo s_vstTimeInfo;

typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);


inline VstIntPtr hostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{

    //qDebug() << "hostCallback in thread" << QThread::currentThreadId();

    VstIntPtr result = 0;

    // Filter idle calls...
    bool filtered = false;
    if (opcode == audioMasterIdle)
    {
        static bool wasIdle = false;
        if (wasIdle)
            filtered = true;
        else
        {
            //printf ("(Future idle calls will not be displayed!)\n");
            wasIdle = true;
        }
    }

    //qDebug("audioMasterGetCurrentProcessLevel is %d", audioMasterGetCurrentProcessLevel);

    if (!filtered)
        //qDebug("PLUG> HostCallback (opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f", opcode, index, FromVstPtr<void> (value), ptr, opt);

        switch (opcode)
        {
        case audioMasterVersion:
            result = kVstVersion;
            break;

        case DECLARE_VST_DEPRECATED (audioMasterPinConnected):
            //qDebug() << "pin connected";
            break;

        case DECLARE_VST_DEPRECATED (audioMasterWantMidi):
            break;

        case audioMasterGetTime:
            result = (VstIntPtr)&s_vstTimeInfo;
            break;

        case audioMasterGetCurrentProcessLevel:
            result = kVstProcessLevelUser;
            break;

        case audioMasterCanDo:
        {
            QString canDoQuery((char*)ptr);

            QMap<QString, int> canDos;
            canDos["sendVstEvents"] = 1;
            canDos["sendVstMidiEvent"] = 1;
            canDos["sendVstTimeInfo"] = 1;
            canDos["sizeWindow"] = 1;

            QMap<QString, int>::iterator it = canDos.find(canDoQuery);
            if (it == canDos.end()) {
                result = 0; // don't know
                qDebug() << "unknown audioMasterCanDo:" << canDoQuery;
            } else {
                result = *it;
            }

            break;
        }

        case audioMasterSizeWindow:
        {
            // TODO: handle this!
            int width = index;
            int height = value;
            qDebug() << "SIZE WINDOW" << width << height;
            break;
        }

        default:
            //qDebug() << "UNHANDLED HOST OPCODE" << opcode;
            //Q_ASSERT(false);
            break;

        }

    return result;
}


class VstModule: public QObject {
    Q_OBJECT

private:
    CFBundleRef m_bundle;
    PluginEntryProc m_mainProc;

public:
    VstModule(const QString& fileName) : m_bundle(0), m_mainProc(0) {
        load(fileName);
    }

    virtual ~VstModule() {
        unload();
    }

    AEffect* createVstInstance() {
        if (!m_mainProc) {
            qDebug("createVstInstance failure: no main proc");
            return 0;
        }
        AEffect* effect = m_mainProc(hostCallback);
        if (!effect) {
            qDebug("createVstInstance failure: couldn't create vst instance");
            return 0;
        }
        return effect;
    }

private:
    bool load(const QString& fileName) {

        CFStringRef fileNameString = CFStringCreateWithCString(0, fileName.toUtf8(), kCFStringEncodingUTF8);
        if (fileNameString == 0) {
            qDebug("bad filename");
            return false;
        }
        CFURLRef url = CFURLCreateWithFileSystemPath (0, fileNameString, kCFURLPOSIXPathStyle, false);
        CFRelease(fileNameString);
        if (url == 0) {
            qDebug("bad url");
            return false;
        }
        m_bundle = CFBundleCreate(0, url);
        CFRelease(url);
        if (!m_bundle) {
            qDebug("couldn't create bundle");
            return false;
        }

        if (CFBundleLoadExecutable(m_bundle) == false) {
            qDebug("couldn't load executable for bundle");
            unload();
            return false;
        }

        m_mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName(m_bundle, CFSTR("VSTPluginMain"));
        if (!m_mainProc) {
            m_mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName(m_bundle, CFSTR("main_macho"));
        }
        if (!m_mainProc) {
            qDebug("couldn't get main proc");
            unload();
            return false;
        }

        qDebug("vst mainproc is: %p", m_mainProc);
        return true;
    }

    void unload() {
        if (m_bundle) {
            CFBundleUnloadExecutable(m_bundle);
            CFRelease(m_bundle);
        }
        m_mainProc = 0;
    }




};

class Node;
class VstNode;
class MixerNode;
class NodeGroup;


class NodeGroup: public QObject {

    Q_OBJECT

    QVector<Node*> m_nodes;

public:

    virtual ~NodeGroup();

    void addNode(Node* node);

    unsigned numChildNodes() const {
        return m_nodes.size();
    }

    Node* childNode(unsigned index) const {
        return m_nodes[index];
    }

    const QVector<Node*>& childNodes() const {
        return m_nodes;
    }

    // is this necessary!?
    void emitNodeInputsChanged(Node* node)
    {
        emit nodeInputsChanged(node);
    }

signals:

    void nodeAdded(Node* node);

    void nodeInputsChanged(Node* node);

};


class Node: public QObject {

    Q_OBJECT

public:

    class Factory {
    public:
        virtual Node* create() = 0;
    };

private:

    NodeGroup* m_nodeGroup;

    // position in node graph.
    QPointF m_position;

protected:

    class ChannelData {
    public:
        QString name;
        QVector<float> buffer;

        ChannelData(const QString& name, size_t bufferSize)
            : name(name)
            , buffer(bufferSize) {}

        // why the fuck is this necessary to use ChannelData in a QVector?
        ChannelData() {}

    };

    QVector<ChannelData> m_inputChannelData; // WARNING: unused buffer ATM
    QVector<ChannelData> m_outputChannelData;

public:

    static int blockSize() {
        return 512;
    }

    struct Visitor {
        virtual void visit(Node* node) {}
        virtual void visit(VstNode* node);
        virtual void visit(MixerNode* node);
    };

    static const float* s_nullInputBuffer;

    Node()
        : m_nodeGroup(0) {
    }

    virtual NodeGroup* nodeGroup() const {
        return m_nodeGroup;
    }

    void setNodeGroup(NodeGroup* nodeGroup) {
        // WARNING: this doesn't actually add this node to the group!
        m_nodeGroup = nodeGroup;
    }

    virtual void accept(Visitor&) {}

    unsigned numInputChannels() const {
        return m_inputChannelData.count();
    }

    unsigned numOutputChannels() const {
        return m_outputChannelData.count();
    }

    float* outputChannelBuffer(int channelIndex) {
        return m_outputChannelData[channelIndex].buffer.data();
    }

    const float* outputChannelBufferConst(int channelIndex) {
        return m_outputChannelData[channelIndex].buffer.constData();
    }

    virtual QString displayLabel() const = 0;

    QPointF position() const {
        return m_position;
    }

    void setPosition(const QPointF& position) {
        //qDebug() << "Setting node position to" << position;
        m_position = position;
    }

    virtual void openEditorWindow() {
        // do nothing
    }

};


class MixerNode: public Node
{
    Q_OBJECT

    QList<Node*> m_inputs;
    //QVector<QVector<float> > m_outputChannelBuffers;

public:

    class Factory: public Node::Factory
    {

    public:

        virtual Node* create()
        {
            return new MixerNode;
        }

    };

    MixerNode()
    {
        m_outputChannelData << ChannelData("Mix L", blockSize());
        m_outputChannelData << ChannelData("Mix R", blockSize());
    }

    void addInput(Node* node)
    {
        m_inputs << node;
        inputsChanged();
    }

    void removeInput(int i)
    {
        m_inputs.removeAt(i);
        inputsChanged();
    }

    void removeInput(Node* node)
    {
        // definite bug if we allow more than one connection from the same node!
        m_inputs.removeOne(node);
        inputsChanged();
    }

    const QList<Node*>& inputs() const
    {
        return m_inputs;
    }

    void inputsChanged() {
        m_inputChannelData.clear();
        for (int i = 0; i < numInputs(); ++i) {
            for (int c = 0; c < 2; ++c) {
                m_inputChannelData << ChannelData(QString("In %1 %2").arg(QString::number(i), QString(c == 0 ? "L" : "R")), blockSize());
            }
        }
        nodeGroup()->emitNodeInputsChanged(this);
    }

    unsigned numInputs() const {
        return m_inputs.size();
    }

    Node* input(int i) const {
        return m_inputs[i];
    }

    virtual void accept(Visitor& visitor) {
        visitor.visit(this);
    }

    virtual QString displayLabel() const {
        return tr("Mixer");
    }

    virtual void processAudio() {

        // zero the result
        for (int c = 0; c < 2; ++c) {
            for (int s = 0; s < blockSize(); ++s) {
                outputChannelBuffer(c)[s] = 0;
            }
        }

        // accumulate
        for (int i = 0; i < numInputs(); ++i) {
            for (int c = 0; c < 2; ++c) {
                for (int s = 0; s < blockSize(); ++s) {
                    outputChannelBuffer(c)[s] += input(i)->outputChannelBufferConst(c)[s];
                }
            }
        }

    }

};

class NodeWindow;

class VstNode: public Node {

    Q_OBJECT

    AEffect* m_vstInstance;
    QMutex m_eventQueueMutex;
    QVector<VstEvent*> m_eventQueue[2];// 'double buffered' to mitigate lock contention.
    int m_eventQueueWriteIndex; // 0 or 1 depending on which of m_eventQueue is for writing.
    Node* m_input;
    QString m_productName;
    NodeWindow* m_editorWindow; // move this to Node eventually



public:

    class Factory: public Node::Factory {

        QString m_vstName;

    public:

        Factory(const QString& vstName) : m_vstName(vstName) {}

        virtual Node* create();
    };

    VstNode(VstModule* vstModule)
        : m_vstInstance(vstModule->createVstInstance())
        , m_eventQueueWriteIndex(0)
        , m_input(0)
        , m_editorWindow(0) {
        //this->setParent(vstModule); // hm, this causes a crash on the VstModule QObject children cleanup.

        const size_t blockSize = 512;

        qDebug() << "opening instance";

        m_vstInstance->dispatcher (m_vstInstance, effOpen, 0, 0, 0, 0);


        qDebug() << "getting product string";

        int ret = 0;
        char charBuffer[64] = {0};
        ret = m_vstInstance->dispatcher (m_vstInstance, effGetProductString, 0, 0, &charBuffer, 0);
        if (ret) {
            m_productName = QString(charBuffer);
        } else {
            m_productName = tr("Untitled");
        }

        qDebug() << "vst product string is" << m_productName;


        VstPinProperties pinProps;

        qDebug() << "output props";

        int pin;
        const int maxPins = 128;
        for (pin = 0; pin < maxPins; ++pin) {
            int ret = m_vstInstance->dispatcher (m_vstInstance, effGetInputProperties, pin, 0, &pinProps, 0);
            if (ret == 1) {
                qDebug() << "Input pin" << pin << pinProps.label << pinProps.flags;
                m_inputChannelData << ChannelData(pinProps.label, blockSize);
            } else {
                break;
            }
        }

        for (pin = 0; pin < maxPins; ++pin) {
            int ret = m_vstInstance->dispatcher (m_vstInstance, effGetOutputProperties, pin, 0, &pinProps, 0);
            if (ret == 1) {
                qDebug() << "Output pin" << pin << pinProps.label << pinProps.flags;
                m_outputChannelData << ChannelData(pinProps.label, blockSize);
            } else {
                break;
            }
        }

        qDebug() << "inputs:" << numInputChannels();
        qDebug() << "outputs:" << numOutputChannels();

//        VstSpeakerArrangement inputSpeakerArr = {kSpeakerArrStereo, 2};
//        VstSpeakerArrangement outputSpeakerArr = {kSpeakerArrStereo, 2};

//        VstSpeakerArrangement *in = 0, *out = 0;


//        qDebug() << "Setting speaker arrangements and shit";

//        int ret = m_vstInstance->dispatcher (m_vstInstance, effGetSpeakerArrangement, 0, (VstIntPtr)&in, &out, 0);

//        if (in) {
//            qDebug() << "initial speaker arrangement: ret" << ret << "inputs" << in->numChannels << "outputs" << out->numChannels;
//        }

//        ret = m_vstInstance->dispatcher (m_vstInstance, effSetSpeakerArrangement, 0, (VstIntPtr)&inputSpeakerArr, &outputSpeakerArr, 0);

//        ret = m_vstInstance->dispatcher (m_vstInstance, effGetSpeakerArrangement, 0, (VstIntPtr)&in, &out, 0);

//        if (in) {
//            qDebug() << "new speaker arrangement: ret" << ret << "inputs" << in->numChannels << "outputs" << out->numChannels;
//        }

        qDebug() << "setSampleRate";
        m_vstInstance->dispatcher (m_vstInstance, effSetSampleRate, 0, 0, 0, 44100);

        qDebug() << "setBlockSize";
        m_vstInstance->dispatcher (m_vstInstance, effSetBlockSize, 0, blockSize, 0, 0);

        qDebug() << "mainsChanged";
        m_vstInstance->dispatcher (m_vstInstance, effMainsChanged, 0, 1, 0, 0);


    }

    AEffect* vstInstance() const {
        return m_vstInstance;
    }

    virtual ~VstNode() {
        m_vstInstance->dispatcher (m_vstInstance, effClose, 0, 0, 0, 0);
        // TODO: how the f do we delete this once we're done with it?
        //delete m_vstInstance; // this SIGABRTs
        //free(m_vstInstance);// this malloc errors: pointer being freed was not allocated.
        m_vstInstance = 0;
    }

    const QString& productName() const {
        return m_productName;
    }

    virtual QString displayLabel() const {
        return tr("VST %1").arg(productName());
    }

    virtual void accept(Visitor& visitor) {
        visitor.visit(this);
    }

    void queueEvent(VstEvent* event) {
        QMutexLocker locker(&m_eventQueueMutex);
        Q_UNUSED(locker);
        m_eventQueue[m_eventQueueWriteIndex].push_back(event);
    }

    void processEvents() {
        // Take a reference to the event queue, and buffer swap
        QMutexLocker locker(&m_eventQueueMutex);
        QVector<VstEvent*>& eventQueue = m_eventQueue[m_eventQueueWriteIndex];
        m_eventQueueWriteIndex = !m_eventQueueWriteIndex;
        locker.unlock();

        size_t numEvents = eventQueue.size();
        if (numEvents) {
            //qDebug() << "processing events" << numEvents;
            VstEvents* events = (VstEvents*)malloc(sizeof(VstEvents) + sizeof(VstEvent*) * numEvents);
            events->numEvents = numEvents;
            events->reserved = 0;
            std::copy(eventQueue.begin(), eventQueue.end(), &events->events[0]);
            eventQueue.clear();

            events->events[numEvents] = 0;
            vstInstance()->dispatcher (vstInstance(), effProcessEvents, 0, 0, events, 0);

            // Think I need to free the events here. This could be done in a different thread?
            for (unsigned i = 0; i < numEvents; ++i) {
                free(events->events[i]);
            }

        }
    }

    void processAudio() {

        const int blockSize = 512; // temp hack!

        int numInputOutputChannels = 0;
        if (input()) {
            numInputOutputChannels = input()->numOutputChannels();
        }

        int numInputChannels = this->numInputChannels();

        const float* inputFloats[numInputChannels];

        for (int c = 0; c < numInputChannels; ++c) {
            if (c < numInputOutputChannels) {
                inputFloats[c] = input()->outputChannelBufferConst(c);
            } else {
                inputFloats[c] = s_nullInputBuffer;
            }
        }

        int numOutputChannels = this->numOutputChannels();

        float* outputFloats[numOutputChannels];

        for (int c = 0; c < numOutputChannels; ++c) {
            outputFloats[c] = outputChannelBuffer(c);
        }

        vstInstance()->processReplacing(vstInstance(), const_cast<float**>(inputFloats), outputFloats, blockSize);
    }

    void setInput(Node* node) {
        //qDebug() << "setting input to" << node;
        m_input = node;
        nodeGroup()->emitNodeInputsChanged(this);
    }

    Node* input() const {
        return m_input;
    }

    virtual void openEditorWindow();

};




class VstEditorWidget: public QMacCocoaViewContainer {
    Q_OBJECT

    VstNode* m_vstNode;
    class NSView* m_view;

public:
    VstEditorWidget(VstNode* vstNode, QWidget* parent = 0);
    virtual ~VstEditorWidget();

    QSize sizeHint() const;

    void showEvent(QShowEvent* event);

    void hideEvent(QHideEvent* event);

};



class Host: public QObject {

    Q_OBJECT

    QIODevice* m_ioDevice; // not owned
    QAudioFormat m_format;
    QByteArray m_buffer; // PCM audio buffer
    //QVector<float> m_floatBlock[4]; // 2 in 2 out
    QAudioOutput* m_audioOutput;
    QTimer* m_timer;
    unsigned long m_globalSampleIndex;
    Node* m_outputNode;
    unsigned m_blockSizeSamples;

    // stuff for profiling utilisation
    uint64_t m_lastProfiledTime;
    uint64_t m_busyAccumulatedTime;

    QVector<float> m_nullInputBuffer;

public:

    Host(Node* outputNode)
        : m_ioDevice(0)
        , m_audioOutput(0)
        , m_timer(0)
        , m_globalSampleIndex(0)
        , m_outputNode(outputNode)
        , m_lastProfiledTime(0)
        , m_busyAccumulatedTime(0) {
        init();
    }

    virtual ~Host() {
        delete m_audioOutput;
    }

    void init() {

        // Set up the format, eg.
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

        int sampleIndex = 0;

        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(0);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(writeData()));
        m_timer->start();

    }

    void stop() {
        m_timer->stop();
    }


    const float* nullInputBuffer() const {
        return m_nullInputBuffer.constData();
    }


public slots:

    void stateChanged(QAudio::State state) {
        qDebug() << "State changed" << state;
    }

    void writeData() {

        //QElapsedTimer elTimer;
        //elTimer.start();

        if (m_lastProfiledTime != 0) {
            uint64_t thisTime = mach_absolute_time();
            uint64_t elapsed = thisTime - m_lastProfiledTime;
            uint64_t elapsedNano = *(uint64_t*) &AbsoluteToNanoseconds( *(AbsoluteTime *) &elapsed );
            if (elapsedNano > 2e8) {
                uint64_t busyNano = *(uint64_t*) &AbsoluteToNanoseconds( *(AbsoluteTime *) &m_busyAccumulatedTime );
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




//                // might need to strip out all the heap stuff in here to make this more efficient?
//                struct Dispatcher {

//                    typedef QVector<QVector<float> > FloatChannels;

//                    typedef QMap<Node*, FloatChannels> nodeBlocksMap_t;
//                    nodeBlocksMap_t m_nodeBlocksMap;

//                    Host* host;


//                    Dispatcher(Host* host) : host(host) {}

//                    FloatChannels process(Node* node, unsigned blockSize) {


//                        nodeBlocksMap_t::iterator it = m_nodeBlocksMap.find(node);
//                        if (it != m_nodeBlocksMap.end()) {
//                            // already calculated this mofo.
//                            return it.value();
//                        }





//                        // initialise our output to zeros.
//                        FloatChannels outputBlocks(numChannels);
//                        for (int channel = 0; channel < numChannels; ++channel) {
//                            outputBlocks[channel].resize(blockSize);
//                        }

//                        do {

//                            MixerNode* mixerNode = node->mixerNode();
//                            if (mixerNode) {
//                                for (int input = 0; input < mixerNode->numInputs(); ++input) {
//                                    FloatChannels thisOutputBlocks = this->process(mixerNode->input(input), blockSize);
//                                    for (int channel = 0; channel < numChannels; ++channel) {
//                                        for (int sample = 0; sample < blockSize; ++sample) {
//                                            outputBlocks[channel][sample] += thisOutputBlocks[channel][sample];
//                                        }
//                                    }
//                                }
//                                break;
//                            }

//                            VstNode* vstNode = node->vstNode();
//                            if (vstNode) {

//                                const int numInputChannels = vstNode->numInputChannels();
//                                const int numOutputChannels = vstNode->numOutputChannels();

//                                FloatChannels inputBlocks;
//                                if (vstNode->input()) {
//                                    inputBlocks = this->process(vstNode->input(), blockSize);
//                                }

//                                const float* inputFloats[numInputChannels];

//                                for (int channel = 0; channel < numInputChannels; ++channel) {
//                                    if (channel < inputBlocks.size()) {
//                                        inputFloats[channel] = inputBlocks[channel].constData();
//                                    } else {
//                                        inputFloats[channel] = this->host->nullInputBuffer();
//                                    }
//                                }

//                                float* outputFloats[numOutputChannels];
//                                for (int channel = 0; channel < numOutputChannels; ++channel) {
//                                    outputFloats[channel] = vstNode->outputChannelBuffer(channel);
//                                }

//                                vstNode->processEvents();
//                                vstNode->vstInstance()->processReplacing(vstNode->vstInstance(), const_cast<float**>(inputFloats), outputFloats, blockSize);
//                                break;

//                            }

//                            // unknown node type!
//                            Q_ASSERT(false);

//                        } while (0);

//    //                    // peak detection
//    //                    double peak[2] = {0., 0.};
//    //                    double rmsAccum[2] = {0., 0.};
//    //                    for (int channel = 0; channel < 2; ++channel) {
//    //                        for (int sample = 0; sample < blockSize; ++sample) {
//    //                            float thisSample = outputBlocks[channel][]
//    //                        }
//    //                    }


//                        m_nodeBlocksMap.insert(node, outputBlocks);
//                        return outputBlocks;
//                    }
//                } dispatcher(this);

                //QVector<QVector<float> > outputBlocks = dispatcher.process(outputNode, m_blockSizeSamples);

//                QVector<QVector<float> > outputBlocks(2);
//                for (int i = 0; i < 2; ++i) {
//                    outputBlocks[i].resize(m_blockSizeSamples);
//                }


                //QMap<Node*, QSet<Node*> > dependencyMap;
                //QMap<Node*, QSet<Node*> > dependentMap;


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


                //qApp->exit();

                struct SerialNodeExecutor: public Node::Visitor {

                    DependencyVisitor& dependencyVisitor;

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
                            node->accept(*this);
                        }
                    }

                    void postVisit(Node* node) {
                        Q_FOREACH (Node* dependentNode, dependencyVisitor.dependentMap[node]) {
                            dependentNode->accept(*this);
                        }
                    }

                    virtual void visit(MixerNode* mixerNode) {
                        mixerNode->processAudio();
                        postVisit(mixerNode);
                    }

                    virtual void visit(VstNode* vstNode) {
                        vstNode->processEvents();
                        vstNode->processAudio();
                        postVisit(vstNode);
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

    void notified()
    {
//        qWarning() << "bytesFree = " << m_audioOutput->bytesFree()
//                   << ", " << "elapsedUSecs = " << m_audioOutput->elapsedUSecs()
//                   << ", " << "processedUSecs = " << m_audioOutput->processedUSecs();
    }

signals:
    void utilisation(float);

};



class HostThread : public QThread {

    Q_OBJECT

    //QVector<VstNode*> m_vstNodes;
    Node* m_outputNode;
    Host* m_host;
public:
    HostThread(Node* outputNode)
        : m_outputNode(outputNode)
        , m_host(0) {}

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
    Ui::MainWindow *ui;
    VstNode* m_vstNode; // not owned
    QWidget* m_vstEditorWidget;
    int m_note;
    QVector<int> m_funkyTown;
    unsigned m_funkyTownPos;
    int m_lastNote;
};

// UNUSED OBSOLETE DEAD CLASS
class NodeButton: public QPushButton {

    Q_OBJECT

public:

    NodeButton(Node* node, QWidget* parent=0)
        : m_node(node) {
        //setCheckable(true);
        setText(node->displayLabel());
        connect(this, SIGNAL(clicked()), SLOT(openEditorWindow()));
    }

private slots:

    void openEditorWindow();

private:

    Node* m_node;

};



class RootGraphicsItem: public QGraphicsItem {

public:

    QRectF boundingRect() const {
        return QRectF(0, 0, 0, 0);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
        // do nothing;
    }


};




void graphicsItemStackOnTop(QGraphicsItem* item);


class InputSetter: public QObject
{
    Q_OBJECT

    VstNode* m_node;

public:

    InputSetter(VstNode* node)
        : m_node(node)
    {

    }

public slots:

    void setInput(QObject* inputNodeObject)
    {
        qDebug() << "setting input";
        Node* inputNode = qobject_cast<Node*>(inputNodeObject);
        // TODO: mutex
        m_node->setInput(inputNode);
    }

    void unsetInput()
    {
        m_node->setInput(0);
    }

};

class InputChanger: public QObject
{
    Q_OBJECT

    MixerNode* m_node;

public:

    InputChanger(MixerNode* node)
        : m_node(node)
    {

    }

public slots:

    void addInput(QObject* inputNodeObject)
    {
        //qDebug() << "adding input";
        Node* inputNode = qobject_cast<Node*>(inputNodeObject);
        // TODO: mutex
        m_node->addInput(inputNode);
    }

    void removeInput(QObject* inputNodeObject)
    {
        //qDebug() << "removing input";
        Node* inputNode = qobject_cast<Node*>(inputNodeObject);
        // TODO: mutex
        m_node->removeInput(inputNode);
    }

};


/*

  random thoughts:

   - should maintain one scene per group?

*/

class NodeConnectionArrow;

class NodeGraphicsItem: public QGraphicsItem {

    Node* m_node; // not owned!
    QSet<NodeConnectionArrow*> m_connectionArrows;

public:

    NodeGraphicsItem(Node* node)
        : m_node(node) {

        setFlag(ItemIsMovable);
        setFlag(ItemSendsGeometryChanges);

        setAcceptHoverEvents(true);

        setPos(m_node->position());

        QGraphicsSimpleTextItem* label = new QGraphicsSimpleTextItem(this);
        label->setText(m_node->displayLabel());
        label->setPos(-label->boundingRect().width() / 2.0, -label->boundingRect().height() / 2.0);

    }

    QRectF boundingRect() const {
        return QRectF(QPointF(-100, -20), QSizeF(200, 40));
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
        QBrush brush;
        brush = QBrush(QColor(240, 240, 240));
        painter->setBrush(brush);
        // FIXME: use a standard brush somehow? or just use a palette
        qreal radius = 10.;

        QPen pen(painter->pen());
        pen.setWidthF(2.0);
        painter->setPen(pen);

        QRectF rect = boundingRect();
        qreal halfPenWidth = painter->pen().widthF() / 2.;
        rect.adjust(halfPenWidth, halfPenWidth, -halfPenWidth, -halfPenWidth);
        painter->drawRoundedRect(rect, radius, radius);
    }

    void addConnectionArrow(NodeConnectionArrow* arrow)
    {
        m_connectionArrows << arrow;
    }

    void removeConnectionArrow(NodeConnectionArrow* arrow)
    {
        m_connectionArrows.remove(arrow);
    }

protected:

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        if (change == ItemPositionChange) {
            // value is the new position.
            QPointF newPos = value.toPointF();
            m_node->setPosition(newPos);
            updateConnectionArrows();
        }
        return QGraphicsItem::itemChange(change, value);
    }

private:

    void updateConnectionArrows() const;

};

class NodeConnectionArrow: public QGraphicsLineItem {

    Node* m_node;
    Node* m_inputNode;
    QGraphicsPolygonItem* m_arrowhead;
    QGraphicsEllipseItem* m_arrowtail;

public:

    NodeConnectionArrow(Node* node, Node* inputNode)
        : m_node(node)
        , m_inputNode(inputNode)
        , m_arrowhead(0)
        , m_arrowtail(0) {

        QPen pen(this->pen());
        pen.setWidth(2);
        setPen(pen);

        QPolygonF polygon;
        polygon << QPointF(0, 0) << QPointF(-12, 6) << /*QPointF(10, 0) <<*/ QPointF(-12, -6);
        m_arrowhead = new QGraphicsPolygonItem(polygon);
        m_arrowhead->setPen(Qt::NoPen);
        QBrush brush(m_arrowhead->brush());
        brush.setStyle(Qt::SolidPattern);
        m_arrowhead->setBrush(brush);
        m_arrowhead->setParentItem(this);

        m_arrowtail = new QGraphicsEllipseItem(-5, -5, 10, 10);
        m_arrowtail->setPen(Qt::NoPen);
        m_arrowtail->setBrush(brush);
        m_arrowtail->setParentItem(this);

        updatePositions();
    }

    void updatePositions() {
        // FIXME: these offsets are hardcoded...
        QLineF arrowLine(m_inputNode->position() + QPointF(0, 20), m_node->position() + QPointF(0, -20));
        qreal angle = arrowLine.angle();

        QLineF arrowBodyLine(arrowLine);
        arrowBodyLine.setLength(arrowBodyLine.length() - 5); // shave a bit off the line so we don't overdraw at the tip of the arrowhead.
        setLine(arrowBodyLine);

        m_arrowhead->setPos(arrowLine.p2());
        m_arrowhead->setRotation(-angle);
        m_arrowtail->setPos(arrowLine.p1());
    }

    Node* node() const
    {
        return m_node;
    }

    Node* inputNode() const
    {
        return m_inputNode;
    }

};


class NodeGraphEditor: public QGraphicsView {

    Q_OBJECT

    NodeGroup* m_nodeGroup; // not owned
    QGraphicsScene* m_scene;
    QGraphicsItem* m_rootItem;
    QMap<Node*, NodeGraphicsItem*> m_nodeItemMap;
    QMap<Node*, QSet<NodeConnectionArrow*> > m_nodeInputArrows;

public:

    NodeGraphEditor(NodeGroup* nodeGroup, QWidget* parent = 0)
        : QGraphicsView(parent)
        , m_nodeGroup(nodeGroup)
        , m_scene(0)
        , m_rootItem(0)
    {

        setFocusPolicy(Qt::StrongFocus);

        // Enable multisampling for decent AA in the graph
        if (false) {
            QGLFormat glFormat;
            glFormat.setSampleBuffers(true);
            glFormat.setSamples(4);
            setViewport(new QGLWidget(glFormat));
            setRenderHint(QPainter::Antialiasing);
        }
        // is this necessary?
        //setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

        m_scene = new QGraphicsScene(this);
        m_scene->setSceneRect(-1000, -1000, 2000, 2000);
        setScene(m_scene);

        m_rootItem = new RootGraphicsItem;
        scene()->addItem(m_rootItem);

        // this code /will/ move at some point
        for (int i = 0; i < m_nodeGroup->numChildNodes(); ++i) {
            nodeAdded(m_nodeGroup->childNode(i));
        }

        // dodgy, but it'll do for now
        for (int i = 0; i < m_nodeGroup->numChildNodes(); ++i) {
            nodeInputsChanged(m_nodeGroup->childNode(i));
#if 0
            Node* node = m_nodeGroup->childNode(i);
            MixerNode* mixerNode = dynamic_cast<MixerNode*>(node);
            if (mixerNode) {
                for (int i = 0; i < mixerNode->numInputs(); ++i) {
                    Node* inputNode = mixerNode->input(i);
                    NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                    m_nodeItemMap[node]->addConnectionArrow(arrow);
                    m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                    arrow->setParentItem(m_rootItem);
                }
            }
            VstNode* vstNode = dynamic_cast<VstNode*>(node);
            if (vstNode) {
                Node* inputNode = vstNode->input();
                if (inputNode) {
                    NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                    m_nodeItemMap[node]->addConnectionArrow(arrow);
                    m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                    arrow->setParentItem(m_rootItem);
                }
            }
#endif
        }


        connect(m_nodeGroup, SIGNAL(nodeAdded(Node*)), SLOT(nodeAdded(Node*)));
        connect(m_nodeGroup, SIGNAL(nodeInputsChanged(Node*)), SLOT(nodeInputsChanged(Node*)));

    }

protected:

    virtual void keyPressEvent(QKeyEvent *event);

protected slots:

    void nodeAdded(Node* node)
    {
        //qDebug() << "NODE ADDED" << node->displayLabel();

        NodeGraphicsItem* item = new NodeGraphicsItem(node);
        item->setParentItem(m_rootItem);
        m_nodeItemMap[node] = item;

        //nodeInputsChanged(node);
    }

    void nodeInputsChanged(Node* node)
    {
        // FIXME: do nothing if the node isn't in the graph yet - we'll get to that after nodeAdded!
        //qDebug() << "rejigging node inputs for node" << node->displayLabel();
        // delete all current arrows and re-add
        // this could probably use a re-think. contain the madness!
        // so bored of this code, apologies that it's retarded.
        Q_FOREACH (NodeConnectionArrow* arrow, m_nodeInputArrows[node]) {
            m_nodeItemMap[arrow->inputNode()]->removeConnectionArrow(arrow);
            m_nodeItemMap[node]->removeConnectionArrow(arrow);
            delete arrow;
        }
        // clear set of dangling pointers
        m_nodeInputArrows[node].clear();

        MixerNode* mixerNode = dynamic_cast<MixerNode*>(node);
        if (mixerNode) {
            for (int i = 0; i < mixerNode->numInputs(); ++i) {
                Node* inputNode = mixerNode->input(i);
                NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                m_nodeItemMap[node]->addConnectionArrow(arrow);
                m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                m_nodeInputArrows[node] << arrow;
                arrow->setParentItem(m_rootItem);
            }
        }
        VstNode* vstNode = dynamic_cast<VstNode*>(node);
        if (vstNode) {
            Node* inputNode = vstNode->input();
            if (inputNode) {
                NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                m_nodeItemMap[node]->addConnectionArrow(arrow);
                m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                m_nodeInputArrows[node] << arrow;
                arrow->setParentItem(m_rootItem);
            }
        }

    }

};


class MainWindow: public QMainWindow {

    Q_OBJECT

    QProgressBar* m_utilisationMeter;
    NodeGroup* m_group;

public:

    MainWindow(NodeGroup* group, QWidget* parent=0)
        : QMainWindow(parent)
        , m_group(group) {

        setWindowTitle("Evilnote");

        QWidget* widget = new QWidget();

        QVBoxLayout* mainLayout = new QVBoxLayout(widget);
        setCentralWidget(widget);

        m_utilisationMeter = new QProgressBar(this);
        m_utilisationMeter->setRange(0, INT_MAX);

        mainLayout->addWidget(m_utilisationMeter);
        //setCentralWidget(m_utilisationMeter);

#if 0
        for (unsigned i = 0; i < m_group->numChildNodes(); ++i) {
            Node* node = m_group->childNode(i);
            NodeButton* button = new NodeButton(node, widget);
            mainLayout->addWidget(button);
        }
#endif


        NodeGraphEditor* graph = new NodeGraphEditor(group);

        mainLayout->addWidget(graph);

    }

public slots:

    void utilisation(float utilisationAmount) {
        m_utilisationMeter->setValue(utilisationAmount * INT_MAX);
    }

};



inline void Node::Visitor::visit(VstNode* node) {
    visit(static_cast<Node*>(node));
}

inline void Node::Visitor::visit(MixerNode* node) {
    visit(static_cast<Node*>(node));
}



struct VstInfo {
    QString name;
    QString moduleFileName;
};


class Core : public QObject {

    Q_OBJECT

    typedef QMap<QString, VstInfo> VstInfoMap;
    VstInfoMap m_vstInfoMap;
    QMap<QString, VstModule*> m_vstModuleMap;

    static Core* s_instance;

public:

    Core() {
        Q_ASSERT(s_instance == 0);
        s_instance = this;
    }

    ~Core();

    static Core* instance() {
        return s_instance;
    }

    void scanVstDir(QDir dir);

    void scanVstDirs();

    VstModule* vstModule(const QString& vstName);

    const VstInfoMap& vstInfoMap() {
        return m_vstInfoMap;
    }

};


class NodeCreationWidget: public QLineEdit {

    Q_OBJECT

    NodeGroup* m_nodeGroup;
    typedef QMap<QString, Node::Factory*> FactoryMap;
    FactoryMap m_factories;
    QPointF m_pos;

public:

    NodeCreationWidget(NodeGroup* nodeGroup, QPointF pos, QWidget* parent = 0)
        : QLineEdit(parent)
        , m_nodeGroup(nodeGroup)
        , m_pos(pos)
    {

        connect(this, SIGNAL(returnPressed()), SLOT(createNode()));
        // TODO: escape handling

        m_factories["Mixer"] = new MixerNode::Factory();


        Core* core = Core::instance();
        Q_FOREACH (QString name, core->vstInfoMap().keys()) {
            m_factories[name] = new VstNode::Factory(name);
        }

        QCompleter* comp = new QCompleter(m_factories.keys(), this);
        // argh, this disables the popup. lame.
        //comp->setCompletionMode(QCompleter::InlineCompletion);
        comp->setCaseSensitivity(Qt::CaseInsensitive);
        setCompleter(comp);

    }

    ~NodeCreationWidget() {
        Q_FOREACH (Node::Factory* factory, m_factories) {
            delete factory;
        }
    }

signals:

    void done();

private slots:

    void createNode() {
        QString name = this->text().trimmed();

        // if there's exactly 1 item in the completion, just run with it. bit crappy, but useful.
        if (completer()->completionCount() == 1) {
            completer()->setCurrentRow(0);
            name = completer()->currentCompletion();
        }

        //qDebug() << "Creating node for:" << name;

        FactoryMap::Iterator it = m_factories.find(name);
        if (it == m_factories.end()) {
            //qDebug() << "nope";
            // TODO: some kind of error feedback
            return;
        }
        Node* node = it.value()->create();
        node->setPosition(m_pos);
        m_nodeGroup->addNode(node);

        emit done();
    }

};

class NodeCreationDialog: public QDialog {

    Q_OBJECT

    NodeGroup* m_nodeGroup;

public:

    NodeCreationDialog(NodeGroup* nodeGroup, QPointF pos, QWidget* parent = 0);

private:

    void initCocoa();

};



} // namespace En










#endif // MAINWINDOW_H
