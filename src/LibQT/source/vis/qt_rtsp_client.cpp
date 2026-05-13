#include "qt_rtsp_client.h"
#undef signals
#include "rtsp_file.h"
#include "rtsp_live.h"
#define signals protected
#include "qt_file_saver.h"

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
        m_savePath = QString::fromStdString(Rset.savepath);
        if (!m_savePath.isEmpty()) {
            FILE_SAVER::ensureDirectoryExists(m_savePath.toStdString());
            FILE_SAVER::ensureDirectoryExists((m_savePath + "/remove_smoke").toStdString());
            FILE_SAVER::ensureDirectoryExists((m_savePath + "/origin").toStdString());
        }
        fileSaver1 = new FILE_SAVER(Aset, Cset, Rset, this);
        fileSaver2 = new FILE_SAVER(Aset, Cset, Rset, this);
    }

    QT_RTSP_CLIENT::~QT_RTSP_CLIENT() {
        if (fileSaver1) {
            fileSaver1->stopThread();
            delete fileSaver1;
            fileSaver1 = nullptr;
        }
        if (fileSaver2) {
            fileSaver2->stopThread();
            delete fileSaver2;
            fileSaver2 = nullptr;
        }
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
                    self->notifyStopped();
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

    void QT_RTSP_CLIENT::startPick(){
        m_isPick = true;
        m_isFirstPick = true;
        first_pick_pts = UINT64_MAX;
        fileSaver1->startRecord();
        fileSaver2->startRecord();
    }
    void QT_RTSP_CLIENT::stopPick(){
        m_isPick = false;
		fileSaver1->stopRecord();
        fileSaver2->stopRecord();
        m_isFirstPick = false;
    }

    void QT_RTSP_CLIENT::NewImage(uint64_t timestamp, cv::Mat Image) {
        if (stop.load()) {
            return;
        }
        EpochGuard guard(this);
        rstp_worker->push_video_frame(Image, timestamp);
        cv::Mat sr = remove_smoke(Image);
        rstp_worker->push_smoke_frame(sr, timestamp);

        if (m_isPick.load() && !m_savePath.isEmpty()) {
            if (m_isFirstPick) {
                QDateTime now = QDateTime::currentDateTime();
                QString fileName = QString("video_%1.mp4")
                    .arg(now.toString("yyyy_MM_dd_hh-mm-ss"));
                QString fullPath1 = QString("%1/%2")
                    .arg(m_savePath + "/origin")
                    .arg(fileName);
                QString fullPath2 = QString("%1/%2")
                    .arg(m_savePath + "/remove_smoke")
                    .arg(fileName);
                fileSaver1->setfilepath(fullPath1.toStdString());
                fileSaver2->setfilepath(fullPath2.toStdString());
                fileSaver1->setStreamfirstpts(timestamp);
                fileSaver2->setStreamfirstpts(timestamp);
                first_pick_pts = timestamp;
                m_isFirstPick = false;
            }

            if (first_pick_pts != UINT64_MAX && timestamp >= first_pick_pts) {
                fileSaver1->push_video_frame(Image, timestamp);
                fileSaver2->push_video_frame(sr, timestamp);
            }
        }

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

        if (m_isPick.load() && !m_savePath.isEmpty()) {
            if (first_pick_pts != UINT64_MAX && timestamp >= first_pick_pts) {
                fileSaver1->push_audio_frame(buffer, timestamp);
                fileSaver2->push_audio_frame(buffer, timestamp);
            }
        }
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
            if (fileSaver1) {
                fileSaver1->stopThread();
                delete fileSaver1;
                fileSaver1 = nullptr;
            }
            if (fileSaver2) {
                fileSaver2->stopThread();
                delete fileSaver2;
                fileSaver2 = nullptr;
            }
            emit stopped();
        } 
    }
}



