#include "gpu_algo_krl.h"
#include "gpu_util.h"
#include "gpu_replicate_boxfilter.h"
#include "gpu_symmetric_boxfilter.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda_types.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/core/cuda_stream_accessor.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudev/ptr2d/gpumat.hpp> 
#include <opencv2/cudev/expr/reduction.hpp> 
#include <future>
#include <chrono>
#include <iomanip>

#define APP_CUDEV_SAFE_CALL(expr) cv::cudev::checkCudaError((expr), __FILE__, __LINE__, CV_Func)

namespace SMOKE{
    struct PContext {
        int Unit = 20;        // @单元大小：片段单元中的帧数，调节范围:[17,1000]
        double v0 = 0.664;       // @视觉浓度阈值，调节范围:[0,1]
        double Light = 2.2;        // @L系数，调节范围:[1,100]
        int f = 5;            // @切换频率，调节范围:[1,100]
        int k = 4;            // @调试片段数，调节范围:[1,100]
        double t_base = 0.2;   // @最小透过率（增加变量名），调节范围:[0,1]
        double s_index2 = 0.7; // @S分量调节范围:[0,1],
        double v_gamma = 0.4;  // @gamma变化
        double v_simu1 = 0.3786;	// @v系数1（增加变量名），调节范围:[0,1]
        double v_simu2 = 0.3911;	// @v系数2（增加变量名），调节范围:[0,1]
        double v_simu3 = 0.4184;	// @v系数3（增加变量名），调节范围:[0,1]
        double fusion_G = 2.5;   // @细节强度（增加变量名），调节范围:[1,10]
        int preExractWXY = 99;  // pre_extract 抽帧模块x window size
        W2_VUpdate_Fun vupdate_fun = W2_VUpdate_Fun::F_Gamma;
        RemoverMethods removerMethods = RemoverMethods::W11;
        size_t frame_idx = 1;
        int n = 1;
        int i = 1;
        int j = 1;
        double a = 0;
        size_t n_start;
        size_t n_end;
        cv::Mat_<double> a_n;
        cv::Mat_<uint8_t> S;
        double a0;
        double a0_star;
        double v_star;
        int p = 1;        
        int way = 0;
        bool ref_change = false;

        cv::cuda::Stream stream;
        cv::cuda::GpuMat pre_extract_frame;
        cv::cudev::GpuMat_<float> W;
        cv::cudev::GpuMat_<float> W0;
        cv::cudev::GpuMat_<float> t;
        bool frame_gpu_freshed;
        cv::cuda::GpuMat upload_buf;
        cv::cuda::GpuMat gpuSum;
        cv::cuda::GpuMat gpu_max_array;
        cv::cuda::GpuMat gpu_mean_vec;
        cv::cudev::GpuMat_<cv::Point3_<float>> frame_gpu_in;
        cv::cudev::GpuMat_<cv::Point3_<float>> frame_gradient_sqrt;
        cv::cudev::GpuMat_<cv::Point3_<float>> G_base;
        cv::cudev::GpuMat_<cv::Point3_<float>> G_detail;
        cv::cudev::GpuMat_<cv::Point3_<float>> I_defog;
        cv::cudev::GpuMat_<cv::Point3_<float>> pic_out;
        cv::cudev::GpuMat_<float> A_twice;
        cv::cudev::GpuMat_<cv::Point3_<float>> box_filter_out;
        cv::cudev::GpuMat_<cv::Point3_<float>> f3_buf;
        cv::cudev::GpuMat_<float>              f1_tmp;
        cv::cudev::GpuMat_<float>              M;
        cv::cudev::GpuMat_<float>              M_average;
        cv::cudev::GpuMat_<float>              G_base_max;
        cv::cudev::GpuMat_<float>              t_dark;
        cv::cudev::GpuMat_<float>              max_G_base_gpu;
        cv::cudev::GpuMat_<float>              max_M_average_gpu;
        cv::cudev::GpuMat_<float>              sum_out_gpu;
        cv::cudev::GpuMat_<float>              minmax_;
    };
}

namespace SMOKE {

    __host__ __device__ __forceinline__ int divUp(int total, int grain)
    {
        return (total + grain - 1) / grain;
    }

