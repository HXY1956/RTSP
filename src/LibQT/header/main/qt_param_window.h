#ifndef QT_PARAM_WINDOW_H
#define QT_PARAM_WINDOW_H
#include <QObject>
#include <QMainWindow>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QApplication>
#include <QLibrary>
#include <QString>
#include <QMetaObject>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QSettings>
#include <QWidget>
#include <QButtonGroup>
#include <QRadioButton>
#include "qt_global.h"

namespace QT {
    struct SmokeParams {
        int    unitSize        = 20;
        int    debugSegmentNum = 4;
        int    switchFreq      = 5;

        double v0              = 0.664;
        double light           = 2.2;
        double tbase           = 0.2;
        double sIndex          = 0.7;
        double fusionG         = 2.5;
        double gamma           = 0.4;
        double simu1           = 0.379;
        double simu2           = 0.391;
        double simu3           = 0.418;
        int function = 0;
    };


    class ParamWindow : public QMainWindow {
        Q_OBJECT
    public:
        explicit ParamWindow(QWidget* parent = nullptr);
        ~ParamWindow(){};

    signals:
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

    private slots:
        void onSaveDefault(){
            saveParams(smokeparam);
        };
        void onUnitChange(int val){
            emit NewUnit(val);
            smokeparam.unitSize = val;
        }
        void onKChange(int val){
            emit NewK(val);
            smokeparam.debugSegmentNum = val;
        }
        void onFChange(int val){
            emit NewF(val);
            smokeparam.switchFreq = val;
        }
        void onV0Change(double val){
            emit NewV0(val);
            smokeparam.v0 = val;
        }
        void onLightChange(double val){
            emit NewLight(val);
            smokeparam.light = val;
        }
        void onTbaseChange(double val){
            emit NewTbase(val);
            smokeparam.tbase = val;
        }
        void onSIndexChange(double val){
            emit NewSIndex(val);
            smokeparam.sIndex = val;
        }
        void onVGammaChange(double val){
            emit NewVGamma(val);
            smokeparam.gamma = val;
        }
        void onVSimu1Change(double val){
            emit NewVSimu1(val);
            smokeparam.simu1 = val;
        }
        void onVSimu2Change(double val){
            emit NewVSimu2(val);
            smokeparam.simu2 = val;
        }
        void onVSimu3Change(double val){
            emit NewVSimu3(val);
            smokeparam.simu3 = val;
        }
        void onFusionGChange(double val){
            emit NewFusionG(val);
            smokeparam.fusionG = val;
        }
        void resetToFactoryDefault();
        void onFunChanged(bool checked) {
            QRadioButton* button = qobject_cast<QRadioButton*>(sender());
            if (button && checked) {
                QString methodName = button->text();
                if (methodName.toStdString() == "Gamma"){
                    smokeparam.function = 0;
                    emit NewFun(0);
                }
                else if (methodName.toStdString() == "Fitting"){
                    smokeparam.function = 1;
                    emit NewFun(1);
                }
            }
        }

    private:
        void setupUi();
        void ConnectThreads();
        void ReadFromUI(const SmokeParams& p);
        SmokeParams loadParams();
        void saveParams(const SmokeParams& p);

    private:
        std::unique_ptr<QDoubleSpinBox> V0Edit;
        std::unique_ptr<QDoubleSpinBox> LightEdit;
        std::unique_ptr<QDoubleSpinBox> TbaseEdit;
        std::unique_ptr<QDoubleSpinBox> SIndexEdit;
        std::unique_ptr<QDoubleSpinBox> VGammaEdit;
        std::unique_ptr<QDoubleSpinBox> VSimu1Edit;
        std::unique_ptr<QDoubleSpinBox> VSimu2Edit;
        std::unique_ptr<QDoubleSpinBox> VSimu3Edit;
        std::unique_ptr<QDoubleSpinBox> FusionGEdit;

        std::unique_ptr<QSpinBox> UnitEdit;
        std::unique_ptr<QSpinBox> KEdit;
        std::unique_ptr<QSpinBox> FEdit;

        std::unique_ptr<QPushButton> savedefault;
        std::unique_ptr<QPushButton> exit;
        std::unique_ptr<QPushButton> reset;

        std::unique_ptr<QButtonGroup> FunGroup;
        std::unique_ptr<QRadioButton> fun1;
        std::unique_ptr<QRadioButton> fun2;
        
        SmokeParams smokeparam;

        const QString btnStyle =
            "QPushButton{"
            "  width:180px; height:20px;"
            "  font: bold 12px \"Microsoft YaHei\";"
            "  margin:4px; padding:5px;"
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
    };
}

#endif