#include "gpu_symmetric_boxfilter.h"
#include "gpu_util.h"

namespace SMOKE {
    __device__ void symmetric_boxfilter_x(float* id, float* od, int w, int h, int r) {
        float scale = 1.0f / (float)((r << 1) + 1);

        float t;
        // do left edge
        t = id[0];
        //first box
        for (int x = 1; x < (r + 1); x++) {
            t += 2 * id[x];
        }

        od[0] = t * scale;

        for (int x = 1; x < (r + 1); x++) {
            t += id[x + r];
            t -= id[r - x];
            od[x] = t * scale;
        }

        // main loop
        for (int x = (r + 1); x < w - r; x++) {
            t += id[x + r];
            t -= id[x - r - 1];
            od[x] = t * scale;
        }

        // do right edge
        int p = 2 * (w - 1) - r;
        for (int x = w - r; x < w; x++) {
            t += id[p - x];
            t -= id[x - r - 1];
            od[x] = t * scale;
        }
    }

    // process column
    __device__ void symmetric_boxfilter_y(float* id, float* od, int in_w, int out_w, int h, int r) {
        float scale = 1.0f / (float)((r << 1) + 1);

        float t;
        // do left edge
        t = id[0];

        //first box
        for (int y = 0; y < (r + 1); y++) {
            t += 2 * id[y * in_w];
        }

        od[0] = t * scale;


        for (int y = 1; y < (r + 1); y++) {
            t += id[(y + r) * in_w];
            t -= id[(r - y) * in_w];
            od[y * out_w] = t * scale;
        }

        // main loop
        for (int y = (r + 1); y < (h - r); y++) {
            t += id[(y + r) * in_w];
            t -= id[((y - r) * in_w) - in_w];
            od[y * out_w] = t * scale;
        }

        // do right edge
        int p = 2 * (h - 1) - r;
        for (int y = h - r; y < h; y++) {
            t += id[(p - y) * in_w];
            t -= id[(y - r - 1) * in_w];
            od[y * out_w] = t * scale;
        }
    }

    __global__ void symmetric_boxfilter_x_krl(
        cv::cuda::PtrStepSz<float> in,
        cv::cuda::PtrStepSz<float> out, int r) {
        unsigned int y = blockIdx.x * blockDim.x + threadIdx.x;
        //replicate_boxfilter_x(&id[y * w], &od[y * w], w, h, r);
        if (y < in.rows) {
            symmetric_boxfilter_x(in.ptr(y), out.ptr(y), in.cols, in.rows, r);
        }
    }

    __global__ void symmetric_boxfilter_y_krl(
        cv::cuda::PtrStepSz<float> in,
        cv::cuda::PtrStepSz<float> out, int r)
    {
        unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
        if (x < in.cols) {
            symmetric_boxfilter_y(in.ptr(0) + x, out.ptr(0) + x, in.step / 4, out.step / 4, in.rows, r);
        }
    }

    int symmetric_boxfilter(
        const cv::cudev::GpuMat_<float>& input,
        cv::cudev::GpuMat_<float>& out,
        cv::cudev::GpuMat_<float>& tmp,
        int r,
        cv::cuda::Stream& stream)
    {
        const int nthreads = 64;

        int rows = input.rows;
        int cols = input.cols;
        tmp.create(rows, cols);
        out.create(rows, cols);
        cudaStream_t cuda_stream = cv::cuda::StreamAccessor::getStream(stream);
        symmetric_boxfilter_x_krl <<<ceil_div(rows, nthreads), nthreads, 0, cuda_stream>>> (input, tmp, r);
        symmetric_boxfilter_y_krl <<<ceil_div(cols, nthreads), nthreads, 0, cuda_stream>>> (tmp, out, r);
        return 0;
    }
}