    /**

    在gpu上， 按照像素点:
    1. 计算x方向的梯度计算dx;
    2. 计算y方向的梯度计算dy;
    3. 计算 sqrt(dx^2 + dy^2)/2
    4. 将结果按照像素点存回out中（中间缓冲)
    */
    template<int CHN, typename BufT>
    __global__ void gradient_sqrt_krl(const cv::cuda::PtrStepSz<BufT> img, cv::cuda::PtrStepSz<BufT> s_out) {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < img.rows) && (col_id < img.cols)) {
            int idx1, idx2, div;

            const BufT* row = img.ptr(row_id);

            if (0 == col_id) {
                idx1 = col_id + 1;
                idx2 = col_id;
                div = 1;
            }
            else if ((img.cols - 1) == col_id) {
                idx1 = col_id;
                idx2 = col_id - 1;
                div = 1;
            }
            else {
                idx1 = col_id + 1;
                idx2 = col_id - 1;
                div = 2;
            }
            const BufT* p1 = row + idx1 * CHN;
            const BufT* p2 = row + idx2 * CHN;

            BufT dx1 = (p1[0] - p2[0]) / div;
            BufT dx2 = (p1[1] - p2[1]) / div;
            BufT dx3 = (p1[2] - p2[2]) / div;

            if (0 == row_id) {
                idx1 = 1;
                idx2 = 0;
                div = 1;
            }
            else if ((img.rows - 1) == row_id) {
                idx1 = img.rows - 1;
                idx2 = img.rows - 2;
                div = 1;
            }
            else {
                idx1 = row_id + 1;
                idx2 = row_id - 1;
                div = 2;
            }

            p1 = img.ptr(idx1) + col_id * CHN;
            p2 = img.ptr(idx2) + col_id * CHN;
            BufT dy1 = (p1[0] - p2[0]) / div;
            BufT dy2 = (p1[1] - p2[1]) / div;
            BufT dy3 = (p1[2] - p2[2]) / div;

            BufT* out_pixel = s_out.ptr(row_id) + col_id * CHN;
            out_pixel[0] = sqrt((dx1 * dx1 + dy1 * dy1) / 2.0);
            out_pixel[1] = sqrt((dx2 * dx2 + dy2 * dy2) / 2.0);
            out_pixel[2] = sqrt((dx3 * dx3 + dy3 * dy3) / 2.0);
        }
    }

    template<typename BufT>
    __host__ void gradient_sqrt_gpu(
        const cv::cuda::GpuMat& img,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& out,
        cv::cuda::Stream& stream)
    {
        int rows = img.rows;
        int cols = img.cols;
        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols,threadsPerBlock.x),
            ceil_div(rows,threadsPerBlock.y)
        );
        out.create(rows, cols);
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        gradient_sqrt_krl<3, BufT> << <numBlocks, threadsPerBlock, 0, cuda_stream >> > (img, out);
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
    }

    /*
     * ref matlab code is:
     function outval = avg_gradient(img)
    % OUTVAL = AVG_GRADIENT(IMG)

    if nargin == 1
        img = im2uint8(img);
        img = double(img);
        % Get the size of img
        [r,c,b] = size(img);

        dx = 1;
        dy = 1;
        for k = 1 : b
            band = img(:,:,k);
            [dzdx,dzdy] = gradient(band,dx,dy);
            s = sqrt((dzdx .^ 2 + dzdy .^2) ./ 2);

            g(k) = sum(sum(s)) / ((r - 1) * (c - 1));
        end
        outval = mean(g);
    else
        error('Wrong number of input!');
    end

    */
    double avg_gradient_gpu(PContext& ctx, const cv::cuda::GpuMat& img, cv::cudev::GpuMat_<cv::Point3_<float>>& img_gradient_sqrt, cv::cuda::Stream& stream) {
        int rows = img.rows;
        int cols = img.cols;
        gradient_sqrt_gpu<float>(img, img_gradient_sqrt, stream);

        cv::cuda::calcSum(img_gradient_sqrt, ctx.gpuSum, cv::noArray(), stream);

        cv::Mat sum;
        ctx.gpuSum.download(sum, stream);
        stream.waitForCompletion();
        double avg = cv::mean(sum.at<cv::Matx13d>(0))[0];
        return avg / ((rows - 1) * (cols - 1));
    }

    template<typename BufT>
    __global__ void merge_3channels_with_avg(const cv::cuda::PtrStepSz<BufT> img, cv::cuda::PtrStepSz<BufT> out) {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < img.rows) && (col_id < img.cols)) {
            const BufT* pixel = img.ptr(row_id) + col_id * 3;
            out(row_id, col_id) = (pixel[0] + pixel[1] + pixel[2]) / 3.0;
        }
    }

    template<typename BufT>
    __global__ void merge_3channels_with_min(const cv::cuda::PtrStepSz<BufT> img, cv::cuda::PtrStepSz<BufT> out) {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < img.rows) && (col_id < img.cols)) {
            const BufT* pixel = img.ptr(row_id) + col_id * 3;
            out(row_id, col_id) = min(min(pixel[0], pixel[1]), pixel[2]);
        }
    }

    template<typename BufT>
    __global__ void merge_3channels_with_max(const cv::cuda::PtrStepSz<BufT> img, cv::cuda::PtrStepSz<BufT> out) {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < img.rows) && (col_id < img.cols)) {
            const BufT* pixel = img.ptr(row_id) + col_id * 3;
            out(row_id, col_id) = max(max(pixel[0], pixel[1]), pixel[2]);
        }
    }

    __host__ void cuda_merge_3channels_with_avg(const cv::cudev::GpuMat_<cv::Point3_<float>>& img, cv::cudev::GpuMat_<float>& out, cv::cuda::Stream& stream) {
        int rows = img.rows;
        int cols = img.cols;
        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols,threadsPerBlock.x),
            ceil_div(rows,threadsPerBlock.y)
        );
        out.create(rows, cols);
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        merge_3channels_with_avg<float> <<<numBlocks, threadsPerBlock, 0, cuda_stream>>> (img, out);
        //stream.waitForCompletion();
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
    }

    __host__ void cuda_merge_3channels_with_min(const cv::cudev::GpuMat_<cv::Point3_<float>>& img, cv::cudev::GpuMat_<float>& out, cv::cuda::Stream& stream) {
        int rows = img.rows;
        int cols = img.cols;
        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols,threadsPerBlock.x),
            ceil_div(rows,threadsPerBlock.y)
        );
        out.create(rows, cols);
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        merge_3channels_with_min<float> <<<numBlocks, threadsPerBlock, 0, cuda_stream >>> (img, out);
        //stream.waitForCompletion();
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
    }

    __host__ void cuda_merge_3channels_with_max(const cv::cudev::GpuMat_<cv::Point3_<float>>& img, cv::cudev::GpuMat_<float>& out, cv::cuda::Stream& stream) {
        int rows = img.rows;
        int cols = img.cols;
        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols,threadsPerBlock.x),
            ceil_div(rows,threadsPerBlock.y)
        );
        out.create(rows, cols);
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        merge_3channels_with_max<float> <<<numBlocks, threadsPerBlock, 0, cuda_stream>>> (img, out);
        //stream.waitForCompletion();
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
    }

    template<typename ColorT>
    __device__ void fRGB2HSV_krl(
        const ColorT R, const ColorT G, const ColorT B,
        ColorT& H, ColorT& S, ColorT& V)
    {
        V = fmax(fmax(R, G), B);
        if (V != 0)
        {
            const ColorT minRGB = fmin(fmin(R, G), B);
            const ColorT max_min = V - minRGB;
            S = max_min / V;

            ColorT factor = 0.6666666666666666;
            ColorT C1 = R;
            ColorT C2 = G;

            if (fabs(V - R) < 1e-5)
            {
                factor = 0;
                C1 = G;
                C2 = B;
            }
            else if (fabs(V - G) < 1e-5)
            {
                factor = 0.3333333333333333;
                C1 = B;
                C2 = R;
            }
            H = factor + (0.16666666666666666 * (C1 - C2)) / max_min;
            if (H < 0)
            {
                H = H + 1.;
            }
        }
        else
        {
            S = 0;
            H = 0;
        }

    }

    template<typename ColorT>
    __device__ void HSV2fRGB_krl(
        const ColorT h, const ColorT s, const ColorT v,
        ColorT& r, ColorT& g, ColorT& b)
    {
        if (fabs(s) < 1e-5) {
            r = g = b = v;
        }
        else {
            ColorT hi = h * 6;
            int offset = floor(hi);
            ColorT f = hi - offset;
            ColorT p = v * (1 - s);
            ColorT q = v * (1 - s * f);
            ColorT t = v * (1 - s * (1 - f));
            switch (offset)
            {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
            default:
                break;
            }
        }
    }

    template<typename BufT>
    __global__ void pre_step_t_dark(
        const BufT minFactor,
        const BufT A,
        const cv::cuda::PtrStepSz<BufT> M_average,
        const cv::cuda::PtrStepSz<BufT> M,
        cv::cuda::PtrStepSz<BufT> t_dark) {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < M.rows) && (col_id < M.cols)) {
            BufT t = 1 - fmin(minFactor * M_average(row_id, col_id), M(row_id, col_id)) / A;
            const BufT t_min = 0.1f;
            t_dark(row_id, col_id) = fmin(fmax(t, t_min), (BufT)1);
        }
    }

    //从小到大排序
    template<typename BufT>
    __device__ void SortSwap(BufT& v1, int& id1, BufT& v2, int& id2) {
        if (v1 > v2) {
            BufT t = v1; v1 = v2; v2 = t;
            int idx = id1;  id1 = id2; id2 = idx;
        }
    }

    /*
        %矩阵运算
        A_self_adaption = ones(height,width,D).*A;
        %求均值滤波后图像每个像素点rgb三通道最大值RGB_max和所在的位置
        [~,MAX] = max(G_base,[],3);
        %寻找RGB三通道中中间值的位置
        MID = 6*ones(height,width)-MIN-MAX;
        MID(MID>3) = 3;
        %遍历每个像素点，进行代数运算
        for ii = 1:height
            for jj = 1:width
                a = MAX(ii,jj);
                b = MID(ii,jj);
                c = MIN(ii,jj);
                A_self_adaption(ii,jj,a) = A+((G_base(ii,jj,a)-G_base(ii,jj,b)));
                A_self_adaption(ii,jj,c) = A-((G_base(ii,jj,b)-G_base(ii,jj,c)));
            end
        end

        % 求解I_defog
        % 建立一个与图像等大的图像
        I_defog = zeros(height,width,D);
        % 分别对RGB三个通道进行代数运算
        for ii = 1:D
                I_defog(:,:,ii) = (G_base(:,:,ii)-A_self_adaption(:,:,ii).*(1-t_dark))./t_dark;
        end
        % 进行数值约束
        I_defog(I_defog<0) = 0;
        I_defog(I_defog>1) = 1;
    */
    template<int CHN, typename BufT>
    __global__ void pre_step_I_defog(
        const BufT A,
        const cv::cuda::PtrStepSz<BufT> G_base,
        const cv::cuda::PtrStepSz<BufT> t_dark,
        cv::cuda::PtrStepSz<BufT> I_defog
    ) {

        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < G_base.rows) && (col_id < G_base.cols)) {
            const BufT* base_pixelPtr = G_base.ptr(row_id) + col_id * CHN;
            BufT MAX = base_pixelPtr[0];
            BufT MID = base_pixelPtr[1];
            BufT MIN = base_pixelPtr[2];
            int MAX_ID = 0;
            int MID_ID = 1;
            int MIN_ID = 2;
            SortSwap<BufT>(MID, MID_ID, MAX, MAX_ID);
            SortSwap<BufT>(MIN, MIN_ID, MAX, MAX_ID);
            SortSwap<BufT>(MIN, MIN_ID, MID, MID_ID);
            BufT A_self_adaption_max = A + (MAX - MID);
            BufT A_self_adaption_min = A - (MID - MIN);
            BufT A_self_adaption_mid = A;

            BufT t_dark_v = t_dark(row_id, col_id);
            BufT* I_defog_pixelPtr = I_defog.ptr(row_id) + col_id * CHN;
            I_defog_pixelPtr[MAX_ID] =
                fmin(fmax(
                    (base_pixelPtr[MAX_ID] - A_self_adaption_max * (1 - t_dark_v)) / t_dark_v,
                    0.0f), 1.0f);
            I_defog_pixelPtr[MID_ID] =
                fmin(fmax(
                    (base_pixelPtr[MID_ID] - A_self_adaption_mid * (1 - t_dark_v)) / t_dark_v,
                    0.0f), 1.0f);
            I_defog_pixelPtr[MIN_ID] =
                fmin(fmax(
                    (base_pixelPtr[MIN_ID] - A_self_adaption_min * (1 - t_dark_v)) / t_dark_v,
                    0.0f), 1.0f);
        }
    }

    /****
     *
        % 初步处理：将待处理图像素值数据类型归一化，将8位无符号整数类型专为双精度浮点型
        I1 = im2double(pic);

        % 计算基准图：对待处理图I进行窗口大小为5的均值滤波
        w = fspecial('average',5);
        G_base = imfilter(I1,w,'replicate');

        % 计算细节图：用待处理图与均值滤波后图像作差
        G_detail = I1-G_base;
     *
    */
    template<typename BufT>
    void pre_step1_gpu(
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& img,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& G_base,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& G_detail,
        cv::cuda::Stream& stream) {
        replicate_boxfilter_3ch(img, G_base, G_detail, 2, stream);
        cv::cuda::subtract(img, G_base, G_detail, cv::noArray(), -1, stream);
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
    }

    /***
     *

        % 求均值滤波后图像每个像素点rgb三通道最小值M和所在的位置
        [M,MIN] = min(G_base,[],3);

        % 求图像大小
        mask = ceil(max(height,width)/50);
        if mod(mask,2) == 0
            mask = mask + 1;
        end
        f_mask = fspecial('average',mask);
        M_average = imfilter(M,f_mask,'symmetric');

        % 求引导滤波之后图像M_average所有像素点像素值的平均值
        M_average_value = sum(sum(M_average))/(height*width);

        % 计算自适应A：
        % 取最大值运算和代数运算
        A = 0.5*(max(max(max(G_base,[],3))) + max(max(M_average)));


        % 计算L：按像素点取最小值
        delta = 2;
        L = min(min(delta*M_average_value,0.9)*M_average,M);
        % 计算t：代数运算
        t_dark = 1 - L./A;


        %矩阵运算
        A_self_adaption = ones(height,width,D).*A;
        %求均值滤波后图像每个像素点rgb三通道最大值RGB_max和所在的位置
        [~,MAX] = max(G_base,[],3);
        %寻找RGB三通道中中间值的位置
        MID = 6*ones(height,width)-MIN-MAX;
        MID(MID>3) = 3;
        %遍历每个像素点，进行代数运算
        for ii = 1:height
            for jj = 1:width
                a = MAX(ii,jj);
                b = MID(ii,jj);
                c = MIN(ii,jj);
                A_self_adaption(ii,jj,a) = A+((G_base(ii,jj,a)-G_base(ii,jj,b)));
                A_self_adaption(ii,jj,c) = A-((G_base(ii,jj,b)-G_base(ii,jj,c)));
            end
        end


        % 求解I_defog
        % 建立一个与图像等大的图像
        I_defog = zeros(height,width,D);
        % 分别对RGB三个通道进行代数运算
        for ii = 1:D
                I_defog(:,:,ii) = (G_base(:,:,ii)-A_self_adaption(:,:,ii).*(1-t_dark))./t_dark;
        end
        % 进行数值约束
        I_defog(I_defog<0) = 0;
        I_defog(I_defog>1) = 1;


     *
    */

    template<typename BufT>
    void pre_step2_gpu(
        PContext& ctx,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& G_base,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& I_defog,
        cv::cuda::Stream& stream) {

        /*
          % 求均值滤波后图像每个像素点rgb三通道最小值M和所在的位置
          [M,MIN] = min(G_base,[],3);
        */
        int rows = G_base.rows;
        int cols = G_base.cols;

        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols,threadsPerBlock.x),
            ceil_div(rows,threadsPerBlock.y)
        );
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);

        cuda_merge_3channels_with_min(G_base, ctx.M, stream);




        /**
            % 求图像大小
            mask = ceil(max(height,width)/50);
            if mod(mask,2) == 0
                mask = mask + 1;
            end
            f_mask = fspecial('average',mask);
            M_average = imfilter(M,f_mask,'symmetric');
         *
         */
        int mx = rows;
        if (cols > rows) {
            mx = cols;
        }
        int mask = ceil(mx / 50);
        if (mask % 2 == 0) {
            mask += 1;
        }
        int HalfCXCY = mask / 2;

        symmetric_boxfilter(ctx.M, ctx.M_average, ctx.f1_tmp, HalfCXCY, stream);

        APP_CUDEV_SAFE_CALL(cudaGetLastError());


        /*
            % 求引导滤波之后图像M_average所有像素点像素值的平均值
            M_average_value = sum(sum(M_average))/(height*width);
        */
        BufT M_average_value;
        {
            ctx.sum_out_gpu.assign(cv::cudev::sum_(ctx.M_average), stream);
            BufT sum_out;
            ctx.sum_out_gpu.download(cv::_OutputArray(&sum_out, 1), stream);
            M_average_value = sum_out / (rows * cols);
        }


        /*
         % 计算自适应A：
         % 取最大值运算和代数运算
         A = 0.5*(max(max(max(G_base,[],3))) + max(max(M_average)));
        */

        BufT A;
        {

            cuda_merge_3channels_with_max(G_base, ctx.G_base_max, stream);

            ctx.max_G_base_gpu.assign(cv::cudev::maxVal_(ctx.G_base_max), stream);
            ctx.max_M_average_gpu.assign(cv::cudev::maxVal_(ctx.M_average), stream);

            BufT max_G_base;
            BufT max_M_average;
            ctx.max_G_base_gpu.download(cv::_OutputArray(&max_G_base, 1), stream);
            ctx.max_M_average_gpu.download(cv::_OutputArray(&max_M_average, 1), stream);

            A = 0.5 * max_G_base + max_M_average;
        }




        /*
        % 计算L：按像素点取最小值
        delta = 2;
        L = min(min(delta*M_average_value,0.9)*M_average,M);
        % 计算t：代数运算
        t_dark = 1 - L./A;
        */
        BufT delta = 2;
        BufT minFactor = std::min(delta * M_average_value, 0.9f);
        ctx.t_dark.create(rows, cols);
        pre_step_t_dark<BufT> << <numBlocks, threadsPerBlock, 0, cuda_stream >> > (minFactor, A, ctx.M_average, ctx.M, ctx.t_dark);
        APP_CUDEV_SAFE_CALL(cudaGetLastError());


        /*
        %矩阵运算
        A_self_adaption = ones(height,width,D).*A;
        %求均值滤波后图像每个像素点rgb三通道最大值RGB_max和所在的位置
        [~,MAX] = max(G_base,[],3);
        %寻找RGB三通道中中间值的位置
        MID = 6*ones(height,width)-MIN-MAX;
        MID(MID>3) = 3;
        %遍历每个像素点，进行代数运算
        for ii = 1:height
            for jj = 1:width
                a = MAX(ii,jj);
                b = MID(ii,jj);
                c = MIN(ii,jj);
                A_self_adaption(ii,jj,a) = A+((G_base(ii,jj,a)-G_base(ii,jj,b)));
                A_self_adaption(ii,jj,c) = A-((G_base(ii,jj,b)-G_base(ii,jj,c)));
            end
        end

        % 求解I_defog
        % 建立一个与图像等大的图像
        I_defog = zeros(height,width,D);
        % 分别对RGB三个通道进行代数运算
        for ii = 1:D
                I_defog(:,:,ii) = (G_base(:,:,ii)-A_self_adaption(:,:,ii).*(1-t_dark))./t_dark;
        end
        % 进行数值约束
        I_defog(I_defog<0) = 0;
        I_defog(I_defog>1) = 1;
        */
        I_defog.create(rows, cols);
        pre_step_I_defog<3, BufT> << <numBlocks, threadsPerBlock, 0, cuda_stream >> > (A, G_base, ctx.t_dark, I_defog);
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
    }

    template<typename BufT>
    void call_pre_step_gpu(
        PContext& ctx,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& img,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& G_detail,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& I_defog,
        cv::cuda::Stream& stream
    ) {
        pre_step1_gpu<float>(img, ctx.G_base, G_detail, stream);
        pre_step2_gpu<float>(ctx, ctx.G_base, I_defog, stream);
    }
    /*
        % （1）gamma变化
        % Mo Notes: v = I_defog1(:,:,3).^0.4
        v = imadjust(I_defog1(:,:,3),[0,1],[0,1],0.4);
     */
    __device__ __inline__ float gamma_f3(float V, float v_gamma, float /*no used*/, float /*no used*/) {
        return powf(V, v_gamma);
    }

    /*
        % （2）拟合函数变化
        %  v = 0.8-0.3911*cos(2.445*I_defog1(:,:,3))+0.4184*sin(2.445*I_defog1(:,:,3));
     */
    __device__ __inline__ float fitting_f3(float V, float v_simu1, float v_simu2, float v_simu3) {
        return v_simu1 - v_simu2 * cos(2.445 * V) + v_simu3 * sin(2.445 * V);
    }

    /*
            % 将rgb色域转化到HSV色域
            I_defog1 = rgb2hsv(I_defog);

            % @对S分量进行代数运算
            S_index = 2;
            s = S_index*(1-1./(1+((I_defog1(:,:,2))./0.3).^2));
            % 进行数值约束
            s(s>1) = 1;
            s(s<0) = 0;
            I_defog1(:,:,2) = s;

            % @对V分量进行代数运算，以下两种变化（1）和（2）进行选择，都需要改写为并行程序：
            % （1）gamma变化
            % Mo Notes: v = I_defog1(:,:,3).^0.4
            v = imadjust(I_defog1(:,:,3),[0,1],[0,1],0.4);

            % （2）拟合函数变化
            %  v = 0.8-0.3911*cos(2.445*I_defog1(:,:,3))+0.4184*sin(2.445*I_defog1(:,:,3));

            I_defog1(:,:,3) = v;

            % 将HSV色域转化到rgb色域
            I_defog2 = hsv2rgb(I_defog1);

            % @融合图像：计算defog1
            defog1 = I_defog2+1.5*G_detail;
            pic_out = defog1;
    */
    template<int CHN, typename BufT, float FnUpdateV(float) >
    __global__ void process_w2_krl(
        const cv::cuda::PtrStepSz<BufT> I_defog,
        const cv::cuda::PtrStepSz<BufT> G_detail,
        BufT s_index2,
        BufT fusion_G,
        cv::cuda::PtrStepSz<BufT> pic_out
    ) {

        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < I_defog.rows) && (col_id < I_defog.cols)) {
            const BufT* I_defog_pixelPtr = I_defog.ptr(row_id) + col_id * CHN;
            BufT  R, G, B, H, S, V;
            B = I_defog_pixelPtr[0];
            G = I_defog_pixelPtr[1];
            R = I_defog_pixelPtr[2];
            fRGB2HSV_krl<BufT>(R, G, B, H, S, V);

            /*
             % @对S分量进行代数运算
            S_index = 2;
            %(old)s = S_index*(1-1./(1+((I_defog1(:,:,2))./0.3).^2));
            s = S_index*(1-1./(1+((I_defog1(:,:,2))./ S_index2).^2));
            % 进行数值约束
            s(s>1) = 1;
            s(s<0) = 0;
            I_defog1(:,:,2) = s;
            */
            // S*S >=0, 
            // 0<=1.0/(1+(S*S/0.09))<=1,  
            // 0<=(1- 1.0/(1.0+(S*S/0.09))<=1
            S = S / s_index2;
            S = 2 * (1 - 1.0 / (1.0 + S * S));
            if (S > 1) {
                S = 1.0f;
            }


            V = FnUpdateV(V);

            /*
             % 将HSV色域转化到rgb色域
             I_defog2 = hsv2rgb(I_defog1);

             % @融合图像：计算defog1
             (old) defog1 = I_defog2+1.5*G_detail;
             defog1 = I_defog2+fusion_G*G_detail;
             pic_out = defog1;
            */
            HSV2fRGB_krl<BufT>(H, S, V, R, G, B);
            const BufT* G_detail_pixelPtr = G_detail.ptr(row_id) + col_id * CHN;
            BufT* pic_out_pixelPtr = pic_out.ptr(row_id) + col_id * CHN;
            pic_out_pixelPtr[0] = B + fusion_G * G_detail_pixelPtr[0];
            pic_out_pixelPtr[1] = G + fusion_G * G_detail_pixelPtr[1];
            pic_out_pixelPtr[2] = R + fusion_G * G_detail_pixelPtr[2];
        }

    }

    template<int CHN, typename BufT, float FnUpdateV(float, float, float, float) >
    __global__ void process_w2_krl_with_args(
        const cv::cuda::PtrStepSz<BufT> I_defog,
        const cv::cuda::PtrStepSz<BufT> G_detail,
        BufT s_index2,
        BufT fusion_G,
        cv::cuda::PtrStepSz<BufT> pic_out,
        float v1,
        float v2,
        float v3
    ) {

        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < I_defog.rows) && (col_id < I_defog.cols)) {
            const BufT* I_defog_pixelPtr = I_defog.ptr(row_id) + col_id * CHN;
            BufT  R, G, B, H, S, V;
            B = I_defog_pixelPtr[0];
            G = I_defog_pixelPtr[1];
            R = I_defog_pixelPtr[2];
            fRGB2HSV_krl<BufT>(R, G, B, H, S, V);

            /*
               % @对S分量进行代数运算
              S_index = 2;
              %(old)s = S_index*(1-1./(1+((I_defog1(:,:,2))./0.3).^2));
              s = S_index*(1-1./(1+((I_defog1(:,:,2))./ S_index2).^2));
              % 进行数值约束
              s(s>1) = 1;
              s(s<0) = 0;
              I_defog1(:,:,2) = s;
              */
              // S*S >=0,
              // 0<=1.0/(1+(S*S/0.09))<=1,
              // 0<=(1- 1.0/(1.0+(S*S/0.09))<=1
            S = S / s_index2;
            S = 2 * (1 - 1.0 / (1.0 + S * S));
            if (S > 1) {
                S = 1.0f;
            }


            V = FnUpdateV(V, v1, v2, v3);

            /*
              % 将HSV色域转化到rgb色域
              I_defog2 = hsv2rgb(I_defog1);

              % @融合图像：计算defog1
              (old) defog1 = I_defog2+1.5*G_detail;
              defog1 = I_defog2+fusion_G*G_detail;
              pic_out = defog1;
             */
            HSV2fRGB_krl<BufT>(H, S, V, R, G, B);
            const BufT* G_detail_pixelPtr = G_detail.ptr(row_id) + col_id * CHN;
            BufT* pic_out_pixelPtr = pic_out.ptr(row_id) + col_id * CHN;
            pic_out_pixelPtr[0] = B + fusion_G * G_detail_pixelPtr[0];
            pic_out_pixelPtr[1] = G + fusion_G * G_detail_pixelPtr[1];
            pic_out_pixelPtr[2] = R + fusion_G * G_detail_pixelPtr[2];
        }

    }

