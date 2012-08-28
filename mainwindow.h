#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMultimedia>
#include <QAudioOutput>
#include <QMacCocoaViewContainer>

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

#include "pluginterfaces/vst2.x/aeffectx.h"
#include <algorithm>

// bits for high res timing on Mach
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>




namespace En {

extern VstTimeInfo s_vstTimeInfo;

typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);


inline VstIntPtr hostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
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
        case audioMasterGetTime: // 7
            result = (VstIntPtr)&s_vstTimeInfo;
            break;

        case audioMasterGetCurrentProcessLevel: // 23
            result = kVstProcessLevelUser;
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

class Node: public QObject {
    Q_OBJECT
public:
    virtual MixerNode* mixerNode() {
        return 0;
    }
    virtual VstNode* vstNode() {
        return 0;
    }
};

class MixerNode: public Node {
    Q_OBJECT
    QVector<Node*> m_inputs;
public:
    virtual MixerNode* mixerNode() {
        return this;
    }

    void addInput(Node* node) {
        m_inputs.push_back(node);
    }

    unsigned numInputs() {
        return m_inputs.size();
    }

    Node* input(int i) {
        return m_inputs[i];
    }

};

class VstNode : public Node {
    Q_OBJECT

    AEffect* m_vstInstance;
    QMutex m_eventQueueMutex;
    QVector<VstEvent*> m_eventQueue[2];// 'double buffered' to mitigate lock contention.
    int m_eventQueueWriteIndex; // 0 or 1 depending on which of m_eventQueue is for writing.
    VstNode* m_input;

public:
    //VstNode(AEffect* vstInstance) : m_vstInstance(vstInstance) {
    VstNode(VstModule* vstModule)
        : m_vstInstance(vstModule->createVstInstance())
        , m_eventQueueWriteIndex(0)
        , m_input(0) {
        //this->setParent(vstModule); // hm, this causes a crash on the VstModule QObject children cleanup.
        m_vstInstance->dispatcher (m_vstInstance, effOpen, 0, 0, 0, 0);
        m_vstInstance->dispatcher (m_vstInstance, effSetSampleRate, 0, 0, 0, 44100);
        m_vstInstance->dispatcher (m_vstInstance, effSetBlockSize, 0, 512, 0, 0);
        m_vstInstance->dispatcher (m_vstInstance, effMainsChanged, 0, 1, 0, 0);
    }

    virtual VstNode* vstNode() {
        return this;
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


    void queueEvent(VstEvent* event) {
        QMutexLocker locker(&m_eventQueueMutex);
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
            qDebug() << "processing events" << numEvents;
            VstEvents* events = (VstEvents*)malloc(sizeof(VstEvents) + sizeof(VstEvent*) * (numEvents - 1));
            events->numEvents = numEvents;
            events->reserved = 0;
            std::copy(eventQueue.begin(), eventQueue.end(), &events->events[0]);
            eventQueue.clear();
            locker.unlock();

            events->events[numEvents] = 0;
            vstInstance()->dispatcher (vstInstance(), effProcessEvents, 0, 0, events, 0);

            // Think I need to free the events here. This could be done in a different thread?
            for (unsigned i = 0; i < numEvents; ++i) {
                free(events->events[i]);
            }

        }
    }

    void setInput(VstNode* vstNode) {
        m_input = vstNode;
    }

    VstNode* input() {
        return m_input;
    }


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



class Host : public QObject {

    Q_OBJECT

    QIODevice* m_ioDevice; // not owned
    QAudioFormat m_format;
    QByteArray m_buffer;
    QVector<float> m_floatBlock[4]; // 2 in 2 out
    QAudioOutput* m_audioOutput;
    QTimer* m_timer;
    unsigned long m_globalSampleIndex;
    Node* m_outputNode;
    unsigned m_blockSizeSamples;

    // stuff for profiling utilisation
    uint64_t m_lastProfiledTime;
    uint64_t m_busyAccumulatedTime;

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

        for (int i = 0; i < 4; ++i)
            m_floatBlock[i].resize(m_blockSizeSamples);

        m_buffer.resize(m_audioOutput->periodSize());

        int sampleIndex = 0;

        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(1);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(writeData()));
        m_timer->start();

    }

    void stop() {
        m_timer->stop();
    }


