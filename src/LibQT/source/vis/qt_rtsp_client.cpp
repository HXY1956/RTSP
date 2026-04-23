#include "qt_rtsp_client.h"
#undef signals
#include "rtsp_file.h"
#include "rtsp_live.h"
#define signals protected

using namespace RTSP;

namespace QT {
    QT_RTSP_CLIENT::QT_RTSP_CLIENT(AudioInfo Aset,CamInfo Cset, RtspInfo Rset, ProcMode mode, QObject* parent) :
        QObject(parent){
        if (mode == ProcMode::ONLINE) {
            rstp_worker = std::make_unique<RTSP_LIVE>(Aset, Cset, Rset);
        }
        else if (mode == ProcMode::OFFLINE) {
            rstp_worker = std::make_unique<RTSP_FILE>(Aset, Cset, Rset);
        }
        _smokeRemover = std::make_unique<CudaSmokeRemover>();
        _smokeRemover->setRemoverMethods(Rset.way);
    }

    QT_RTSP_CLIENT::~QT_RTSP_CLIENT() {
    }

    struct EpochGuard {
        QT_RTSP_CLIENT* self;
        explicit EpochGuard(QT_RTSP_CLIENT* s) : self(s) {
            self->activeEpochs.fetch_add(1, std::memory_order_acq_rel);
        }

        ~EpochGuard() {
        if (self->activeEpochs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (self->stop.load(std::memory_order_acquire)) {
                    self->rstp_worker->Stop();
                    emit self->stopped();
                }
            }
        }
    };

    cv::Mat QT_RTSP_CLIENT::remove_smoke(cv::Mat img) {
        if(img.channels() != 3){
            std::cout<<"Smoke Remover Requires Input Image Has 3 Channels ; Ouput Original Image.\n";
            return img;
        }
        cv::Mat frame_out;
        _smokeRemover->process_one_frame(img, frame_out);
        return frame_out;
    }

    void QT_RTSP_CLIENT::NewRM(int way) {
        _smokeRemover->setRemoverMethods(way);
    }

    void QT_RTSP_CLIENT::NewFun(const int& fun) {
        _smokeRemover->setW2VUpdateFun(fun);
    }

    void QT_RTSP_CLIENT::NewUnit(const int& unit){
        _smokeRemover->setUnit(unit);
    }
    void QT_RTSP_CLIENT::NewK(const int& K){
        _smokeRemover->setK(K);
    }
    void QT_RTSP_CLIENT::NewF(const int& F){
        _smokeRemover->setF(F);
    }
    void QT_RTSP_CLIENT::NewV0(const double& v0){
        _smokeRemover->setV0(v0);
    }
    void QT_RTSP_CLIENT::NewLight(const double& light){
        _smokeRemover->setLight(light);
    }
    void QT_RTSP_CLIENT::NewTbase(const double& Tbase){
        _smokeRemover->setTbase(Tbase);
    }
    void QT_RTSP_CLIENT::NewSIndex(const double& SIndex){
        _smokeRemover->setSIndex(SIndex);
    }
    void QT_RTSP_CLIENT::NewVGamma(const double& VGamma){
        _smokeRemover->setVGamma(VGamma);
    }
    void QT_RTSP_CLIENT::NewVSimu1(const double& VSimu1){
        _smokeRemover->setVSimu1(VSimu1);
    }
    void QT_RTSP_CLIENT::NewVSimu2(const double& VSimu2){
        _smokeRemover->setVSimu2(VSimu2);
    }
    void QT_RTSP_CLIENT::NewVSimu3(const double& VSimu3){
        _smokeRemover->setVSimu3(VSimu3);
    }
    void QT_RTSP_CLIENT::NewFusionG(const double& FusionG){
        _smokeRemover->setFusionG(FusionG);
    }

    void QT_RTSP_CLIENT::NewImage(uint64_t timestamp, cv::Mat Image) {
        if (stop.load()) {
            return;
        }
        EpochGuard guard(this);
        rstp_worker->push_video_frame(Image, timestamp);
        cv::Mat sr = remove_smoke(Image);
        rstp_worker->push_smoke_frame(sr, timestamp);
        emit ImageTime(timestamp);
        emit NewOrigin(Image);
        emit NewSmoke(sr);
    }

    void QT_RTSP_CLIENT::NewAudio(uint64_t timestamp, std::vector<uint8_t> buffer) {
        if (stop.load()) {
            return;
        }
        EpochGuard guard(this);
        rstp_worker->push_audio_frame(buffer, timestamp);
        emit AudioTime(timestamp);
    }

    void QT_RTSP_CLIENT::startWork() {
        if(!rstp_worker->Init()) return;
    }

    void QT_RTSP_CLIENT::stopWork()
    {
        stop.store(true);
        if(activeEpochs == 0){
            rstp_worker->Stop();
            emit stopped();
        } 
    }
}



