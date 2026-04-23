#include "qt_main_window.h"
#include "vis_decoder.h"
#include "hicon_reader.h"
#include "audio_decoder.h"
#include <opencv2/opencv.hpp>
#include <QDebug>

using namespace VIS;
using namespace AUDIO;

namespace QT {
    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        gst_init(nullptr, nullptr);
        DeviceInfo UsbCam = VIS_CODER::list_capture_devices();
        DeviceInfo HiconCam = VIS_HICON_READER::FindHiconCamera();
        Cams.insert(UsbCam.begin(), UsbCam.end());
        Cams.insert(HiconCam.begin(), HiconCam.end());
        Audios = AUDIO_CODER::listFFmpegAudioDevices();
        mainparam = loadParams();
        setupUi();
        setWindowTitle("RTSP推流端");
    }

    MainWindow::~MainWindow() {
        onStop();
    }

    void MainWindow::resetToFactoryDefault()
    {
        QSettings s("HXY", "RTSP");
        s.clear();
        mainparam = loadParams();

        widthEdit->setText(mainparam.width);
        heightEdit->setText(mainparam.height);
        framerateEdit->setText(mainparam.framerate);
        formatCombo->setCurrentText(mainparam.format);
        zipCombo->setCurrentText(mainparam.zip);
        viszipformat->setCurrentText(mainparam.writeformat);
        videopathEdit->setText(mainparam.videopath);

        channelCombo->setCurrentText(mainparam.channels);
        samplerateCombo->setCurrentText(mainparam.samplerate);
        samplesizeCombo->setCurrentText(mainparam.samplesize);
        audiopathEdit->setText(mainparam.audiopath);

        rtspIPEdit->setText(mainparam.rtspurl);
        rtspportEdit->setText(mainparam.port);
        rtspsuffixEdit->setText(mainparam.suffix);
        rtsppathEdit->setText(mainparam.rtsppath);

        rbs1->setChecked(true);
        rbs2->setChecked(false);
        rbr1->setChecked(true);
        rbr2->setChecked(false);
        rbn1->setChecked(true);
        rbn2->setChecked(false);
        origin->setChecked(false);
        self_adjustment->setChecked(false);     
        way1->setChecked(true);
        way2->setChecked(false);
    }

    MainParams MainWindow::loadParams()
    {
        QSettings s("HXY", "RTSP");
        MainParams p;
        p.sensormode        = s.value("sensormode",p.sensormode).toString();
        p.rtspmode          = s.value("rtspmode",p.rtspmode).toString();
        p.framerate         = s.value("framerate",p.framerate).toString();
        p.width             = s.value("width",p.width).toString();
        p.height            = s.value("height",p.height).toString();
        p.srmethod          = s.value("srmethod",p.srmethod).toString();
        p.format            = s.value("format",p.format).toString();
        p.zip               = s.value("zip",p.zip).toString();
        p.videopath         = s.value("videopath",p.videopath).toString();
        p.writeformat       = s.value("writeformat",p.writeformat).toString();
        p.channels          = s.value("channels",p.channels).toString();
        p.samplerate        = s.value("samplerate",p.samplerate).toString();
        p.samplesize        = s.value("samplesize",p.samplesize).toString();
        p.audiopath         = s.value("audiopath",p.audiopath).toString();
        p.rtspurl           = s.value("rtspurl",p.rtspurl).toString();
        p.port              = s.value("port",p.port).toString();
        p.suffix            = s.value("suffix",p.suffix).toString();
        p.rtsppath          = s.value("rtsppath",p.rtsppath).toString();
        p.noisemode          = s.value("noisemode",p.noisemode).toString();


        return p;
    }

    void MainWindow::saveParams(const MainParams& p)
    {
        QSettings s("HXY", "RTSP");
        s.setValue("sensormode",p.sensormode);
        s.setValue("rtspmode",p.rtspmode);
        s.setValue("framerate",p.framerate);
        s.setValue("width",p.width);
        s.setValue("height",p.height);
        s.setValue("srmethod",p.srmethod);
        s.setValue("format",p.format);
        s.setValue("zip",p.zip);
        s.setValue("writeformat",p.writeformat);
        s.setValue("videopath",p.videopath);
        s.setValue("channels",p.channels);
        s.setValue("samplerate",p.samplerate);
        s.setValue("samplesize",p.samplesize);
        s.setValue("audiopath",p.audiopath);
        s.setValue("rtspurl",p.rtspurl);
        s.setValue("port",p.port);
        s.setValue("suffix",p.suffix);
        s.setValue("rtsppath",p.rtsppath);
        s.setValue("noisemode", p.noisemode);

        s.sync();
    }

    void MainWindow::showErrorAndExit(const QString& message) {
        QMessageBox::critical(nullptr, "Fatal Error", message);
        qApp->exit(1);
    }

    void MainWindow::ReadFromUI() {
        SensorMode = (mainparam.sensormode == "0") ? ProcMode::ONLINE : ProcMode::OFFLINE;
        std::string devicename = visdeviceCombo->currentText().toStdString();
        const std::string head = "Industrial Camera";
        if (devicename.compare(0, head.size(), head) == 0) Cset.camtype = BASE::HICON_CAM;
        else Cset.camtype = BASE::USB_CAM;

        Cset.devicepath = Cams[devicename];
        Cset.width = mainparam.width.toInt();
        Cset.height = mainparam.height.toInt();
        Cset.framerate = mainparam.framerate.toInt();
        Cset.zip = mainparam.zip.toStdString();
        Cset.format = mainparam.format.toStdString();
        Cset.filepath= mainparam.videopath.toStdString();

        Aset.devicepath = Audios[audiodeviceCombo->currentText().toStdString()];
        Aset.channels = mainparam.channels.toInt();
        Aset.sample_rate = mainparam.samplerate.toInt();
        Aset.sample_size = mainparam.samplesize.toInt();
        Aset.filepath = mainparam.audiopath.toStdString();
        Aset.noisemode = mainparam.noisemode.toInt();

        RtspMode = (mainparam.rtspmode == "0") ? ProcMode::ONLINE : ProcMode::OFFLINE;
        Rset.IP = mainparam.rtspurl.toStdString();
        Rset.filepath = mainparam.rtsppath.toStdString();
        Rset.port = mainparam.port.toInt();
        Rset.suffix = mainparam.suffix.toStdString();
        Rset.way = mainparam.srmethod.toInt();
        Rset.vformat = mainparam.writeformat.toStdString();


        if (RtspMode == ProcMode::OFFLINE && Rset.filepath == "") showErrorAndExit("You Should Browse the Media Output File if Using File Mode !");
        if (SensorMode == ProcMode::OFFLINE && (Aset.filepath == "" || Cset.filepath == "")) showErrorAndExit("You Should Browse the Video && Audio Input File if Using File Mode !");
    }

    void MainWindow::Init() {
        ReadFromUI();
        ImgReader = std::make_unique<QT_IMG_READER>(Cset, SensorMode);
        AudioReader = std::make_unique<QT_AUDIO_READER>(Aset, SensorMode);
        RtspWorker = std::make_unique<QT_RTSP_CLIENT>(Aset, Cset, Rset, RtspMode);
        paramwindow = std::make_unique<ParamWindow>(this);
        ConnectThreads();
    }

    void MainWindow::ConnectThreads() {
        // VIS
        ImgReader->moveToThread(&ImgReadThread);
        connect(this, &MainWindow::startImageReader,
            ImgReader.get(), &QT_IMG_READER::startWork);
        connect(this, &MainWindow::stopImageReader,
            ImgReader.get(), &QT_IMG_READER::stopWork,
            Qt::QueuedConnection);
        connect(ImgReader.get(), &QT_IMG_READER::stopped,
            this, &MainWindow::onVideoStopped,
            Qt::QueuedConnection);

        // AUDIO
        AudioReader->moveToThread(&AudioReadThread);
        connect(this, &MainWindow::startAudioReader,
            AudioReader.get(), &QT_AUDIO_READER::startWork);
        connect(this, &MainWindow::stopAudioReader,
            AudioReader.get(), &QT_AUDIO_READER::stopWork,
            Qt::QueuedConnection);
        connect(AudioReader.get(), &QT_AUDIO_READER::stopped,
            this, &MainWindow::onAudioStopped,
            Qt::QueuedConnection);
        connect(this, &MainWindow::NewNoise,      
            AudioReader.get(), &QT_AUDIO_READER::onNewNoise,
            Qt::QueuedConnection);

        // RTSP
        RtspWorker->moveToThread(&RtspThread);
        connect(this, &MainWindow::startRtspWorker,
            RtspWorker.get(), &QT_RTSP_CLIENT::startWork);
        connect(this, &MainWindow::stopRtspWorker,
            RtspWorker.get(), &QT_RTSP_CLIENT::stopWork,
            Qt::QueuedConnection);
        connect(RtspWorker.get(), &QT_RTSP_CLIENT::stopped,
            this, &MainWindow::onRtspStopped,
            Qt::QueuedConnection);

        // SEND - ACCEPT
        connect(ImgReader.get(), &QT_IMG_READER::NewImage,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewImage,
            Qt::QueuedConnection);
        connect(AudioReader.get(), &QT_AUDIO_READER::NewAudio,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewAudio,
            Qt::QueuedConnection);

        connect(RtspWorker.get(), &QT_RTSP_CLIENT::ImageTime,
            this, &MainWindow::ImgTime,
            Qt::QueuedConnection);
        connect(RtspWorker.get(), &QT_RTSP_CLIENT::AudioTime,
            this, &MainWindow::AudTime,
            Qt::QueuedConnection);
        connect(RtspWorker.get(), &QT_RTSP_CLIENT::NewOrigin,
            this, &MainWindow::onNewImage1,
            Qt::QueuedConnection);
        connect(RtspWorker.get(), &QT_RTSP_CLIENT::NewSmoke,
            this, &MainWindow::onNewImage2,
            Qt::QueuedConnection);

        connect(this, &MainWindow::NewRM,      
            RtspWorker.get(), &QT_RTSP_CLIENT::NewRM,
            Qt::QueuedConnection);

        //SMOKE PARAM
        connect(paramwindow.get(), &ParamWindow::NewFun,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewFun,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewUnit,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewUnit,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewK,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewK,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewF,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewF,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewV0,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewV0,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewLight,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewLight,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewTbase,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewTbase,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewSIndex,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewSIndex,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewVGamma,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewVGamma,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewVSimu1,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewVSimu1,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewVSimu2,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewVSimu2,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewVSimu3,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewVSimu3,
            Qt::QueuedConnection);
        connect(paramwindow.get(), &ParamWindow::NewFusionG,
            RtspWorker.get(), &QT_RTSP_CLIENT::NewFusionG,
            Qt::QueuedConnection);

        connect(widthEdit.get(), &QLineEdit::editingFinished, this, [=](){
            emit NewWidth(widthEdit->text().toInt());
        });
        connect(this, &MainWindow::NewWidth,
            ImgReader.get(), &QT_IMG_READER::onNewWidth,
            Qt::QueuedConnection);
        connect(heightEdit.get(), &QLineEdit::editingFinished, this, [=](){
            emit NewHeight(heightEdit->text().toInt());
        });
        connect(this, &MainWindow::NewHeight,
            ImgReader.get(), &QT_IMG_READER::onNewHeight,
            Qt::QueuedConnection);
        connect(framerateEdit.get(), &QLineEdit::editingFinished, this, [=](){
            emit NewFrameRate(framerateEdit->text().toInt());
        });
        connect(this, &MainWindow::NewFrameRate,
            ImgReader.get(), &QT_IMG_READER::onNewFrameRate,
            Qt::QueuedConnection);
        connect(this, &MainWindow::NewFormat,
            ImgReader.get(), &QT_IMG_READER::onNewFormat,
            Qt::QueuedConnection);
        connect(formatCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            emit NewFormat(formatCombo->currentText());
        });

        connect(samplerateCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            emit NewSampleRate(samplerateCombo->currentText().toInt());
        });
        connect(this, &MainWindow::NewSampleRate,
            AudioReader.get(), &QT_AUDIO_READER::onNewSampleRate,
            Qt::QueuedConnection);
        connect(samplesizeCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            emit NewSampleSize(samplesizeCombo->currentText().toInt());
        });
        connect(this, &MainWindow::NewSampleSize,
            AudioReader.get(), &QT_AUDIO_READER::onNewSampleSize,
            Qt::QueuedConnection);
        connect(channelCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            emit NewChannels(channelCombo->currentText().toInt());
        });
        connect(this, &MainWindow::NewChannels,
            AudioReader.get(), &QT_AUDIO_READER::onNewChannels,
            Qt::QueuedConnection);
    }

    void MainWindow::onVideoStopped(){
        ImgReadThread.quit(); 
        ImgReadThread.wait();
        VisLabel1->clear();
        VisLabel2->clear();
    }
    void MainWindow::onAudioStopped(){
        AudioReadThread.quit(); AudioReadThread.wait();
    }
    void MainWindow::onRtspStopped(){
        RtspThread.quit(); 
        RtspThread.wait();
        VisTime->setText("0.000(s)");
        AudioTime->setText("0.000(s)");
    }

    void MainWindow::KillThread() {
        emit stopImageReader();
        emit stopAudioReader();
        emit stopRtspWorker();
    }

    void MainWindow::createNavGroupBox()
    {
        navBox = std::make_unique<QGroupBox>("Time(s)");
        auto* lay = new QFormLayout(navBox.get());

        VisTime = std::make_unique<QLabel>("0.000(s)");
        AudioTime = std::make_unique<QLabel>("0.000(s)");

        lay->addRow("视频时间戳:", VisTime.get());
        lay->addRow("音频时间戳:", AudioTime.get());

        navBox->setStyleSheet(
            "QLabel {"
            "   color : red;"
            "   font : bold 14px;"
            "   padding : 4px 0px;"
            "}");
    }

    void MainWindow::setupUi() {
        auto central = new QWidget(this);
        auto mainLayout = new QHBoxLayout(central);
        auto btnGrid = new QGridLayout();

        auto ModeBox = new QGroupBox("模式设置", this);
        auto ModeLayout = new QFormLayout(ModeBox);
        auto videoBox = new QGroupBox("视频设置", this);
        auto videoLayout = new QFormLayout(videoBox);
        auto audioBox = new QGroupBox("音频设置", this);
        auto audioLayout = new QFormLayout(audioBox);
        auto rtspBox = new QGroupBox("推流设置", this);
        auto rtspLayout = new QFormLayout(rtspBox);
        auto visBox_1 = new QGroupBox("原始图像", this);
        auto visLayout1 = new QFormLayout(visBox_1);
        auto visBox_2 = new QGroupBox("去雾后图像", this);
        auto visLayout2 = new QFormLayout(visBox_2);
        auto btnBox = new QGroupBox("控件区", this);

        auto settings = new QVBoxLayout();
        auto btns = new QHBoxLayout();
        auto others = new QVBoxLayout();

        int row = 0, col = 0;
        const int Cols = 1;
        auto addBtn = [&](QPushButton* btn) {
            btnGrid->addWidget(btn, row, col);
            if (++col == Cols) { col = 0; ++row; }
            };
        auto makeBtn = [&](const QString& text) {
            auto btn = std::make_unique<QPushButton>(text, this);
            btn->setStyleSheet(btnStyle1);
            return btn;
            };
        auto makeEdit = [&](const QString& text, QGroupBox* box, QFormLayout* layout, const QString& defaultV = "") {
            auto Edit = std::make_unique<QLineEdit>(box);
            Edit->setText(defaultV);
            layout->addRow(text, Edit.get());
            return Edit;
            };
        auto makeCombo = [&](const QString& text, QGroupBox* box, QFormLayout* layout) {
            auto Combo = std::make_unique<QComboBox>(box);
            layout->addRow(text, Combo.get());
            return Combo;
            };
        auto makeRadio = [&](const QString& text) {
            auto Radio = std::make_unique<QRadioButton>(text);
            return Radio;
            };

        btnStartALL_ = makeBtn("开始推流");
        btnStopALL_ = makeBtn("停止推流");
        btnQuit_ = makeBtn("退出程序");

        addBtn(btnStartALL_.get());
        addBtn(btnStopALL_.get());
        addBtn(btnQuit_.get());

        rbs1 = makeRadio("直播");
        rbs2 = makeRadio("离线");
        if(mainparam.sensormode == "0") rbs1->setChecked(true);
        else rbs2->setChecked(true);
        connect(rbs1.get(), &QRadioButton::toggled, this, &MainWindow::onSensorModeChanged);
        connect(rbs2.get(), &QRadioButton::toggled, this, &MainWindow::onSensorModeChanged);
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->addWidget(rbs1.get()); 
        hLayout->addWidget(rbs2.get());
        ModeLayout->addRow("设备模式: ", hLayout);

        sensorGroup = std::make_unique<QButtonGroup>(ModeLayout->parentWidget());
        sensorGroup->addButton(rbs1.get(), 0);
        sensorGroup->addButton(rbs2.get(), 1);
        sensorGroup->setExclusive(true);

        rbr1 = makeRadio("直播");
        rbr2 = makeRadio("离线");
        if(mainparam.rtspmode == "0") rbr1->setChecked(true);
        else rbr2->setChecked(true);
        connect(rbr1.get(), &QRadioButton::toggled, this, &MainWindow::onRtspModeChanged);
        connect(rbr2.get(), &QRadioButton::toggled, this, &MainWindow::onRtspModeChanged);
        QHBoxLayout* hLayout1 = new QHBoxLayout();
        hLayout1->addWidget(rbr1.get());
        hLayout1->addWidget(rbr2.get());
        ModeLayout->addRow("推流模式: ", hLayout1);

        rtspGroup = std::make_unique<QButtonGroup>(ModeLayout->parentWidget());
        rtspGroup->addButton(rbr1.get(), 0);
        rtspGroup->addButton(rbr2.get(), 1);
        rtspGroup->setExclusive(true);

        ModeBox->setLayout(ModeLayout);
        ModeBox->setFixedHeight(90);
        ModeBox->setFixedWidth(450);
        settings->addWidget(ModeBox);

        visdeviceCombo = makeCombo("设备列表:", videoBox, videoLayout);
        for (auto i = Cams.begin(); i != Cams.end(); i++) {
            visdeviceCombo->addItem(QString::fromStdString(i->first));
        }
        widthEdit = makeEdit("图像宽度:", videoBox, videoLayout, mainparam.width);
        connect(widthEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.width = widthEdit->text();
        });
        heightEdit = makeEdit("图像高度:", videoBox, videoLayout,  mainparam.height);
        connect(heightEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.height = heightEdit->text();
        });
        framerateEdit = makeEdit("视频帧率:", videoBox, videoLayout,  mainparam.framerate);
        connect(framerateEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.framerate = framerateEdit->text();
        });
        zipCombo = makeCombo("图像格式:", videoBox, videoLayout);
        zipCombo->addItems({ "RAW","JPEG"});
        zipCombo->setCurrentText(mainparam.zip);
        connect(zipCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            mainparam.zip = zipCombo->currentText();
        });
        formatCombo = makeCombo("图像色彩:", videoBox, videoLayout);
        formatCombo->addItems({ "GRAY8","BGR","RGB" });
        formatCombo->setCurrentText(mainparam.format);
        connect(formatCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            mainparam.format = formatCombo->currentText();
        });

        origin = makeRadio("原始图像");
        self_adjustment = makeRadio("自适应调整");     
        way1 = makeRadio("去雾方式一");
        way2 = makeRadio("去雾方式二");
        switch(mainparam.srmethod.toInt()){
            case 0: origin->setChecked(true); break;
            case 1: way1->setChecked(true); break;
            case 2: way2->setChecked(true); break;
            case 3: self_adjustment->setChecked(true); break;
            default: way1->setChecked(true); break;
        }
        connect(way1.get(), &QRadioButton::toggled, this, &MainWindow::onMethodChanged);
        connect(way2.get(), &QRadioButton::toggled, this, &MainWindow::onMethodChanged);
        connect(origin.get(), &QRadioButton::toggled, this, &MainWindow::onMethodChanged);
        connect(self_adjustment.get(), &QRadioButton::toggled, this, &MainWindow::onMethodChanged);
     
        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->addWidget(way1.get(), 0, 0); 
        gridLayout->addWidget(way2.get(), 0, 1);
        gridLayout->addWidget(origin.get(), 1, 0);
        gridLayout->addWidget(self_adjustment.get(), 1, 1);

        QWidget* containerWidget = new QWidget();
        containerWidget->setLayout(gridLayout);

        videoLayout->addRow("去雾方式:", containerWidget);
        wayGroup = std::make_unique<QButtonGroup>(videoLayout->parentWidget());
        wayGroup->addButton(way1.get(), 0);
        wayGroup->addButton(way2.get(), 1);
        wayGroup->addButton(origin.get(), 2);
        wayGroup->addButton(self_adjustment.get(), 3);
        wayGroup->setExclusive(true);


        viszipformat = makeCombo("压缩格式:", videoBox, videoLayout);
        viszipformat->addItems({ "RAW","H264","H265"});
        viszipformat->setCurrentText(mainparam.writeformat);
        connect(viszipformat.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            mainparam.writeformat = viszipformat->currentText();
        });
        QPushButton* browseBtn = new QPushButton("浏览", this);
        videopathEdit = std::make_unique<QLineEdit>(videoBox);
        videopathEdit->setText(mainparam.videopath);
        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            QString fileName = QFileDialog::getOpenFileName(
                this,
                "Select Video File",
                "",                   
                "Video Files (*.mp4 *.avi *.mov);;All Files (*)"
            );
            if (!fileName.isEmpty()) {
                mainparam.videopath = fileName;
                mainparam.audiopath = fileName;
                videopathEdit->setText(fileName);
                audiopathEdit->setText(fileName);
            }
            });
        connect(videopathEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.videopath = videopathEdit->text();
            mainparam.audiopath = videopathEdit->text();
            audiopathEdit->setText(videopathEdit->text());
        });
        QHBoxLayout* videopathLayout = new QHBoxLayout();
        videopathLayout->addWidget(videopathEdit.get());
        videopathLayout->addWidget(browseBtn);
        videoLayout->addRow("视频路径:", videopathLayout);

        btnparam = std::make_unique<QPushButton>("参数设置", this);
        btnparam->setStyleSheet(btnStyle4);
        videoLayout->addRow("去雾参数:", btnparam.get());

        videoBox->setLayout(videoLayout);
        videoBox->setFixedHeight(400);
        videoBox->setFixedWidth(450);
        settings->addStretch();
        settings->addWidget(videoBox);

        audiodeviceCombo = makeCombo("设备列表:", audioBox, audioLayout);
        for (auto i = Audios.begin(); i != Audios.end(); i++) {
            audiodeviceCombo->addItem(QString::fromStdString(i->first));
        }
        samplerateCombo = makeCombo("采样率:", audioBox, audioLayout);
        samplerateCombo->addItems({ "48000","44100"});
        samplerateCombo->setCurrentText(mainparam.samplerate);   
        connect(samplerateCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            mainparam.samplerate = samplerateCombo->currentText();
        });

        samplesizeCombo = makeCombo("单位字节:", audioBox, audioLayout);
        samplesizeCombo->addItems({ "1","2","3","4"});
        samplesizeCombo->setCurrentText(mainparam.samplesize);
        connect(samplesizeCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            mainparam.samplesize = samplesizeCombo->currentText();
        });

        channelCombo = makeCombo("音频通道:", audioBox, audioLayout);
        channelCombo->addItems({ "1","2"});
        channelCombo->setCurrentText(mainparam.channels);
        connect(channelCombo.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),this, [=](int index){
            if (index == -1) return;
            mainparam.channels = channelCombo->currentText();
        });

        rbn1 = makeRadio("去噪");
        rbn2 = makeRadio("原始");
        if(mainparam.noisemode == "0") rbn1->setChecked(true);
        else rbn2->setChecked(true);
        connect(rbn1.get(), &QRadioButton::toggled, this, &MainWindow::onNoiseModeChanged);
        connect(rbn2.get(), &QRadioButton::toggled, this, &MainWindow::onNoiseModeChanged);
        QHBoxLayout* noiseLayout = new QHBoxLayout();
        noiseLayout->addWidget(rbn1.get()); 
        noiseLayout->addWidget(rbn2.get());
        audioLayout->addRow("音频去噪: ", noiseLayout);

        noiseGroup = std::make_unique<QButtonGroup>(audioLayout->parentWidget());
        noiseGroup->addButton(rbn1.get(), 0);
        noiseGroup->addButton(rbn2.get(), 1);
        noiseGroup->setExclusive(true);

        QPushButton* browseBtn1 = new QPushButton("浏览", this);
        audiopathEdit = std::make_unique<QLineEdit>(audioBox);
        audiopathEdit->setText(mainparam.audiopath);
        connect(browseBtn1, &QPushButton::clicked, this, [this]() {
            QString fileName = QFileDialog::getOpenFileName(
                this,
                "Select Audio File",
                "",
                "Audio Files (*.mp3 *.mp4 *.wav *.aac *.flac *.ogg);;All Files (*)"
            );
            if (!fileName.isEmpty()) {
                mainparam.audiopath = fileName;
                audiopathEdit->setText(fileName);
            }
            });
        connect(audiopathEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.audiopath = audiopathEdit->text();
        });
        QHBoxLayout* audiopathLayout = new QHBoxLayout();
        audiopathLayout->addWidget(audiopathEdit.get());
        audiopathLayout->addWidget(browseBtn1);
        audioLayout->addRow("音频路径:", audiopathLayout);

        audioBox->setLayout(audioLayout);
        audioBox->setFixedHeight(230);
        audioBox->setFixedWidth(450);
        settings->addStretch();
        settings->addWidget(audioBox);

        rtspIPEdit = makeEdit("直播IP:", rtspBox, rtspLayout, mainparam.rtspurl);
        connect(rtspIPEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.rtspurl = rtspIPEdit->text();
        });
        rtspportEdit = makeEdit("直播端口:", rtspBox, rtspLayout, mainparam.port);
        connect(rtspportEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.port = rtspportEdit->text();
        });
        rtspsuffixEdit = makeEdit("域名后缀:", rtspBox, rtspLayout, mainparam.suffix);
        connect(rtspsuffixEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.suffix = rtspsuffixEdit->text();
        });

        QPushButton* browseBtn2 = new QPushButton("浏览", this);
        rtsppathEdit = std::make_unique<QLineEdit>(rtspBox);
        rtsppathEdit->setText(mainparam.rtsppath);
        connect(browseBtn2, &QPushButton::clicked, this, [this]() {
            QString fileName = QFileDialog::getOpenFileName(
                this,
                "Select Media File",
                "",
                "Media Files (*.mp4 *.mkv *.mov *.avi *.flv);;All Files (*)"
            );
            if (!fileName.isEmpty()) {
                mainparam.rtsppath = fileName;
                rtsppathEdit->setText(fileName);
            }
            });
        connect(rtsppathEdit.get(), &QLineEdit::editingFinished, this, [=](){
            mainparam.rtsppath = rtsppathEdit->text();
        });
        QHBoxLayout* rtsppathLayout = new QHBoxLayout();
        rtsppathLayout->addWidget(rtsppathEdit.get());
        rtsppathLayout->addWidget(browseBtn2);
        rtspLayout->addRow("输出路径:", rtsppathLayout);

        savedefault = std::make_unique<QPushButton>("保存当前设置", this);  
        savedefault->setStyleSheet(btnStyle4);
        rtspLayout->addRow("保存设置:", savedefault.get());

        reset = std::make_unique<QPushButton>("恢复出厂设置", this);  
        reset ->setStyleSheet(btnStyle4);
        rtspLayout->addRow("重置设置:", reset .get());
        
        connect(savedefault.get(), &QPushButton::clicked, this, &MainWindow::onSaveDefault);
        connect(reset.get(), &QPushButton::clicked, this, &MainWindow::onReset);

        auto rtspBoxLayout = new QVBoxLayout(rtspBox);
        rtspBoxLayout->addLayout(rtspLayout);   
        rtspBoxLayout->setAlignment(rtspLayout, Qt::AlignCenter); 
        rtspBox->setLayout(rtspBoxLayout);
        rtspBox->setFixedHeight(250);
        rtspBox->setFixedWidth(450);
        settings->addStretch();
        settings->addWidget(rtspBox);


        VisLabel1 = std::make_unique<QLabel>();
        VisLabel1->setAlignment(Qt::AlignCenter);
        VisLabel1->setText("图像1");
        VisLabel1->setFixedWidth(592);
        VisLabel1->setFixedHeight(333);
        visLayout1->addWidget(VisLabel1.get());
        visLayout1->setAlignment(VisLabel1.get(), Qt::AlignCenter);
        visBox_1->setLayout(visLayout1);

        VisLabel2 = std::make_unique<QLabel>();
        VisLabel2->setAlignment(Qt::AlignCenter);
        VisLabel2->setText("图像2");
        VisLabel2->setFixedWidth(592);
        VisLabel2->setFixedHeight(333);
        visLayout2->addWidget(VisLabel2.get());
        visLayout2->setAlignment(VisLabel2.get(), Qt::AlignCenter);
        visBox_2->setLayout(visLayout2);

        others->addWidget(visBox_1);
        others->addStretch();
        others->addWidget(visBox_2);

        createNavGroupBox();
        navBox->setFixedHeight(120);
        navBox->setFixedWidth(200);
        btns->addWidget(navBox.get());

        auto btnBar = new QWidget;
        btnBar->setLayout(btnGrid);
        btnBar->setFixedHeight(200);
        btnBar->setFixedWidth(200);
        btns->addWidget(btnBar);
        btnBox->setLayout(btns);
        others->addStretch();
        others->addWidget(btnBox);

        mainLayout->addLayout(settings);
        mainLayout->addStretch();
        mainLayout->addLayout(others);

        setCentralWidget(central);
        this->resize(1080, 900);

        connect(btnparam.get(), &QPushButton::clicked, this, &MainWindow::onparam);
        connect(btnStartALL_.get(), &QPushButton::clicked, this, &MainWindow::onStart);
        connect(btnStopALL_.get(), &QPushButton::clicked, this, &MainWindow::onStop);
        connect(btnQuit_.get(), &QPushButton::clicked, this, &MainWindow::onQuit);

        btnStartALL_->setEnabled(true);
        btnStopALL_->setEnabled(false);

        paramwindow = std::make_unique<ParamWindow>(this);
    }

    void MainWindow::onparam(){
        if (!paramwindow) {
           paramwindow = std::make_unique<ParamWindow>(this);
        }
        paramwindow->show();
        paramwindow->raise();
    }

    void MainWindow::onStart() {
        Init();
        ProjectStartTime = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
        if (!ImgReader || !AudioReader || !RtspWorker) {
            qCritical() << "Init Failed";
            return;
        };
        btnStartALL_->setStyleSheet(btnStyle2);
        RtspThread.start();
        ImgReadThread.start();
        AudioReadThread.start();
        emit startImageReader();
        emit startAudioReader();
        emit startRtspWorker();
        btnStartALL_->setEnabled(false);
        btnStopALL_->setEnabled(true);
    }

    void MainWindow::onStop() {
        btnStartALL_->setStyleSheet(btnStyle1);
        btnStartALL_->setEnabled(true);
        btnStopALL_->setEnabled(false);
        if(paramwindow->isVisible()) paramwindow->hide();
        KillThread();
    }

    void MainWindow::onQuit() {
        onStop();
        close();
    }

    void MainWindow::onNewImage1(const cv::Mat& frame) {
        if (frame.empty()) return;
        QImage img;
        if (mainparam.format == "RGB") img = QImage((const uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888).copy();
        else if (mainparam.format == "BGR") {
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            img = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
        }
        else if (mainparam.format == "GRAY8") img = QImage((const uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_Grayscale8).copy();
        QPixmap pix = QPixmap::fromImage(img);

        pix = pix.scaled(VisLabel1->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);

        VisLabel1->setPixmap(pix);
    }

    void MainWindow::onNewImage2(const cv::Mat& frame) {
        if (frame.empty()) return;
        QImage img;
        if (mainparam.format == "RGB") img = QImage((const uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888).copy();
        else if (mainparam.format == "BGR") {
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            img = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
        }
        else if (mainparam.format == "GRAY8") img = QImage((const uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_Grayscale8).copy();
        QPixmap pix = QPixmap::fromImage(img);

        pix = pix.scaled(VisLabel2->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
        
        VisLabel2->setPixmap(pix);
    }
}
