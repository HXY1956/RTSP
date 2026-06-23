#ifndef QT_IMAGE_READER_H
#define QT_IMAGE_READER_H
#include <QObject>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include "qt_global.h"
#include "base_global.h"

using namespace BASE;

namespace VIS {
    class VIS_CODER;
}

namespace QT {
    /**
	*@简介  视频解码器类，继承自QObject，整合并提供对视频流的读取、解码和处理功能。它使用VIS_CODER进行视频解码，并通过信号与槽机制与其他组件进行通信。
    */
    class QT_IMG_READER : public QObject {
        Q_OBJECT
    public slots:
        void startWork();
        void stopWork();
        void onNewWidth(int width);
        void onNewHeight(int height);
        void onNewFrameRate(int fr);
        void onNewFormat(const QString format);
    signals:
        void NewImage(uint64_t timestamp, cv::Mat img);
        void stopped();

    public:
        QT_IMG_READER(CamInfo Cset, ProcMode mode, QObject* parent = nullptr);
        ~QT_IMG_READER() override;
        void _stop() { stop = true; }
    
    protected:
        /**
		*@简介  单个周期的视频解码处理函数，负责从视频流中解码一帧图像数据，并将其通过信号发送出去
        */
        void OneEpoch();

    private:
        bool Status = true;
        std::atomic<bool> stop{ false };
        QTimer* cycleTimer = nullptr;
        std::unique_ptr<VIS::VIS_CODER> coder;
        std::atomic<bool> active{ false };
    };
}

#endif