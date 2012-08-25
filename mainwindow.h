#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMultimedia>
#include <QAudioOutput>
#include <QMacCocoaViewContainer>

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
//#include <Cocoa/Cocoa.h>

#include "pluginterfaces/vst2.x/aeffectx.h"
#include <algorithm>




namespace En {



typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);

// TODO: add this to the AudioWriter object
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

    //if (!filtered)
    //    qDebug("PLUG> HostCallback (opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f", opcode, index, FromVstPtr<void> (value), ptr, opt);

    switch (opcode)
    {
        case audioMasterVersion:
            result = kVstVersion;
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



class VstNode : public QObject {
    Q_OBJECT

    AEffect* m_vstInstance;
    std::vector<VstEvent*> m_eventQueue;

public:
    //VstNode(AEffect* vstInstance) : m_vstInstance(vstInstance) {
    VstNode(VstModule* vstModule) : m_vstInstance(vstModule->createVstInstance()) {
        this->setParent(vstModule); // is this right?
        m_vstInstance->dispatcher (m_vstInstance, effOpen, 0, 0, 0, 0);
        m_vstInstance->dispatcher (m_vstInstance, effSetSampleRate, 0, 0, 0, 44100);
        m_vstInstance->dispatcher (m_vstInstance, effSetBlockSize, 0, 512, 0, 0);
        m_vstInstance->dispatcher (m_vstInstance, effMainsChanged, 0, 1, 0, 0);
    }

    AEffect* vstInstance() const {
        return m_vstInstance;
    }

    ~VstNode() {
        m_vstInstance->dispatcher (m_vstInstance, effClose, 0, 0, 0, 0);
        // TODO: how the f do we delete this once we're done with it?
        //delete m_vstInstance; // this SIGABRTs
        m_vstInstance = 0;
    }


    void queueEvent(VstEvent* event) {
        m_eventQueue.push_back(event);
    }

    void processEvents() {
        size_t numEvents = m_eventQueue.size();
        if (numEvents) {
            qDebug() << "processing events" << numEvents;
            VstEvents* events = (VstEvents*)malloc(sizeof(VstEvents) + sizeof(VstEvent*) * (numEvents - 1));
            events->numEvents = numEvents;
            events->reserved = 0;
            //VstEvent* event = &events->events[0];
            std::copy(m_eventQueue.begin(), m_eventQueue.end(), &events->events[0]);
            events->events[numEvents] = 0;
            vstInstance()->dispatcher (vstInstance(), effProcessEvents, 0, 0, events, 0);
            m_eventQueue.clear();
        }
    }


};




class VstInterfaceWidget: public QMacCocoaViewContainer {
    Q_OBJECT

    VstNode* m_vstNode;

public:
    VstInterfaceWidget(VstNode* vstNode, QWidget* parent = 0);

    QSize sizeHint() const {
        QSize ret = QMacCocoaViewContainer::sizeHint();
        qDebug() << "sizeHint" << ret;
        return ret;
    }

};



class Host : public QObject {

    Q_OBJECT

    QIODevice* m_ioDevice; // not owned
    QAudioFormat m_format;
    QByteArray m_buffer;
    QVector<float> m_floatBlock[2];
    QAudioOutput* m_audioOutput;
    QTimer* m_timer;
    unsigned long m_globalSampleIndex;
    En::VstNode* m_vstNode;
    unsigned m_blockSizeSamples;

public:

    Host(En::VstNode* vstNode)
        : m_ioDevice(0)
        , m_audioOutput(0)
        , m_timer(0)
        , m_globalSampleIndex(0)
        , m_vstNode(vstNode) {
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

        const int bufferSizeSamples = 4096;
        m_audioOutput->setBufferSize(bufferSizeSamples * m_format.channels() * m_format.sampleSize() / 8);


        connect(m_audioOutput, SIGNAL(notify()), SLOT(notified()));
        connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));

        m_ioDevice = m_audioOutput->start();

        qDebug() << m_audioOutput->error() << QAudio::NoError;

        qDebug() << "buffer size" << m_audioOutput->bufferSize();
        qDebug() << "period size" << m_audioOutput->periodSize();

        m_blockSizeSamples = m_audioOutput->periodSize() / m_format.channelCount() / (m_format.sampleSize() / 8);
        qDebug() << "block size (samples)" << m_blockSizeSamples;

        for (int i = 0; i < 2; ++i)
            m_floatBlock[i].resize(m_blockSizeSamples);

        m_buffer.resize(m_audioOutput->periodSize());

        int sampleIndex = 0;

        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(20);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(writeData()));
        m_timer->start();

        //m_audioOutput->resume();


    }


public slots:

    void stateChanged(QAudio::State state) {
        qDebug() << "State changed" << state;
    }

    void writeData() {

        if (!m_audioOutput || m_audioOutput->state() == QAudio::StoppedState) {
            return;
        }

        //qDebug() << "checking in writeData";

        //qDebug() << "bytesFree" << m_audioOutput->bytesFree();
        //qDebug() << "periodSize" << m_audioOutput->periodSize();

        int chunks = m_audioOutput->bytesFree() / m_audioOutput->periodSize();

        //qDebug() << "chunks" << chunks;

        int sampleRate = m_format.sampleRate();


        float* shadowFloats[2];
        shadowFloats[0] = m_floatBlock[0].data();
        shadowFloats[1] = m_floatBlock[1].data();

        for (int i = 0; i < chunks; ++i) {
        // generate a buffer and write it


            // generate float data first, like rendering VST.
            if (0) {
                for (unsigned sampleIndex = 0; sampleIndex < m_blockSizeSamples; sampleIndex++) {
                    m_floatBlock[0][sampleIndex] = qSin(2.f * M_PI * 880 * (m_globalSampleIndex % sampleRate) / sampleRate);
                    m_floatBlock[1][sampleIndex] = qSin(2.f * M_PI * 990 * (m_globalSampleIndex % sampleRate) / sampleRate);
                    m_globalSampleIndex++;
                }
            } else {
                m_vstNode->processEvents();
                m_vstNode->vstInstance()->processReplacing (m_vstNode->vstInstance(), 0, shadowFloats, m_blockSizeSamples);
            }

            // convert float data buffers to our audio format

            qint16* ptr = (qint16*)m_buffer.data();

            for (unsigned sampleIndex = 0; sampleIndex < m_blockSizeSamples; sampleIndex++) {
                for (int channel = 0; channel < m_format.channelCount(); ++channel) {
                    *ptr++ = qint16(m_floatBlock[channel][sampleIndex] * 32767);
                }
            }


            //qDebug("writing buffer");
            qint64 written = m_ioDevice->write(m_buffer.data(), m_buffer.size());
            //m_ioDevice->waitForBytesWritten(-1);
            //qDebug("%lld bytes written!", written);

            //qDebug() << "error state" << m_audioOutput->error() << QAudio::NoError;
        }

        m_timer->start();
    }

    void notified()
    {
        qWarning() << "bytesFree = " << m_audioOutput->bytesFree()
                   << ", " << "elapsedUSecs = " << m_audioOutput->elapsedUSecs()
                   << ", " << "processedUSecs = " << m_audioOutput->processedUSecs();
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
    int note;
};







#endif // MAINWINDOW_H
