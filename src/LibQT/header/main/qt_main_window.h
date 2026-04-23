#ifndef QT_MAIN_WINDOW_H
#define QT_MAIN_WINDOW_H
#include <QMainWindow>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QTimer>
#include <QProcess>
#include <QDir>
#include <QApplication>
#include <QMetaObject>
#include <QScrollArea>
#include <QMessageBox>
#include <QLibrary>
#include <QDateTime>
#include <QString>
#include <QLineEdit>
#include <QStringList>
#include <QIntValidator>
#include <QSettings>
#include <QComboBox>
#include <QButtonGroup>
#include <QRadioButton>
#include "base_global.h"
#include "qt_image_reader.h"
#include "qt_audio_reader.h"
#include "qt_rtsp_client.h"
#include "qt_param_window.h"

using namespace BASE;
using namespace std::chrono;

namespace QT {
    struct MainParams {
        QString sensormode   = "0";
        QString rtspmode     = "0";
        QString framerate    = "30";
        QString width        = "1920";
        QString height       = "1080";
        QString format       = "RGB";
        QString zip          = "RAW";
        QString srmethod     = "1";
        QString writeformat  = "H264";
        QString videopath    = "";
        QString channels     = "2";
        QString samplerate   = "48000";
        QString samplesize   = "2";
        QString audiopath    = "";
        QString rtspurl      = "192.168.31.104";
        QString port         = "8554";
        QString suffix       = "live";
        QString rtsppath     = "";
        QString noisemode    = "0";
    };
    using DeviceInfo = std::map<std::string, std::string>;

    class MainWindow : public QMainWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow();

    signals:
        void startImageReader();
        void stopImageReader();
        void startAudioReader();
        void stopAudioReader();
        void startRtspWorker();
        void stopRtspWorker();
        void NewRM(int way);
        void NewWidth(int width);
        void NewHeight(int height);
        void NewFrameRate(int fr);
        void NewFormat(const QString format);
        void NewSampleRate(int sr);
        void NewSampleSize(int sz);
        void NewChannels(int channels);
        void NewNoise(int noise);

    private slots:
        void onVideoStopped();
        void onAudioStopped();
        void onRtspStopped();
        void onparam();
        void onStart();
        void onStop();
        void onQuit();
        void ImgTime(uint64_t timestamp) {
            VisTime->setText(QString::number(double(timestamp / 10e8), 'f', 3) + "(s)");
        };
        void AudTime(uint64_t timestamp) {
            AudioTime->setText(QString::number(double(timestamp / 10e8), 'f', 3) + "(s)");
        }
        void onNewImage1(const cv::Mat& img);
        void onNewImage2(const cv::Mat& img);
        void onMethodChanged(bool checked) {
            QRadioButton* button = qobject_cast<QRadioButton*>(sender());
            if (button && checked) {
                QString methodName = button->text();
                int way = 0;
                if (methodName.toStdString() == "去雾方式一") way = 1;
                else if (methodName.toStdString() == "去雾方式二") way = 2;
                else if (methodName.toStdString() == "原始图像") way = 0;
                else if (methodName.toStdString() == "自适应调整") way = 3;
                mainparam.srmethod = QString::number(way);
                emit NewRM(way);
            }
        }
        void onNoiseModeChanged(bool checked) {
            QRadioButton* button = qobject_cast<QRadioButton*>(sender());
            if (button && checked) {
                QString Name = button->text();
                if(Name == "去噪") mainparam.noisemode = "0";
                else if(Name == "原始") mainparam.noisemode = "1";
                emit NewNoise(mainparam.noisemode.toInt());
            }
        }
        void onRtspModeChanged(bool checked) {
            QRadioButton* button = qobject_cast<QRadioButton*>(sender());
            if (button && checked) {
                QString Name = button->text();
                if(Name == "直播") mainparam.rtspmode = "0";
                else if(Name == "离线") mainparam.rtspmode = "1";
            }
        }
        void onSensorModeChanged(bool checked) {
            QRadioButton* button = qobject_cast<QRadioButton*>(sender());
            if (button && checked) {
                QString Name = button->text();
                if(Name == "直播") mainparam.sensormode = "0";
                else if(Name == "离线") mainparam.sensormode = "1";
            }
        }
        void onSaveDefault(){
            saveParams(mainparam);
        };
        void onReset(){
            resetToFactoryDefault();
        };

    private:
        void Init();
        void showErrorAndExit(const QString& message);
        void ReadFromUI();
        void setupUi();
        void ConnectThreads();
        void createNavGroupBox();
        void KillThread();
        MainParams loadParams();
        void saveParams(const MainParams& p);
        void resetToFactoryDefault();

    private:
        DeviceInfo Cams;
        DeviceInfo Audios;
        ProcMode SensorMode;
        ProcMode RtspMode;
        AudioInfo Aset;
        CamInfo Cset;
        RtspInfo Rset;
        std::string rtsppath;
        MainParams mainparam;

