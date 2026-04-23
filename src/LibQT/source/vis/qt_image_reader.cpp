#include "qt_image_reader.h"
#undef signals
#include "vis_decoder.h"
#include "vis_streamer.h"
#include "hicon_reader.h"
#define signals protected

using namespace VIS;

namespace QT {
    QT_IMG_READER::QT_IMG_READER(CamInfo set, ProcMode mode, QObject* parent)
        : QObject(parent), stop(false){
        if (mode == ProcMode::ONLINE) {
            if(set.camtype == BASE::HICON_CAM) coder = std::make_unique<VIS_HICON_READER>(set);
            else if(set.camtype == BASE::USB_CAM) coder = std::make_unique<VIS_STREAMER>(set);
        }
        else if (mode == ProcMode::OFFLINE) {
            coder = std::make_unique<VIS_DECODER>(set);
        }
    }

    QT_IMG_READER::~QT_IMG_READER() {
    }

    void QT_IMG_READER::onNewWidth(int width){
        coder->setwidth(width);
    }
    void QT_IMG_READER::onNewHeight(int height){
        coder->setheight(height);
    }
    void QT_IMG_READER::onNewFrameRate(int fr){
        coder->setframerate(fr);
    }
    void QT_IMG_READER::onNewFormat(const QString format){
        coder->setformat(format.toStdString());
    }

    void QT_IMG_READER::OneEpoch() {
        if (stop.load()) {
            return;
        }
        active.store(true);
        uint64_t timestamp;
        cv::Mat frame;
        if(!coder->Capture(timestamp, frame)) {
            stop.store(true);
            coder->Stop();
            emit stopped();
            return;
        }
        emit NewImage(timestamp, frame.clone());
        if (stop.load()) {
            coder->Stop();
            emit stopped();
        }
        active.store(false);
    }

    void QT_IMG_READER::startWork() {
        if (!coder->Init()) {
            std::cerr << "Open Video Stream Failed or No Video Stream" << std::endl;
            Status = false;
            return;
        };
        cycleTimer = new QTimer(this);
        connect(cycleTimer, &QTimer::timeout, this, &QT_IMG_READER::OneEpoch);
        cycleTimer->start(5);
    }

    void QT_IMG_READER::stopWork()
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

