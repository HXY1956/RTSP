#include "qt_audio_reader.h"
#undef signals
#include "audio_decoder.h"
#include "audio_streamer.h"
#define signals protected

using namespace AUDIO;

namespace QT {
    QT_AUDIO_READER::QT_AUDIO_READER(AudioInfo Aset, ProcMode mode, QObject* parent) :
        QObject(parent), stop(false) {
        if (mode == ProcMode::ONLINE) {
            coder = std::make_unique<AUDIO_STREAMER>(Aset);
        }
        else if (mode == ProcMode::OFFLINE) {
            coder = std::make_unique<AUDIO_DECODER>(Aset);
        }
    };
    QT_AUDIO_READER::~QT_AUDIO_READER() {
    };

    void QT_AUDIO_READER::onNewSampleRate(int sr){
        //coder->setsamplerate(sr);
    }
    void QT_AUDIO_READER::onNewSampleSize(int sr){
        //coder->setsamplesize(sr);
    }
    void QT_AUDIO_READER::onNewChannels(int channels){
        //coder->setchannels(channels);
    }

    void QT_AUDIO_READER::onNewNoise(int noise){
        coder->setnoise(noise);
    }

    void QT_AUDIO_READER::OneEpoch() {
        if (stop.load()) {
            return;
        }
        active.store(true);
        std::vector<uint8_t> buffer;
        uint64_t timestamp;
        if (coder->Decode() < 0) {
            stop.store(true);
            coder->Stop();
            emit stopped();
            return;
        }
        while (coder->Iget(timestamp, buffer)) {
            emit NewAudio(timestamp, buffer);
        }
        if (stop.load()) {
            coder->Stop();
            emit stopped();
        }
        active.store(false);
    }

    void QT_AUDIO_READER::startWork() {
        if (!coder->Init()) {
            std::cerr << "Open Audio Stream Failed or No Audio Stream." << std::endl;
            Status = false;
            return;
        }
        cycleTimer = new QTimer(this);
        connect(cycleTimer, &QTimer::timeout, this, &QT_AUDIO_READER::OneEpoch);
        cycleTimer->start(5);
    }

    void QT_AUDIO_READER::stopWork()
    {
        if(!Status) return;
        stop.store(true);
        if (cycleTimer) {
            cycleTimer->stop();
        }
        if(!active.load()){
            coder->Stop();
            emit stopped();
        }
    }
}