        std::unique_ptr<ParamWindow> paramwindow;
        std::unique_ptr<QPushButton> btnparam{}; 
        std::unique_ptr<QPushButton> btnStartALL_{};
        std::unique_ptr<QPushButton> btnStopALL_{};
        std::unique_ptr<QPushButton> btnQuit_{};
        std::unique_ptr<QGroupBox> navBox{};
        std::unique_ptr<QLabel> VisTime{};
        std::unique_ptr<QLabel> AudioTime{};
        std::unique_ptr<QLabel> VisLabel1{};
        std::unique_ptr<QLabel> VisLabel2{};

        std::unique_ptr<QButtonGroup> sensorGroup;
        std::unique_ptr<QRadioButton> rbs1;
        std::unique_ptr<QRadioButton> rbs2;
        std::unique_ptr<QComboBox> visdeviceCombo;
        std::unique_ptr<QLineEdit> widthEdit;
        std::unique_ptr<QLineEdit> heightEdit;
        std::unique_ptr<QLineEdit> framerateEdit;
        std::unique_ptr<QComboBox> formatCombo;
        std::unique_ptr<QComboBox> zipCombo;
        std::unique_ptr<QButtonGroup> wayGroup;
        std::unique_ptr<QRadioButton> origin;
        std::unique_ptr<QRadioButton> self_adjustment;
        std::unique_ptr<QRadioButton> way1;
        std::unique_ptr<QRadioButton> way2;
        std::unique_ptr<QComboBox> viszipformat;
        std::unique_ptr<QLineEdit> videopathEdit;

        std::unique_ptr<QComboBox> audiodeviceCombo;
        std::unique_ptr<QComboBox> channelCombo;
        std::unique_ptr<QComboBox> samplerateCombo;
        std::unique_ptr<QComboBox> samplesizeCombo;
        std::unique_ptr<QLineEdit> audiopathEdit;
        std::unique_ptr<QButtonGroup> noiseGroup;
        std::unique_ptr<QRadioButton> rbn1;
        std::unique_ptr<QRadioButton> rbn2;

        std::unique_ptr<QButtonGroup> rtspGroup;
        std::unique_ptr<QRadioButton> rbr1;
        std::unique_ptr<QRadioButton> rbr2;
        std::unique_ptr<QLineEdit> rtspIPEdit;
        std::unique_ptr<QLineEdit> rtsppathEdit;
        std::unique_ptr<QLineEdit> rtspportEdit;
        std::unique_ptr<QLineEdit> rtspsuffixEdit;
        std::unique_ptr<QPushButton> savedefault;
        std::unique_ptr<QPushButton> reset;

        QThread ImgReadThread;
        std::unique_ptr<QT_IMG_READER> ImgReader;
        QThread AudioReadThread;
        std::unique_ptr<QT_AUDIO_READER> AudioReader;
        QThread RtspThread;
        std::unique_ptr<QT_RTSP_CLIENT> RtspWorker;

        bool m_init = false;
        const QString btnStyle1 =
            "QPushButton{"
            "  width:180px; height:40px;"
            "  font: bold 14px \"Microsoft YaHei\";"
            "  margin:6px; padding:10px;"
            "  background-color:#0078D4;"
            "  color:#FFFFFF;"
            "  border:none;"
            "  border-radius:4px;"
            "}"
            "QPushButton:hover{"
            "  background-color:#106EBE;"
            "}"
            "QPushButton:pressed{"
            "  background-color:#005A9E;"
            "}";

        const QString btnStyle2 =
            "QPushButton{"
            "  width:180px; height:40px;"
            "  font: bold 14px \"Microsoft YaHei\";"
            "  margin:6px; padding:10px;"
            "  background-color:#FF5722;"
            "  color:#FFFFFF;"
            "  border:none;"
            "  border-radius:4px;"
            "}"
            "QPushButton:hover{"
            "  background-color:#106EBE;"
            "}"
            "QPushButton:pressed{"
            "  background-color:#005A9E;"
            "}";

        const QString btnStyle3 =
            "QPushButton{"
            "  width:180px; height:40px;"
            "  font: bold 14px \"Microsoft YaHei\";"
            "  margin:6px; padding:10px;"
            "  background-color:#4CAF50;"
            "  color:#FFFFFF;"
            "  border:none;"
            "  border-radius:4px;"
            "}"
            "QPushButton:hover{"
            "  background-color:#388E3C;"
            "}"
            "QPushButton:pressed{"
            "  background-color:#2E7D32;"
            "}";
        
        const QString btnStyle4 =
            "QPushButton{"
            "  width:80px; height:20px;"
            "  font: bold 12px \"Microsoft YaHei\";"
            "  margin:3px; padding:5px;"
            "  background-color:#4CAF50;"
            "  color:#FFFFFF;"
            "  border:none;"
            "  border-radius:4px;"
            "}"
            "QPushButton:hover{"
            "  background-color:#388E3C;"
            "}"
            "QPushButton:pressed{"
            "  background-color:#2E7D32;"
            "}";
    };
}

#endif