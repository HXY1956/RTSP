#ifndef QT_AUDIO_READER_H
#define QT_AUDIO_READER_H
#include <QObject>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include "base_global.h"
#include "qt_global.h"

using namespace BASE;

namespace AUDIO {
    class AUDIO_CODER;
}

namespace QT {
    /**
	*@简介  音频解码器类，继承自QObject，整合并提供对音频流的读取、解码和处理功能。它使用AUDIO_CODER进行音频解码，并通过信号与槽机制与其他组件进行通信。
    */
    class QT_AUDIO_READER : public QObject {
        Q_OBJECT
    public slots:
        void startWork();
        void stopWork();
        void onNewSampleRate(int sr);
        void onNewSampleSize(int sz);
        void onNewChannels(int channels);
        void onNewNoise(int noise);
    signals:
        void NewAudio(uint64_t _time, std::vector<uint8_t> _buffer);
        void stopped();

    public:
        QT_AUDIO_READER(AudioInfo Aset, ProcMode mode, QObject* parent = nullptr);
        ~QT_AUDIO_READER() override;

    protected:
        /**
		*@简介  单个周期的音频解码处理函数，负责从音频流中解码一帧音频数据，并将其通过信号发送出去
        */
        void OneEpoch();

    private:
        bool Status = true;
        std::atomic<bool> stop{ false };
        QTimer* cycleTimer = nullptr;
        std::unique_ptr<AUDIO::AUDIO_CODER> coder;
        std::atomic<bool> active{ false };
    };
}

#endif

