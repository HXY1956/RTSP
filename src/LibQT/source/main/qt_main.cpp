#include <QApplication>
#include <QLoggingCategory>
#include <opencv2/core.hpp>
#include "qt_main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QLoggingCategory::setFilterRules(
        "*.debug=false\n"
        "*.warning=false"
    );
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<cv::Mat>("cv::Mat");

    QT::MainWindow w;

    bool autostart = false;
    for(int i = 0; i<argc; i++){
        if(QString(argv[i])=="--auto-start") autostart = true;
    }

    w.show();

    if(autostart){
        QTimer::singleShot(500, &w, SLOT(onStart()));
    } 

    return app.exec();
}