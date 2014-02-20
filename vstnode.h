#pragma once

#include "node.h"
#include "nodewindow.h"
#include "vstmodule.h"
#include "mididevice.h"
#include "en.h"
#include <QtCore>

// TODO: change this so that the vst module instantiates this node?

namespace En
{

class VstNode : public Node
{
    Q_OBJECT

    AEffect* m_vstInstance;
    QMutex m_eventQueueMutex;
    QVector<VstEvent*> m_eventQueue[2];// 'double buffered' to mitigate lock contention.
    int m_eventQueueWriteIndex; // 0 or 1 depending on which of m_eventQueue is for writing.
    Node* m_input;
    MidiDevice *m_midiInput;
    QString m_productName;
    NodeWindow* m_editorWindow; // move this to Node eventually

public:

    class Factory: public Node::Factory
    {
        QString m_vstName;

    public:

        Factory(const QString& vstName) : m_vstName(vstName) {}

        virtual Node* create(Host *);
    };

    VstNode(Host* host, VstModule* vstModule) :
        m_vstInstance(vstModule->createVstInstance()),
        m_eventQueueWriteIndex(0),
        m_input(0),
        m_editorWindow(0)
    {
        //this->setParent(vstModule); // hm, this causes a crash on the VstModule QObject children cleanup.


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
                m_inputChannelData << ChannelData(pinProps.label, blockSize());
            } else {
                break;
            }
        }

        for (pin = 0; pin < maxPins; ++pin) {
            int ret = m_vstInstance->dispatcher (m_vstInstance, effGetOutputProperties, pin, 0, &pinProps, 0);
            if (ret == 1) {
                qDebug() << "Output pin" << pin << pinProps.label << pinProps.flags;
                m_outputChannelData << ChannelData(pinProps.label, blockSize());
            } else {
                break;
            }
        }

        // the AEffect::numInputs / numOutputs fields are the final word on IO config. Make sure we're set up right,
        // even if the effGetOutputProperties opcode lied to us.

        if (unsigned(m_vstInstance->numInputs) != numInputChannels() || unsigned(m_vstInstance->numOutputs) != numOutputChannels()) {

            if (numInputChannels() != 0 || numOutputChannels() != 0) {
                qDebug() << "WARNING: plugins input/output properties don't match actual number of inputs/outputs!";
            }

            m_inputChannelData.clear();
            for (int c = 0; c < m_vstInstance->numInputs; ++c) {
                m_inputChannelData << ChannelData(QString("In %2").arg(QString::number(c)), blockSize());
            }

            m_outputChannelData.clear();
            for (int c = 0; c < m_vstInstance->numOutputs; ++c) {
                m_outputChannelData << ChannelData(QString("Out %2").arg(QString::number(c)), blockSize());
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
        m_vstInstance->dispatcher (m_vstInstance, effSetBlockSize, 0, blockSize(), 0, 0);

        qDebug() << "mainsChanged";
        m_vstInstance->dispatcher (m_vstInstance, effMainsChanged, 0, 1, 0, 0);

        setMidiInput(host->globalMidiDevice());
    }

    AEffect* vstInstance() const {
        return m_vstInstance;
    }

    virtual ~VstNode();

    const QString& productName() const {
        return m_productName;
    }

    virtual QString displayLabel() const {
        return tr("%1 (VST)").arg(productName());
    }

    virtual void accept(Visitor& visitor) {
        visitor.visit(this);
    }

    void queueEvent(VstEvent* event) {
        QMutexLocker locker(&m_eventQueueMutex);
        Q_UNUSED(locker);
        m_eventQueue[m_eventQueueWriteIndex].push_back(event);
    }

    Q_SLOT void postMidiEvent(const QByteArray& midiEvent);

    void processAudio();

    void setInput(Node* node);

    Node* input() const {
        return m_input;
    }

    void setMidiInput(MidiDevice* node);

    MidiDevice *midiInput() const {
        return m_midiInput;
    }

    QList<Node*> inputs() const
    {
        QList<Node*> ret;
        ret << m_input;
        return ret;
    }

    void removeInput(Node* node);

    virtual void openEditorWindow();

};

}