template<typename BufT>
bool call_process_w2_gpu(
    PContext& ctx,
    const cv::cudev::GpuMat_<cv::Point3_<BufT>>& I_defog,
    const cv::cudev::GpuMat_<cv::Point3_<BufT>>& G_detail,
    cv::cudev::GpuMat_<cv::Point3_<BufT>>& pic_out,
    cv::cuda::Stream& stream
) {
        int rows = I_defog.rows;
        int cols = I_defog.cols;

        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols, threadsPerBlock.x),
            ceil_div(rows, threadsPerBlock.y)
        );

        pic_out.create(rows, cols);
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);

        switch (ctx.vupdate_fun) {
        case W2_VUpdate_Fun::F_Gamma:
            process_w2_krl_with_args<3, BufT, gamma_f3> <<<numBlocks, threadsPerBlock, 0, cuda_stream>>>(
                I_defog, G_detail, ctx.s_index2, ctx.fusion_G, pic_out, ctx.v_gamma, 0.0f, 0.0f);
            break;
        case W2_VUpdate_Fun::F_Fitting:
            process_w2_krl_with_args<3, BufT, fitting_f3> <<<numBlocks, threadsPerBlock, 0, cuda_stream>>>(
                I_defog, G_detail, ctx.s_index2, ctx.fusion_G, pic_out, ctx.v_simu1, ctx.v_simu2, ctx.v_simu3);
            break;
        default:
            break;
        }

        APP_CUDEV_SAFE_CALL(cudaGetLastError());
        APP_CUDEV_SAFE_CALL(cudaStreamSynchronize(cuda_stream));
}

    /*
       V_defog = 1-W./W0;
        t = (V_defog - 0.9674)/-0.989;    % 模型计算的透过率
    */
    template<typename BufT>
    __global__ void w3_update_t_step1_krl(
        const cv::cuda::PtrStepSz<BufT> W,
        const cv::cuda::PtrStepSz<BufT> W0,
        cv::cuda::PtrStepSz<BufT> t)
    {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < W.rows) && (col_id < W.cols)) {
            if(W0(row_id,col_id) > 1e-6){
                t(row_id, col_id) = (1 - 0.9674 - W(row_id, col_id) / W0(row_id, col_id)) / (-0.989);
            }
            else if(W0(row_id,col_id) <= 1e-6 && W(row_id,col_id) <= 1e-6){
                t(row_id, col_id) = (1 - 0.9674 - 1) / (-0.989);
            }
            else{
                t(row_id, col_id) = (1 - 0.9674 - W(row_id, col_id) / 1e-6) / (-0.989);
            }
        }
    }

    /*
    if tmOn ~= 0
        t(posi_tm) = t_start+(t_mid-t_start)*(t(posi_tm)-min_t)/(t_mid-min_t);
    end
    t(posi_tm) = t_start  - (t_mid-t_start)*min_t/(t_mid-min_t) + (t_mid-t_start)/(t_mid-min_t)*t(posi_tm) ;

    m1 = t_start  - (t_mid-t_start)*min_t/(t_mid-min_t);
    m2 = (t_mid-t_start)/(t_mid-min_t);
    t(posi_tm) = m1 + m2*t(posi_tm);
    */
    template<typename BufT>
    __global__ void w3_update_t_step2_m_krl(
        const BufT t_mid,
        const BufT m1,
        const BufT m2,
        cv::cuda::PtrStepSz<BufT> t)
    {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < t.rows) && (col_id < t.cols)) {
            BufT tv = t(row_id, col_id);
            if (tv < t_mid) {
                t(row_id, col_id) = m2 * tv + m1;
            }
        }
    }

    /*
    if tMOn ~= 0
        t(posi_tM) = t_mid+(t_end-t_mid)*(t(posi_tM)-t_mid)/(max_t-t_mid);
    end
    => t(posi_tM) = t_mid -(t_end-t_mid)*t_mid/(max_t-t_mid) + (t_end-t_mid)/(max_t-t_mid)* t(posi_tM);
    => M1 =  t_mid -(t_end-t_mid)*t_mid/(max_t-t_mid)
    M2 =  (t_end-t_mid)/(max_t-t_mid)
    t(posi_tM) = M2*t(posi_tM) + M1;

    */
    template<typename BufT>
    __global__ void w3_update_t_step2_M_krl(
        const BufT t_mid,
        const BufT M1,
        const BufT M2,
        cv::cuda::PtrStepSz<BufT> t)
    {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < t.rows) && (col_id < t.cols)) {
            BufT tv = t(row_id, col_id);
            if (tv > t_mid) {
                t(row_id, col_id) = M2 * tv + M1;
            }
        }
    }

    template<typename BufT>
    __global__ void w3_update_t_step2_mM_krl(
        const BufT t_mid,
        const BufT m1,
        const BufT m2,
        const BufT M1,
        const BufT M2,
        cv::cuda::PtrStepSz<BufT> t)
    {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < t.rows) && (col_id < t.cols)) {
            BufT tv = t(row_id, col_id);
            if (tv < t_mid) {
                t(row_id, col_id) = m2 * tv + m1;
            }
            else if (tv > t_mid) {
                t(row_id, col_id) = M2 * tv + M1;
            }
        }
    }

    /*
     % 单元第一帧更新t
        if i == 1
        % 计算和处理透过率
            tMOn = 1;       % 执行修正后半段透过率
            tmOn = 1;       % 执行修正前半段透过率
            t_start = 0.2+v_star-v0;      % 限制透过率最小值
            t_end = 1;          % 限制透过率最大值
            t_mid = v_star;          % 分段分界点

            % 单线程：窗口函数处理上一个判断的抽中帧 pic_n
            V_defog = 1-W./W0;
            t = (V_defog - 0.9674)/-0.989;    % 模型计算的透过率
            % 找出 t 中最小值和最大值
            min_t = min(min(t));
            max_t = max(max(t));

            % 限制透过率最小值为t_start
            if t_start < min_t
                t_start = min_t;
                if t_start > t_mid
                    tmOn = 0;       % 透过率最小值也大于t_mid时，取消修前半段改透过率
                end
            end

            % 限制透过率最大值为t_end
            if t_end > max_t
                t_end = max_t;
                if t_end < t_mid
                    tMOn = 0;       % 取透过率最大值也小于t_mid时，消修后半段改透过率
                end
            end

            % 按t_mid的值，分前后两段来限制t
            if t_mid > max_t
                t_mid = max_t;
            end
            posi_tM = find(t>t_mid);
            posi_tm = find(t<t_mid);
            if tmOn ~= 0
                t(posi_tm) = t_start+(t_mid-t_start)*(t(posi_tm)-min_t)/(t_mid-min_t);
            end
            if tMOn ~= 0
                t(posi_tM) = t_mid+(t_end-t_mid)*(t(posi_tM)-t_mid)/(max_t-t_mid);
            end
        end
    */
    template<typename BufT>
    void process_w3_update_t_gpu(PContext& ctx, cv::cudev::GpuMat_<BufT>& t, cv::cuda::Stream& stream) {
        if (ctx.i != 1) {
            return;
        }

        float tMOn = 1;
        float tmOn = 1;
        float t_start = ctx.t_base + ctx.v_star - ctx.v0;
        float t_end = 1;
        float t_mid = ctx.v_star;
        int rows = ctx.W.rows;
        int cols = ctx.W.cols;

        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols,threadsPerBlock.x),
            ceil_div(rows,threadsPerBlock.y)
        );
        t.create(rows, cols);

        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        w3_update_t_step1_krl<BufT> << <numBlocks, threadsPerBlock, 0, cuda_stream >> > (ctx.W, ctx.W0, t);
        APP_CUDEV_SAFE_CALL(cudaGetLastError());

        ctx.minmax_.assign(cv::cudev::minMaxVal_(t), stream);
        float minmax[2];
        ctx.minmax_.download(cv::Mat(1, 2, CV_32FC1, minmax), stream);

        stream.waitForCompletion();

        const float& min_t = minmax[0];
        const float& max_t = minmax[1];

        if (t_start < min_t) {
            t_start = min_t;
            if (t_start > t_mid) {
                tmOn = 0;
            }
        }
        if (t_end > max_t) {
            t_end = max_t;
            if (t_end < t_mid) {
                tMOn = 0;
            }
        }
        if (t_mid > max_t) {
            t_mid = max_t;
        }

        /*
         if tmOn ~= 0
            t(posi_tm) = t_start+(t_mid-t_start)*(t(posi_tm)-min_t)/(t_mid-min_t);
        end
         => t(posi_tm) = t_start  - (t_mid-t_start)*min_t/(t_mid-min_t) + (t_mid-t_start)/(t_mid-min_t)*t(posi_tm) ;
         => m1 = t_start  - (t_mid-t_start)*min_t/(t_mid-min_t);
            m2 = (t_mid-t_start)/(t_mid-min_t);
            t(posi_tm) = m2*t(posi_tm) + m1;


         if tMOn ~= 0
                t(posi_tM) = t_mid+(t_end-t_mid)*(t(posi_tM)-t_mid)/(max_t-t_mid);
         end
         => t(posi_tM) = t_mid -(t_end-t_mid)*t_mid/(max_t-t_mid) + (t_end-t_mid)/(max_t-t_mid)* t(posi_tM);
         => M1 =  t_mid -(t_end-t_mid)*t_mid/(max_t-t_mid)
            M2 =  (t_end-t_mid)/(max_t-t_mid)
            t(posi_tM) = M2*t(posi_tM) + M1;
        */

        float m1 = t_start - (t_mid - t_start) * min_t / (t_mid - min_t);
        float m2 = (t_mid - t_start) / (t_mid - min_t);
        float M1 = t_mid - (t_end - t_mid) * t_mid / (max_t - t_mid);
        float M2 = (t_end - t_mid) / (max_t - t_mid);
        if (tMOn != 0 && tmOn != 0) {
            w3_update_t_step2_mM_krl<float> << <numBlocks, threadsPerBlock, 0, cuda_stream >> > (t_mid, m1, m2, M1, M2, t);
            APP_CUDEV_SAFE_CALL(cudaGetLastError());
        }
        else if (tmOn != 0) {
            w3_update_t_step2_m_krl<float> << <numBlocks, threadsPerBlock, 0, cuda_stream >> > (t_mid, m1, m2, t);
            APP_CUDEV_SAFE_CALL(cudaGetLastError());
        }
        else if (tMOn != 0) {
            w3_update_t_step2_M_krl<float> << <numBlocks, threadsPerBlock, 0, cuda_stream >> > (t_mid, M1, M2, t);
            APP_CUDEV_SAFE_CALL(cudaGetLastError());
        }
    }

    /*
        % 计算二次处理的 L_use

        L_use(:,:,1) = L_r*A_twice.*(1-t);
        L_use(:,:,2) = L_g*A_twice.*(1-t);
        L_use(:,:,3) = L_b*A_twice.*(1-t);

        % @计算defog2
        defog2 = (I_defog - Light*L_use)./t + G_detail;
        pic_out = defog2;
    */
    template<int CHN, typename BufT>
    __global__ void process_w3_krl(
        const BufT Light,
        const BufT L_r,
        const BufT L_g,
        const BufT L_b,
        const cv::cuda::PtrStepSz<BufT> t,
        const cv::cuda::PtrStepSz<BufT> A_twice,
        const cv::cuda::PtrStepSz<BufT> I_defog,
        const cv::cuda::PtrStepSz<BufT> G_detail,
        cv::cuda::PtrStepSz<BufT> pic_out
    ) {
        int row_id = threadIdx.y + blockIdx.y * blockDim.y; //行坐标;
        int col_id = threadIdx.x + blockIdx.x * blockDim.x; //列坐标;
        if ((row_id < I_defog.rows) && (col_id < I_defog.cols)) {
            BufT t_value = t(row_id, col_id);
            BufT A_twice_value = A_twice(row_id, col_id);
            BufT L_use_r = L_r * A_twice_value * (1 - t_value);
            BufT L_use_g = L_g * A_twice_value * (1 - t_value);
            BufT L_use_b = L_b * A_twice_value * (1 - t_value);

            const BufT* I_defog_pixelPtr = I_defog.ptr(row_id) + col_id * CHN;
            const BufT* G_detail_pixelPtr = G_detail.ptr(row_id) + col_id * CHN;
            BufT* out_pixelPtr = pic_out.ptr(row_id) + col_id * CHN;
            out_pixelPtr[0] = (I_defog_pixelPtr[0] - Light * L_use_r) / t_value + G_detail_pixelPtr[0];
            out_pixelPtr[1] = (I_defog_pixelPtr[1] - Light * L_use_g) / t_value + G_detail_pixelPtr[1];
            out_pixelPtr[2] = (I_defog_pixelPtr[2] - Light * L_use_b) / t_value + G_detail_pixelPtr[2];
        //     if (t_value == 0) {
        //     printf("row_id: %d, col_id: %d, Light * L_use_r: %f, Light * L_use_g: %f, Light * L_use_b: %f\n",
        //            row_id, col_id, Light * L_use_r, Light * L_use_g, Light * L_use_b);
        //     printf("row_id: %d, col_id: %d, I_defog_pixelPtr[0]: %f, I_defog_pixelPtr[1]: %f, I_defog_pixelPtr[2]: %f\n",
        //            row_id, col_id, I_defog_pixelPtr[0], I_defog_pixelPtr[1], I_defog_pixelPtr[2]);
        //     printf("row_id: %d, col_id: %d, G_detail_pixelPtr[0]: %f, G_detail_pixelPtr[1]: %f, G_detail_pixelPtr[2]: %f\n",
        //            row_id, col_id, G_detail_pixelPtr[0], G_detail_pixelPtr[1], G_detail_pixelPtr[2]);
        //     printf("row_id: %d, col_id: %d, out_pixelPtr[0]: %f, out_pixelPtr[1]: %f, out_pixelPtr[2]: %f\n",
        //            row_id, col_id, out_pixelPtr[0], out_pixelPtr[1], out_pixelPtr[2]);}
        }
    }

    /*

        % 计算暗原色图层：取RGB三通道最小值
        % Mo Notes: 相当于 A_twice = min(I_defog, [], 3)
        tri = zeros(D,height*width);
        for ii = 1:D   % 转成二维数组
            a = I_defog(:,:,ii);
            tri(ii,:) = a(:)';
        end
        a_m = min(tri);
        A_twice = reshape(a_m,height,width);


        % 计算二次处理的 L_use
        A_r = mean(max(I_defog(:,:,1)));
        A_g = mean(max(I_defog(:,:,2)));
        A_b = mean(max(I_defog(:,:,3)));
        L_r = A_r/(A_r+A_g+A_b);
        L_g = A_g/(A_r+A_g+A_b);
        L_b = A_b/(A_r+A_g+A_b);

        L_use(:,:,1) = L_r*A_twice.*(1-t);
        L_use(:,:,2) = L_g*A_twice.*(1-t);
        L_use(:,:,3) = L_b*A_twice.*(1-t);

        % @计算defog2
        defog2 = (I_defog - Light*L_use)./t + G_detail;
        pic_out = defog2;
    */
    template<typename BufT>
    void process_w3_gpu(
        PContext& ctx,
        const BufT Light,
        const cv::cudev::GpuMat_<BufT>& t,
        const cv::cudev::GpuMat_<cv::Point3_<BufT>>& I_defog,
        const cv::cudev::GpuMat_<cv::Point3_<BufT>>& G_detail,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& pic_out,
        cv::cuda::Stream& stream
    ) {
        /*
            % 计算暗原色图层：取RGB三通道最小值
            % Mo Notes: 相当于 A_twice = min(I_defog, [], 3)
            tri = zeros(D,height*width);
            for ii = 1:D   % 转成二维数组
                a = I_defog(:,:,ii);
                tri(ii,:) = a(:)';
            end
            a_m = min(tri);
            A_twice = reshape(a_m,height,width);
        */
        int rows = I_defog.rows;
        int cols = I_defog.cols;
        dim3 threadsPerBlock(16, 16);
        dim3 numBlocks(
            ceil_div(cols,threadsPerBlock.x),
            ceil_div(rows,threadsPerBlock.y)
        );

        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        cuda_merge_3channels_with_min(I_defog, ctx.A_twice, stream);

        /*
            % 计算二次处理的 L_use
            A_r = mean(max(I_defog(:,:,1)));
            A_g = mean(max(I_defog(:,:,2)));
            A_b = mean(max(I_defog(:,:,3)));
            L_r = A_r/(A_r+A_g+A_b);
            L_g = A_g/(A_r+A_g+A_b);
            L_b = A_b/(A_r+A_g+A_b);
        */
        cv::Mat_<cv::Point3_<BufT>> mean_vec;
        {
            cv::cuda::reduce(I_defog, ctx.gpu_max_array, 0, cv::REDUCE_MAX);
            cv::cuda::reduce(ctx.gpu_max_array, ctx.gpu_mean_vec, 1, cv::REDUCE_AVG);
            ctx.gpu_mean_vec.download(mean_vec, stream);
            stream.waitForCompletion();
        }

        const cv::Point3_<BufT>& A_rgb = mean_vec.template at<cv::Point3_<BufT>>(0, 0);
        BufT sum_v = A_rgb.x + A_rgb.y + A_rgb.z;
        BufT L_r = A_rgb.x / sum_v;
        BufT L_g = A_rgb.y / sum_v;
        BufT L_b = A_rgb.z / sum_v;

        /*
        L_use(:,:,1) = L_r*A_twice.*(1-t);
        L_use(:,:,2) = L_g*A_twice.*(1-t);
        L_use(:,:,3) = L_b*A_twice.*(1-t);

        % @计算defog2
        defog2 = (I_defog - Light*L_use)./t + G_detail;
        pic_out = defog2;

        */
        pic_out.create(rows, cols);
        process_w3_krl<3, BufT><<<numBlocks, threadsPerBlock, 0, cuda_stream>>> (
           Light, L_r, L_g, L_b,
           t, ctx.A_twice, I_defog, G_detail, pic_out);
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
        cudaStreamSynchronize(cuda_stream);
    }

    template<typename BufT>
    void call_process_w3_gpu(
        PContext& ctx,
        const cv::cudev::GpuMat_<cv::Point3_<BufT>>& I_defog,
        const cv::cudev::GpuMat_<cv::Point3_<BufT>>& G_detail,
        cv::cudev::GpuMat_<cv::Point3_<BufT>>& pic_out,
        cv::cuda::Stream& stream)
    {
        process_w3_update_t_gpu<BufT>(ctx, ctx.t, stream);
        process_w3_gpu<BufT>(ctx, ctx.Light, ctx.t, I_defog, G_detail, pic_out, stream);
    }

    /*
    在gpu上， 按照像素点:
    1. 计算x方向的梯度计算dx;
    2. 计算y方向的梯度计算dy;
    3. 计算 dxdy = sqrt（(dx^2 + dy^2)/2） [复用 frame_gradient_sqrt]
    4. 计算 avg_dxdy = average_filter(dxdy, cx, cy)
    5. 计算 average =  (avg_dxdy(:,:,1)+avg_dxdy(:,:,2)+avg_dxdy(:,:,3))/3;
    6. 将结果按照像素点存回out中（中间缓冲)
    */
    int avg_SlideCurtain_gpu(
        PContext& ctx,
        const cv::cuda::GpuMat& img,
        const cv::cudev::GpuMat_<cv::Point3_<float>>& frame_gradient_sqrt,
        cv::cudev::GpuMat_<float>& out_avg,
        int wxy,
        cv::cuda::Stream& stream)
    {
        assert(img.channels() == 3);

        replicate_boxfilter_3ch(frame_gradient_sqrt, ctx.box_filter_out, ctx.f3_buf, wxy / 2, stream);
        APP_CUDEV_SAFE_CALL(cudaGetLastError());

        cuda_merge_3channels_with_avg(ctx.box_filter_out, out_avg, stream);
        //stream.waitForCompletion();
        APP_CUDEV_SAFE_CALL(cudaGetLastError());
        return 0;
    }

    void show_double_array(std::vector<double>& values) {
        for (int i = 0; i < values.size(); ++i) {
            std::cout << values[i] << ", ";
            if ((i + 1) % 10 == 0) {
                std::cout << std::endl;
            }
        }
    }

    template<typename T>
    bool cv_mat_all_eq(cv::Mat& mat, T value) {
        for (int row = 0; row < mat.rows; row += 1) {
            for (int col = 0; col < mat.cols; col += 1) {
                if (mat.at<T>(row, col) != value) {
                    return false;
                }
            }
        }
        return true;
    }

    void upload_img2gpu(const cv::Mat& frame_in, cv::cuda::GpuMat& float_frame, PContext& ctx, cv::cuda::Stream& stream) {
        ctx.upload_buf.upload(frame_in, stream);
        ctx.upload_buf.convertTo(float_frame, CV_32FC3, 1.0 / 255., stream);
    }

    void download_gpu2img(cv::cuda::GpuMat& frame, cv::Mat& frame_out, PContext& ctx, cv::cuda::Stream& stream) {
        CV_Assert(!frame.empty());
        if (ctx.upload_buf.empty() || ctx.upload_buf.size() != frame.size()) {
            ctx.upload_buf.create(frame.size(), CV_8UC3);
        }
        frame.convertTo(ctx.upload_buf, CV_8UC3, 255.0, stream);
        stream.waitForCompletion();
        ctx.upload_buf.download(frame_out);
        stream.waitForCompletion();
    }

    /*
    抽帧模块：对抽中帧计算平均梯度，为下个片段准备数据
    */
    void pre_extract(PContext& ctx, const cv::Mat& frame_in, cv::cudev::GpuMat_<cv::Point3_<float>>& frame_gpu_in, cv::cuda::Stream& stream) {
        if (ctx.i != ctx.n) {
            return;
        }
        if (!ctx.frame_gpu_freshed) {
            upload_img2gpu(frame_in, frame_gpu_in, ctx, stream);
            ctx.frame_gpu_freshed = true;
        }
        bool pre_frame_empty = ctx.pre_extract_frame.empty();
        if (pre_frame_empty) {
            frame_gpu_in.copyTo(ctx.pre_extract_frame);
        }
        ctx.a = avg_gradient_gpu(ctx, ctx.pre_extract_frame, ctx.frame_gradient_sqrt, stream);
        avg_SlideCurtain_gpu(ctx, ctx.pre_extract_frame, ctx.frame_gradient_sqrt, ctx.W, ctx.preExractWXY, stream);
        if (!pre_frame_empty) {
            frame_gpu_in.copyTo(ctx.pre_extract_frame);
        }
    }

    void calc_k_a0(PContext& ctx) {
        if (ctx.i == ctx.Unit) {
            if (ctx.a_n.empty()) {
                ctx.a_n.create(1, ctx.k);
            }
            ctx.a_n(0, ctx.j - 1) = ctx.a;
            if (ctx.j == ctx.k) {
                cv::Mat a0_sort;
                cv::sort(ctx.a_n, a0_sort, cv::SORT_EVERY_ROW | cv::SORT_DESCENDING);
                ctx.a0 = cv::mean(a0_sort.colRange(0, ctx.k / 2))[0];
                ctx.a0_star = ctx.a0;
                ctx.W0 = ctx.W.clone();
            }
        }
    }

