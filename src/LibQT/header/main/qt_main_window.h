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
    /**
	*@简介  存储项目参数的结构体
    */
    struct MainParams {
        QString sensormode   = "0";
        QString rtspmode     = "0";
        QString streamchannel  = "音视频";
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
        QString savepath     = "";
    };
    using DeviceInfo = std::map<std::string, std::string>;

    /**
	*@简介  项目主窗口类，继承自QMainWindow，负责管理UI和信号槽连接
    */
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
        void startPick();
        void stopPick();
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
        void onStartPick();
        void onStopPick();
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
                // else if (methodName.toStdString() == "自适应调整") way = 3;
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
        /**
		*@简介  初始化函数，负责加载参数、设置UI和连接线程
        */
        void Init();

        /**
        *@简介  显示错误信息并退出程序
        */
        void showErrorAndExit(const QString& message);

        /**
		*@简介  从UI读取参数并更新mainparam结构体
        */

        void ReadFromUI();

        /**
		*@简介  设置UI，包括创建按钮、组合框、编辑框等，并将它们添加到布局中
        */
        void setupUi();

        /**
		*@简介  连接线程，包括图像读取线程、音频读取线程和RTSP工作线程
        */
        void ConnectThreads();

        /**
		*@简介  创建导航组框，包括开始、停止、录制和退出按钮
        */
        void createNavGroupBox();

        /**
		*@简介  杀死线程，确保所有线程在退出前被正确终止
        */
        void KillThread();

        /**
		*@简介  从配置文件中加载参数，如果配置文件不存在或读取失败，则使用默认参数
        */
        MainParams loadParams();

        /**
		*@简介  保存参数到配置文件中，以便下次启动时加载
        */
        void saveParams(const MainParams& p);

        /**
		*@简介  将参数重置为出厂默认值，并更新UI和配置文件
        */
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
        std::unique_ptr<QPushButton> btnStartPick_{};
        std::unique_ptr<QPushButton> btnStopPick_{};
        std::unique_ptr<QPushButton> btnQuit_{};
        std::unique_ptr<QGroupBox> navBox{};
        std::unique_ptr<QLabel> VisTime{};
        std::unique_ptr<QLabel> AudioTime{};
        std::unique_ptr<QLabel> VisLabel1{};
        std::unique_ptr<QLabel> VisLabel2{};

        std::unique_ptr<QComboBox> streamchannelCombo;

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
        std::unique_ptr<QLineEdit> savepathEdit;
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