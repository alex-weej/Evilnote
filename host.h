#pragma once

#include <QtCore>
#include <QtMultimedia>

#include "mididevice.hh"

namespace En
{

class Node;
class NodeGroup;

class Host : public QObject
{
    Q_OBJECT

    QIODevice* m_ioDevice; // not owned
    QAudioFormat m_format;
    QByteArray m_buffer; // PCM audio buffer
    //QVector<float> m_floatBlock[4]; // 2 in 2 out
    QAudioOutput* m_audioOutput;
    QTimer* m_timer;
    unsigned long m_globalSampleIndex;
    NodeGroup* m_rootGroup;
    unsigned m_blockSizeSamples;

    // stuff for profiling utilisation
    uint64_t m_lastProfiledTime;
    uint64_t m_busyAccumulatedTime;

    QVector<float> m_nullInputBuffer;

    MidiDevice *m_globalMidiDevice;

public:

    Host(NodeGroup* rootGroup);

    virtual ~Host();

    void init();

    void stop();

    const float* nullInputBuffer() const {
        return m_nullInputBuffer.constData();
    }

    MidiDevice *globalMidiDevice() const {
        return m_globalMidiDevice;
    }

public slots:

    void stateChanged(QAudio::State state);

    void writeData();

signals:

    void utilisation(float);

};

} // namespace En