void calc_way_judge(PContext& ctx) {
    if(ctx.i != 1) {
        return;
    }
    ctx.v_star = 1- ctx.a /ctx.a0_star;
    if(ctx.S.empty()) {
        ctx.S.create(1, ctx.f);
    }
    uint8_t s = 1;  //处理方式2 
    if(ctx.a > ctx.a0) {
        // 处理方式1
        s = 0;  
        // ctx.W0 = ctx.W.clone();
        // ctx.a0_star = ctx.a;
    }else if(ctx.v_star > ctx.v0) {
        // 处理方式3
        s = 2;
    }
    ctx.S.at<uint8_t>(0, ctx.p-1) = s;

    ctx.p+=1;
    if(ctx.p>ctx.f) {
        ctx.p = 1;
    }

    if(cv_mat_all_eq<uint8_t>(ctx.S, 0)) {
        ctx.way = 0;
    }else if(cv_mat_all_eq<uint8_t>(ctx.S, 1)) {
        ctx.way= 1;
    }else if(cv_mat_all_eq<uint8_t>(ctx.S, 2)) {
        ctx.way = 2;
    }
}
    
    template<typename BufT>
    void process_ways(PContext& ctx, const cv::Mat& frame_in, cv::cudev::GpuMat_<cv::Point3_<BufT>>& frame_gpu_in, cv::Mat& frame_out) {
        int way = ctx.way;
        if(ctx.way == 0){
            if(ctx.i==ctx.n){
                ctx.W0 = ctx.W.clone();
                ctx.a0_star = ctx.a;
            }
            frame_out = frame_in;
            return;
        }
        if(RemoverMethods::W11 == ctx.removerMethods){
            frame_out = frame_in;
            return;
        }
        if (!ctx.frame_gpu_freshed) {
            upload_img2gpu(frame_in, frame_gpu_in, ctx, ctx.stream);
            ctx.frame_gpu_freshed = true;
        }

        call_pre_step_gpu<BufT>(ctx, frame_gpu_in, ctx.G_detail, ctx.I_defog, ctx.stream);

        if ((RemoverMethods::W12 == ctx.removerMethods)) {
            way = 1;
        }
        else if (RemoverMethods::W13 == ctx.removerMethods) {
            way = 2;
        }

        if (1 == way) {
            call_process_w2_gpu<BufT>(ctx, ctx.I_defog, ctx.G_detail, ctx.pic_out, ctx.stream);
        }
        else if (2 == way) {
            call_process_w3_gpu<BufT>(ctx, ctx.I_defog, ctx.G_detail, ctx.pic_out, ctx.stream);
        }
        else {
            assert("Should not reached here!");
        }

        if (ctx.pic_out.empty() || ctx.pic_out.cols == 0 || ctx.pic_out.rows == 0) {
            frame_out = frame_in;
            return;
        }
        download_gpu2img(ctx.pic_out, frame_out, ctx, ctx.stream);
        ctx.stream.waitForCompletion();
    }

    int process_frame(PContext& ctx, const cv::Mat& frame_in, cv::Mat& frame_out) {
        ctx.frame_gpu_freshed = false;
        pre_extract(ctx, frame_in, ctx.frame_gpu_in, ctx.stream);
        if (ctx.j <= ctx.k) {
            calc_k_a0(ctx);
            frame_out = frame_in;
        }
        else {
            calc_way_judge(ctx);
            process_ways<float>(ctx, frame_in, ctx.frame_gpu_in, frame_out);
        }
        return 0;
    }

    void proc_ctx_update_idx(PContext& ctx) {
        ctx.i += 1;
        if (ctx.i > ctx.Unit) {
            ctx.i = 1;
            ctx.j += 1;
        }
    }

    CudaSmokeRemover::CudaSmokeRemover() {
        _ctx = new PContext();
    }

    CudaSmokeRemover::~CudaSmokeRemover() {
        delete _ctx;
        _ctx = nullptr;
    }

    int CudaSmokeRemover::getUnit() { return _ctx->Unit; };
    void CudaSmokeRemover::setUnit(int Unit) { _ctx->Unit = Unit; }

    double CudaSmokeRemover::getV0() { return _ctx->v0; }
    void CudaSmokeRemover::setV0(double v0) { _ctx->v0 = v0; }

    double CudaSmokeRemover::getLight() { return _ctx->Light; }
    void CudaSmokeRemover::setLight(double Light) { _ctx->Light = Light; }

    int CudaSmokeRemover::getK() { return _ctx->k; }
    void CudaSmokeRemover::setK(int k) { _ctx->k = k; }

    int CudaSmokeRemover::getF() { return _ctx->f; }
    void CudaSmokeRemover::setF(int f) { _ctx->f = f; }

    // @最小透过率（增加变量名），调节范围:[0,1]
    double CudaSmokeRemover::getTbase() { return _ctx->t_base; }
    void CudaSmokeRemover::setTbase(double t_base) { _ctx->t_base = t_base; }

    // @S分量调节范围:[0,1]
    double CudaSmokeRemover::getSIndex() { return _ctx->s_index2; }
    void CudaSmokeRemover::setSIndex(double s_index) { _ctx->s_index2 = s_index; }

    // @gamma变化
    double CudaSmokeRemover::getVGamma() { return _ctx->v_gamma; }
    void CudaSmokeRemover::setVGamma(double v_gamma) { _ctx->v_gamma = v_gamma; }

    // @v系数1（增加变量名），调节范围:[0,1]
    double CudaSmokeRemover::getVSimu1() { return _ctx->v_simu1; }
    void CudaSmokeRemover::setVSimu1(double v_simu1) { _ctx->v_simu1 = v_simu1; }

    // @v系数2（增加变量名），调节范围:[0,1]
    double CudaSmokeRemover::getVSimu2() { return _ctx->v_simu2; }
    void CudaSmokeRemover::setVSimu2(double v_simu2) { _ctx->v_simu2 = v_simu2; }

    // @v系数3（增加变量名），调节范围:[0,1]
    double CudaSmokeRemover::getVSimu3() { return _ctx->v_simu3; }
    void CudaSmokeRemover::setVSimu3(double v_simu3) { _ctx->v_simu3 = v_simu3;}

    // @细节强度（增加变量名），调节范围:[1,10]
    double CudaSmokeRemover::getFusionG() { return _ctx->fusion_G; }
    void CudaSmokeRemover::setFusionG(double fusion_G) { _ctx->fusion_G = fusion_G;}


    W2_VUpdate_Fun CudaSmokeRemover::getW2VUpdateFun() { return _ctx->vupdate_fun; }
    void CudaSmokeRemover::setW2VUpdateFun(W2_VUpdate_Fun vupdate_fun) { _ctx->vupdate_fun = vupdate_fun; }
    void CudaSmokeRemover::setW2VUpdateFun(int fun) { 
        switch (fun) {
        case 0:
            _ctx->vupdate_fun = F_Gamma;
            break;
        case 1:
            _ctx->vupdate_fun = F_Fitting;
            break; 
        default:
            _ctx->vupdate_fun = F_Fitting;
            break;
        }
     }

    RemoverMethods CudaSmokeRemover::getRemoverMethods() { return _ctx->removerMethods; }
    void CudaSmokeRemover::setRemoverMethods(RemoverMethods removerMethods) { _ctx->removerMethods = removerMethods; }
    void CudaSmokeRemover::setRemoverMethods(int way) { 
        switch (way) {
        case 0:
            _ctx->removerMethods = W11;
            break;
        case 1:
            _ctx->removerMethods = W12;
            break;
        case 2:
            _ctx->removerMethods = W13;
            break;
        case 3:
            _ctx->removerMethods = W123;
            break;          
        default:
            _ctx->removerMethods = W123;
            break;
        }
     }

    int CudaSmokeRemover::getPreExractWXY() { return _ctx->preExractWXY; }
    void CudaSmokeRemover::setPreExtractWXY(int preExractWX) { _ctx->preExractWXY = preExractWX; }

    void CudaSmokeRemover::process_one_frame(const cv::Mat input, cv::Mat& output) {
        if (_ctx != nullptr) {
            _ctx->frame_idx += 1;
            if (process_frame(*_ctx, input, output) == 0) {
                proc_ctx_update_idx(*_ctx);
            }
        }
    }

    void CudaSmokeRemover::reset_buf() {
        delete _ctx;
        _ctx = new PContext();
    }
}