public slots:

    void stateChanged(QAudio::State state) {
        qDebug() << "State changed" << state;
    }

    void writeData() {

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


                // might need to strip out all the heap stuff in here to make this more efficient?
                struct Dispatcher {

                    typedef QVector<QVector<float> > FloatChannels;

                    typedef QMap<Node*, FloatChannels> nodeBlocksMap_t;
                    nodeBlocksMap_t m_nodeBlocksMap;

                    FloatChannels process(Node* node, unsigned blockSize) {


                        nodeBlocksMap_t::iterator it = m_nodeBlocksMap.find(node);
                        if (it != m_nodeBlocksMap.end()) {
                            // already calculated this mofo.
                            return it.value();
                        }



                        // initialise our output to zeros.
                        FloatChannels outputBlocks(2);
                        for (int channel = 0; channel < 2; ++channel) {
                            outputBlocks[channel].resize(blockSize);
                        }

                        do {

                            MixerNode* mixerNode = node->mixerNode();
                            if (mixerNode) {
                                for (int input = 0; input < mixerNode->numInputs(); ++input) {
                                    FloatChannels thisOutputBlocks = this->process(mixerNode->input(input), blockSize);
                                    for (int channel = 0; channel < 2; ++channel) {
                                        for (int sample = 0; sample < blockSize; ++sample) {
                                            outputBlocks[channel][sample] += thisOutputBlocks[channel][sample];
                                        }
                                    }
                                }
                                break;
                            }

                            VstNode* vstNode = node->vstNode();
                            if (vstNode) {
                                FloatChannels inputBlocks(2);
                                if (vstNode->input()) {
                                    inputBlocks = this->process(vstNode->input(), blockSize);
                                } else {
                                    for (int channel = 0; channel < 2; ++channel) {
                                        inputBlocks[channel].resize(blockSize); // will be zero filled.
                                    }
                                }

                                const float* inputFloats[2];
                                float* outputFloats[2];
                                for (int i = 0; i < 2; ++i) {
                                    inputFloats[i] = inputBlocks[i].constData();
                                    outputFloats[i] = outputBlocks[i].data();
                                }

                                vstNode->processEvents();
                                vstNode->vstInstance()->processReplacing(vstNode->vstInstance(), const_cast<float**>(inputFloats), outputFloats, blockSize);
                                break;

                            }

                            // unknown node type!
                            Q_ASSERT(false);

                        } while (0);

    //                    // peak detection
    //                    double peak[2] = {0., 0.};
    //                    double rmsAccum[2] = {0., 0.};
    //                    for (int channel = 0; channel < 2; ++channel) {
    //                        for (int sample = 0; sample < blockSize; ++sample) {
    //                            float thisSample = outputBlocks[channel][]
    //                        }
    //                    }


                        m_nodeBlocksMap.insert(node, outputBlocks);
                        return outputBlocks;
                    }
                } dispatcher;

                QVector<QVector<float> > outputBlocks = dispatcher.process(outputNode, m_blockSizeSamples);

                // convert float data buffers to our audio format

                qint16* ptr = (qint16*)m_buffer.data();

                for (unsigned sampleIndex = 0; sampleIndex < m_blockSizeSamples; sampleIndex++) {
                    for (int channel = 0; channel < m_format.channelCount(); ++channel) {
                        float clamped = std::min(std::max(outputBlocks[channel][sampleIndex], -1.f), 1.f);
                        *ptr++ = qint16(clamped * 32767);
                    }
                }


                //qDebug("writing buffer");
                qint64 written = m_ioDevice->write(m_buffer.data(), m_buffer.size());
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
    HostThread(Node* outputNode) : m_outputNode(outputNode), m_host(0) {

    }

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


class MainWindow: public QMainWindow {

    Q_OBJECT

    QProgressBar* m_utilisationMeter;

public:

    MainWindow(QWidget* parent=0)
        : QMainWindow(parent) {

        setWindowTitle("Evilnote");

        //QVBoxLayout* mainLayout = new QVBoxLayout(this);

        m_utilisationMeter = new QProgressBar(this);
        m_utilisationMeter->setRange(0, INT_MAX);

        //mainLayout->addWidget(m_utilisationMeter);
        setCentralWidget(m_utilisationMeter);

    }

public slots:

    void utilisation(float utilisationAmount) {
        m_utilisationMeter->setValue(utilisationAmount * INT_MAX);
    }

};


} // namespace En







namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(En::VstNode* vstNode, QWidget *parent = 0);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

private slots:
    void on_testNoteButton_pressed();

    void on_testNoteButton_released();

    void on_showEditorButton_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    En::VstNode* m_vstNode; // not owned
    int m_note;
    QVector<int> m_funkyTown;
    unsigned m_funkyTownPos;
    int m_lastNote;
};







#endif // MAINWINDOW_H
