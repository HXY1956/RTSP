#ifndef QT_RTSP_CLIENT_H
#define QT_RTSP_CLIENT_H
#include <QObject>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QLabel>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QDebug>
#include <QWaitCondition>
#include <QDateTime>
#include <QDir>
#include "base_global.h"
#include "qt_global.h"
#include "gpu_algo_krl.h"

using namespace BASE;
using namespace SMOKE;
namespace RTSP {
    class RTSP_BASE;
}
namespace QT{
    class FILE_SAVER;
}

namespace QT {
    class QT_RTSP_CLIENT : public QObject {
        Q_OBJECT
    public slots:
        void startWork();
        void stopWork();
        void startPick();
        void stopPick();
        void NewImage(uint64_t timestamp, cv::Mat Image);
        void NewAudio(uint64_t timestamp, std::vector<uint8_t> buffer);
        void NewRM(int way);
        void NewUnit(const int& unit);
        void NewK(const int& K);
        void NewF(const int& F);
        void NewV0(const double& v0);
        void NewLight(const double& light);
        void NewTbase(const double& Tbase);
        void NewSIndex(const double& SIndex);
        void NewVGamma(const double& VGamma);
        void NewVSimu1(const double& VSimu1);
        void NewVSimu2(const double& VSimu2);
        void NewVSimu3(const double& VSimu3);
        void NewFusionG(const double& FusionG);
        void NewFun(const int& fun);
        
    signals:
        void ImageTime(uint64_t timestamp);
        void AudioTime(uint64_t timestamp);
        void NewOrigin(const cv::Mat& img);
        void NewSmoke(const cv::Mat& img);
        void stopped();


    public:
        QT_RTSP_CLIENT(AudioInfo Aset, CamInfo Cset, RtspInfo Rset, ProcMode mode, QObject * parent = nullptr);
        ~QT_RTSP_CLIENT() override;
        void notifyStopped()
        {
            emit stopped();
        }
        std::atomic<bool> stop{ false };
        std::atomic<int> activeEpochs{0};
        std::unique_ptr<RTSP::RTSP_BASE> rstp_worker;

    private:
        cv::Mat remove_smoke(cv::Mat img);
        std::unique_ptr<CudaSmokeRemover> _smokeRemover;
        std::map<uint64_t, cv::Mat> image_storage;
        std::map<uint64_t, std::vector<uint8_t>> audio_storage;

        std::atomic_bool m_isPick{ false };
        std::atomic_bool m_isFirstPick{ false };
        uint64_t first_pick_pts = UINT64_MAX;
        QString m_savePath = "";
        FILE_SAVER* fileSaver1 = nullptr;
        FILE_SAVER* fileSaver2 = nullptr;
    };
}


#endif