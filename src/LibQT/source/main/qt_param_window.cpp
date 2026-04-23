#include "qt_param_window.h"
#include <QDebug>

namespace QT {
    ParamWindow::ParamWindow(QWidget* parent) : QMainWindow(parent)
    {
        smokeparam = loadParams();
        setupUi();
        ReadFromUI(smokeparam);
        ConnectThreads();
        setWindowTitle("Smoke Removal Parameters");
    }

    void ParamWindow::resetToFactoryDefault()
    {
        QSettings s("HXY", "PARAM");
        s.clear();
        smokeparam = loadParams();

        UnitEdit->setValue(smokeparam.unitSize);
        KEdit->setValue(smokeparam.debugSegmentNum);
        FEdit->setValue(smokeparam.switchFreq);
        V0Edit->setValue(smokeparam.v0);
        LightEdit->setValue(smokeparam.light);
        TbaseEdit->setValue(smokeparam.tbase);
        SIndexEdit->setValue(smokeparam.sIndex);
        FusionGEdit->setValue(smokeparam.fusionG);
        VGammaEdit->setValue(smokeparam.gamma);
        VSimu1Edit->setValue(smokeparam.simu1);
        VSimu2Edit->setValue(smokeparam.simu2);
        VSimu3Edit->setValue(smokeparam.simu3);

        if(smokeparam.function == 0){
        fun1->setChecked(true);
        fun2->setChecked(false);
        }
        else{
        fun1->setChecked(false);
        fun2->setChecked(true);
        }
    }

    SmokeParams ParamWindow::loadParams()
    {
        QSettings s("HXY", "PARAM");
        SmokeParams p;

        p.unitSize        = s.value("unitSize",        p.unitSize).toInt();
        p.debugSegmentNum = s.value("debugSegmentNum", p.debugSegmentNum).toInt();
        p.switchFreq      = s.value("switchFreq",      p.switchFreq).toInt();

        p.v0      = s.value("v0",      p.v0).toDouble();
        p.light   = s.value("light",   p.light).toDouble();
        p.tbase   = s.value("tbase",   p.tbase).toDouble();
        p.sIndex  = s.value("sIndex",  p.sIndex).toDouble();
        p.fusionG = s.value("fusionG", p.fusionG).toDouble();
        p.gamma   = s.value("gamma",   p.gamma).toDouble();
        p.simu1   = s.value("simu1",   p.simu1).toDouble();
        p.simu2   = s.value("simu2",   p.simu2).toDouble();
        p.simu3   = s.value("simu3",   p.simu3).toDouble();
        p.function = s.value("function",   p.function).toInt();
        return p;
    }

    void ParamWindow::saveParams(const SmokeParams& p)
    {
        QSettings s("HXY", "PARAM");

        s.setValue("unitSize",        p.unitSize);
        s.setValue("debugSegmentNum", p.debugSegmentNum);
        s.setValue("switchFreq",      p.switchFreq);
        s.setValue("v0",      p.v0);
        s.setValue("light",   p.light);
        s.setValue("tbase",   p.tbase);
        s.setValue("sIndex",  p.sIndex);
        s.setValue("fusionG", p.fusionG);
        s.setValue("gamma",   p.gamma);
        s.setValue("simu1",   p.simu1);
        s.setValue("simu2",   p.simu2);
        s.setValue("simu3",   p.simu3);
        s.setValue("function", p.function);

        s.sync();
    }

    void ParamWindow::ReadFromUI(const SmokeParams& p) {
        emit NewUnit(p.unitSize);
        emit NewK(p.debugSegmentNum);
        emit NewF(p.switchFreq);
        emit NewV0(p.v0);
        emit NewLight(p.light);
        emit NewTbase(p.tbase);
        emit NewSIndex(p.sIndex);
        emit NewVGamma(p.gamma);
        emit NewVSimu1(p.simu1);
        emit NewVSimu2(p.simu2);
        emit NewVSimu3(p.simu3);
        emit NewFusionG(p.fusionG);
        if(FunGroup->checkedId() >= 0){
            emit NewFun(FunGroup->checkedId());
        }
        else{
            emit NewFun(0);
        }
        
    }

    void ParamWindow::ConnectThreads() {
        connect(UnitEdit.get(), QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ParamWindow::onUnitChange);
        connect(KEdit.get(), QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ParamWindow::onKChange);
        connect(FEdit.get(), QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ParamWindow::onFChange);
        connect(V0Edit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onV0Change);
        connect(LightEdit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onLightChange);
        connect(TbaseEdit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onTbaseChange);
        connect(SIndexEdit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onSIndexChange);
        connect(FusionGEdit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onFusionGChange);
        connect(VGammaEdit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onVGammaChange);
        connect(VSimu1Edit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onVSimu1Change);
        connect(VSimu2Edit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onVSimu2Change);
        connect(VSimu3Edit.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParamWindow::onVSimu3Change);
        connect(fun1.get(), &QRadioButton::toggled, this, &ParamWindow::onFunChanged);
        connect(fun2.get(), &QRadioButton::toggled, this, &ParamWindow::onFunChanged);
        
    }

