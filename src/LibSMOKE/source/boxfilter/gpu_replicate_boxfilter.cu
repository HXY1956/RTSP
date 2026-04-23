#include "gpu_util.h"
#include "gpu_replicate_boxfilter.h"


namespace SMOKE {
    // process row
    template<int CHN>
    __device__ void replicate_boxfilter_3ch_x(float* id, float* od, int w, int h, int r) {
        float scale = 1.0f / (float)((r << 1) + 1);

        float t0, t1, t2;
        // do left edge
        t0 = id[0] * r;
        t1 = id[1] * r;
        t2 = id[2] * r;

        for (int x = 0; x < (r + 1); x++) {
            t0 += id[x * CHN + 0];
            t1 += id[x * CHN + 1];
            t2 += id[x * CHN + 2];
        }

        od[0] = t0 * scale;
        od[1] = t1 * scale;
        od[2] = t2 * scale;

        for (int x = 1; x < (r + 1); x++) {
            t0 += id[(x + r) * CHN + 0];
            t1 += id[(x + r) * CHN + 1];
            t2 += id[(x + r) * CHN + 2];

            t0 -= id[0];
            t1 -= id[1];
            t2 -= id[2];

            od[x * CHN + 0] = t0 * scale;
            od[x * CHN + 1] = t0 * scale;
            od[x * CHN + 2] = t0 * scale;
        }

        // main loop
        for (int x = (r + 1); x < w - r; x++) {
            t0 += id[(x + r) * CHN + 0];
            t1 += id[(x + r) * CHN + 1];
            t2 += id[(x + r) * CHN + 2];

            t0 -= id[(x - r - 1) * CHN + 0];
            t1 -= id[(x - r - 1) * CHN + 1];
            t2 -= id[(x - r - 1) * CHN + 2];

            od[x * CHN + 0] = t0 * scale;
            od[x * CHN + 1] = t1 * scale;
            od[x * CHN + 2] = t2 * scale;
        }

        // do right edge
        for (int x = w - r; x < w; x++) {
            t0 += id[(w - 1) * CHN + 0];
            t1 += id[(w - 1) * CHN + 1];
            t2 += id[(w - 1) * CHN + 2];

            t0 -= id[(x - r - 1) * CHN + 0];
            t1 -= id[(x - r - 1) * CHN + 1];
            t2 -= id[(x - r - 1) * CHN + 2];

            od[x * CHN + 0] = t0 * scale;
            od[x * CHN + 1] = t1 * scale;
            od[x * CHN + 2] = t2 * scale;
        }
    }

    // process column
    __device__ void replicate_boxfilter_y(float* id, float* od, int in_w, int out_w, int h, int r) {
        float scale = 1.0f / (float)((r << 1) + 1);

        float t0, t1, t2;
        // do left edge
        t0 = id[0] * r;
        t1 = id[1] * r;
        t2 = id[2] * r;

        for (int y = 0; y < (r + 1); y++) {
            t0 += id[y * in_w + 0];
            t1 += id[y * in_w + 1];
            t2 += id[y * in_w + 2];
        }

        od[0] = t0 * scale;
        od[1] = t1 * scale;
        od[2] = t2 * scale;

        for (int y = 1; y < (r + 1); y++) {
            t0 += id[(y + r) * in_w + 0];
            t1 += id[(y + r) * in_w + 1];
            t2 += id[(y + r) * in_w + 2];

            t0 -= id[0];
            t1 -= id[1];
            t2 -= id[2];
            od[y * out_w + 0] = t0 * scale;
            od[y * out_w + 1] = t1 * scale;
            od[y * out_w + 2] = t2 * scale;
        }

        // main loop
        for (int y = (r + 1); y < (h - r); y++) {
            t0 += id[(y + r) * in_w + 0];
            t1 += id[(y + r) * in_w + 1];
            t2 += id[(y + r) * in_w + 2];

            t0 -= id[((y - r) * in_w) - in_w + 0];
            t1 -= id[((y - r) * in_w) - in_w + 1];
            t2 -= id[((y - r) * in_w) - in_w + 2];

            od[y * out_w + 0] = t0 * scale;
            od[y * out_w + 1] = t1 * scale;
            od[y * out_w + 2] = t2 * scale;
        }

        // do right edge
        for (int y = h - r; y < h; y++) {
            t0 += id[(h - 1) * in_w + 0];
            t1 += id[(h - 1) * in_w + 1];
            t2 += id[(h - 1) * in_w + 2];

            t0 -= id[((y - r) * in_w) - in_w + 0];
            t1 -= id[((y - r) * in_w) - in_w + 1];
            t2 -= id[((y - r) * in_w) - in_w + 2];

            od[y * out_w + 0] = t0 * scale;
            od[y * out_w + 1] = t1 * scale;
            od[y * out_w + 2] = t2 * scale;
        }
    }

    __global__ void replicate_boxfilter_3ch_x_krl(
        cv::cuda::PtrStepSz<float> in,
        cv::cuda::PtrStepSz<float> out, int r) {
        unsigned int y = blockIdx.x * blockDim.x + threadIdx.x;
        //replicate_boxfilter_x(&id[y * w], &od[y * w], w, h, r);
        if (y < in.rows) {
            replicate_boxfilter_3ch_x<3>(in.ptr(y), out.ptr(y), in.cols, in.rows, r);
        }
    }

    __global__ void replicate_boxfilter_3ch_y_krl(
        cv::cuda::PtrStepSz<float> in,
        cv::cuda::PtrStepSz<float> out, int r)
    {
        unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
        if (x < in.cols) {
            replicate_boxfilter_y(in.ptr(0) + x * 3, out.ptr(0) + x * 3, in.step / 4, out.step / 4, in.rows, r);
        }
    }

    int replicate_boxfilter_3ch(
        const cv::cudev::GpuMat_<cv::Point3_<float>>& input,
        cv::cudev::GpuMat_<cv::Point3_<float>>& out,
        cv::cudev::GpuMat_<cv::Point3_<float>>& tmp,
        int r,
        cv::cuda::Stream& stream)
    {
        const int nthreads = 64;

        int rows = input.rows;
        int cols = input.cols;
        tmp.create(rows, cols);
        out.create(rows, cols);
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        replicate_boxfilter_3ch_x_krl <<<ceil_div(rows, nthreads), nthreads, 0, cuda_stream >>> (input, tmp, r);
        replicate_boxfilter_3ch_y_krl <<<ceil_div(cols, nthreads), nthreads, 0, cuda_stream >>> (tmp, out, r);
        return 0;
    }
}


