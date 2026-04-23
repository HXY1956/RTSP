#ifndef __GPU_ALGO_KRL_H
#define __GPU_ALGO_KRL_H
#include <opencv2/opencv.hpp>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>

namespace SMOKE {
    enum W2_VUpdate_Fun {
        F_Gamma, F_Fitting
    };

    enum RemoverMethods {
        W11, W12, W13, W123
    };

    struct PContext;

    class CudaSmokeRemover {
        PContext* _ctx;
    public:
        /*
        int Unit = 20;      //单元大小：片段单元中的帧数
        float v0 = 0.6;    @视觉阈值
        int Light = 1;      @L系数
        int f = 2;          @切换频率
        int k = 4;          @调试片段数
        */
        CudaSmokeRemover();
        ~CudaSmokeRemover();

        static const std::string version() {
            return "v3.0";
        }

        // @单元大小：片段单元中的帧数，调节范围:[17,1000]
        int getUnit();
        void setUnit(int Unit);

        // @视觉浓度阈值，调节范围:[0,1]
        double getV0();
        void setV0(double v0);

        // @L系数，调节范围:[1,100]
        double getLight();
        void setLight(double Light);

        // @调试片段数，调节范围:[1,100]
        int getK();
        void setK(int k);

        // @切换频率，调节范围:[1,100]
        int getF();
        void setF(int f);

        // @最小透过率（增加变量名），调节范围:[0,1]
        double getTbase();
        void setTbase(double t_base);

        // @S分量调节范围:[0,1]
        double getSIndex();
        void setSIndex(double s_index2);

        // @gamma变化
        double getVGamma();
        void setVGamma(double v_gamma);

        // @v系数1（增加变量名），调节范围:[0,1]
        double getVSimu1();
        void setVSimu1(double v_simu1);

        // @v系数2（增加变量名），调节范围:[0,1]
        double getVSimu2();
        void setVSimu2(double v_simu2);

        // @v系数3（增加变量名），调节范围:[0,1]
        double getVSimu3();
        void setVSimu3(double v_simu3);

        // @细节强度（增加变量名），调节范围:[1,10]
        double getFusionG();
        void setFusionG(double fusion_G);

        // @ default is W2_VUpdate_Fun::F_Gamma
        W2_VUpdate_Fun getW2VUpdateFun();
        void setW2VUpdateFun(W2_VUpdate_Fun fn);
        void setW2VUpdateFun(int fun);

        //@ default is RemoverMethods::W23
        RemoverMethods getRemoverMethods();
        void setRemoverMethods(RemoverMethods methods);
        void setRemoverMethods(int way);

        int getPreExractWXY();
        void setPreExtractWXY(int preExractWX);

        /*
         * input must be CV_8UC3 (BGR format), and output will be CV_8UC3 also.
         */
        void process_one_frame(const cv::Mat input, cv::Mat& output);
        //重置处理缓冲
        void reset_buf();

    };
}

#endif
