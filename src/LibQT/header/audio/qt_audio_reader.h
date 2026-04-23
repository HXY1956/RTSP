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