    void ParamWindow::setupUi() {
        QWidget* centralWidget = new QWidget(this);
        auto MainLayout = new QFormLayout(centralWidget);

        auto makeDoubleSpin = [&](QFormLayout* layout, const QString& text, const double startvalue, double dval = 0.0, 
            double uval = 1.0, int Decimal = 3, double singlestep = 0.01) {
            auto spin = std::make_unique<QDoubleSpinBox>(this);
            spin->setRange(dval, uval);
            spin->setDecimals(Decimal);
            spin->setSingleStep(singlestep);
            spin->setValue(startvalue);
            spin->setKeyboardTracking(false);
            spin->setAccelerated(true);
            layout->addRow(text, spin.get());
            return spin;
        };
        auto makeSpin = [&](QFormLayout* layout, const QString& text, const int startvalue, int dval = 0, 
            int uval = 100) {
            auto spin = std::make_unique<QSpinBox>(this);
            spin->setRange(dval, uval);
            spin->setValue(startvalue);
            spin->setKeyboardTracking(false);
            spin->setAccelerated(true);
            layout->addRow(text, spin.get());
            return spin;
            };
        auto makeRadio = [&](const QString& text) {
            auto Radio = std::make_unique<QRadioButton>(text);
            return Radio;
            };

        UnitEdit = makeSpin(MainLayout,"单元大小(Unit);调节范围:[17,1000] :", smokeparam.unitSize, 17, 1000);
        KEdit = makeSpin(MainLayout,"调试片段数(k);调节范围:[1,100]:", smokeparam.debugSegmentNum, 1, 100);
        FEdit = makeSpin(MainLayout,"切换频率(f);调节范围:[1,100]:", smokeparam.switchFreq, 1, 100);
        V0Edit = makeDoubleSpin(MainLayout,"视觉浓度阈值(v0);调节范围:[0,1]:", smokeparam.v0, 0.0, 1.0, 3, 0.002);
        LightEdit = makeDoubleSpin(MainLayout,"L系数(Light);调节范围:[1,100]:", smokeparam.light, 1.0, 100.0, 3, 0.1);
        TbaseEdit = makeDoubleSpin(MainLayout,"最小透过率(t_base);调节范围:[0,1]:", smokeparam.tbase, 0.0, 1.0, 3, 0.002);
        SIndexEdit = makeDoubleSpin(MainLayout,"S分量调节(s_index);调节范围:[0,1]:", smokeparam.sIndex, 0.0, 1.0, 3, 0.002);
        FusionGEdit = makeDoubleSpin(MainLayout,"细节强度(fusion_G);调节范围:[1,10]:", smokeparam.fusionG, 1.0, 10.0, 3, 0.1);
        VGammaEdit = makeDoubleSpin(MainLayout,"Gamma系数(v_gamma);调节范围:[0,1]:", smokeparam.gamma, 0.0, 1000.0, 3, 0.05);
        VSimu1Edit = makeDoubleSpin(MainLayout,"Fitting系数1(v_simu1);调节范围:[0,1]:", smokeparam.simu1, 0.0, 1.0, 3, 0.001);
        VSimu2Edit = makeDoubleSpin(MainLayout,"Fitting系数2(v_simu2);调节范围:[0,1]:", smokeparam.simu2, 0.0, 1.0, 3, 0.001);
        VSimu3Edit = makeDoubleSpin(MainLayout,"Fitting系数3(v_simu3);调节范围:[0,1]:", smokeparam.simu3, 0.0, 1.0, 3, 0.001);

        fun1 = makeRadio("Gamma");
        fun2 = makeRadio("Fitting");
        if(smokeparam.function==0) fun1->setChecked(true);
        else fun2->setChecked(true);
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->addWidget(fun1.get());
        hLayout->addWidget(fun2.get());
        MainLayout->addRow("去雾函数选择:", hLayout);

        FunGroup = std::make_unique<QButtonGroup>(this);
        FunGroup->addButton(fun1.get(), 0);
        FunGroup->addButton(fun2.get(), 1);
        FunGroup->setExclusive(true);

        savedefault = std::make_unique<QPushButton>("保存为默认", this);  
        savedefault->setStyleSheet(btnStyle);
        MainLayout->addRow("保存为默认设置:", savedefault.get());

        reset = std::make_unique<QPushButton>("恢复出厂设置", this);  
        reset ->setStyleSheet(btnStyle);
        MainLayout->addRow("重置为出厂设置:", reset .get());

        exit = std::make_unique<QPushButton>("退出", this);
        exit ->setStyleSheet(btnStyle);
        MainLayout->addRow("退出:", exit.get());
        
        connect(savedefault.get(), &QPushButton::clicked, this, &ParamWindow::onSaveDefault);
        connect(exit.get(), &QPushButton::clicked, this, &ParamWindow::hide);
        connect(reset.get(), &QPushButton::clicked, this, &ParamWindow::resetToFactoryDefault);

        setCentralWidget(centralWidget);
        this->setFixedSize(600, 560);
    }
}
