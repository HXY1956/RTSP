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