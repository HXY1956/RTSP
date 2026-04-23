#ifndef __GPU_SYMMETRIC_BOXFILTER_H
#define __GPU_SYMMETRIC_BOXFILTER_H
#include <cuda_runtime.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/cuda_types.hpp>
#include <device_launch_parameters.h>
#include <opencv2/cudev/ptr2d/gpumat.hpp> 

namespace SMOKE {
    __device__ void symmetric_boxfilter_x(float* id, float* od, int w, int h, int r);
    __device__ void symmetric_boxfilter_y(float* id, float* od, int in_w, int out_w, int h, int r);
    __global__ void symmetric_boxfilter_x_krl(
        cv::cuda::PtrStepSz<float> in,
        cv::cuda::PtrStepSz<float> out, int r);
    __global__ void symmetric_boxfilter_y_krl(
        cv::cuda::PtrStepSz<float> in,
        cv::cuda::PtrStepSz<float> out, int r);
    int symmetric_boxfilter(
        const cv::cudev::GpuMat_<float>& input,
        cv::cudev::GpuMat_<float>& out,
        cv::cudev::GpuMat_<float>& tmp,
        int r,
        cv::cuda::Stream& stream);
}

#endif
