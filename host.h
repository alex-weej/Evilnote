#pragma once

#include <QtCore>
#include <QtMultimedia>

namespace En
{

class Node;

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
    Node* m_outputNode;
    unsigned m_blockSizeSamples;

    // stuff for profiling utilisation
    uint64_t m_lastProfiledTime;
    uint64_t m_busyAccumulatedTime;

    QVector<float> m_nullInputBuffer;

public:

    Host(Node* outputNode);

    virtual ~Host();

    void init();

    void stop();

    const float* nullInputBuffer() const {
        return m_nullInputBuffer.constData();
    }

public slots:

    void stateChanged(QAudio::State state);

    void writeData();

signals:

    void utilisation(float);

};

} // namespace En
