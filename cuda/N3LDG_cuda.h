#ifndef N3LDG_CUDA_N3LDG_CUDA_H
#define N3LDG_CUDA_N3LDG_CUDA_H

#include "Def.h"

#include "Memory_cuda.h"
#include <iostream>
#include <cassert>
#include <cuda.h>
#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <vector>
#include <cmath>

namespace n3ldg_cuda {

template<typename T>
struct GPUArray {
    T *value = nullptr;
    int len = 0;

    GPUArray() = default;
    GPUArray(GPUArray<T>&&) {
        abort();
    }
    GPUArray(const GPUArray &) {
        abort();
    }

    void init(const T *host_arr, int len);
    void init(int len);
    ~GPUArray();

    vector<T> toCpu() const;
};

cudaError_t MyCudaMemcpy(void *dest, const void *src, size_t count, cudaMemcpyKind kind);
void CallCuda(cudaError_t status);

template <typename T>
void GPUArray<T>::init(const T *host_arr, int len) {
    if (value != nullptr) {
        CallCuda(MemoryPool::Ins().Free(value));
        value = nullptr;
    }
    CallCuda(MemoryPool::Ins().Malloc((void**)&value, len * sizeof(T)));
    CallCuda(MyCudaMemcpy(value, host_arr, len * sizeof(T), cudaMemcpyHostToDevice));
    this->len = len;
}

template <typename T>
void GPUArray<T>::init(int len) {
    if (value != nullptr) {
        CallCuda(MemoryPool::Ins().Free(value));
        value = nullptr;
    }
    CallCuda(MemoryPool::Ins().Malloc((void**)&value, len * sizeof(T)));
    this->len = len;
}

template <typename T>
vector<T> GPUArray<T>::toCpu() const {
    vector<T> result;
    result.resize(len);
    if (value == nullptr) {
        cerr << "GPUArray::toCpu - value is nullptr" << endl;
        abort();
    }
    CallCuda(MyCudaMemcpy(result.data(), value, sizeof(T) * len, cudaMemcpyDeviceToHost));
    return result;
}

template <typename T>
GPUArray<T>::~GPUArray() {
    if (value != nullptr) {
        CallCuda(MemoryPool::Ins().Free(value));
        value = nullptr;
    }
}

typedef GPUArray<dtype> NumberArray;
typedef GPUArray<bool> BoolArray;
typedef GPUArray<const bool *> BoolPointerArray;
typedef GPUArray<const dtype *> NumberPointerArray;
typedef GPUArray<const dtype *const *> NumberPointerPointerArray;
typedef GPUArray<int> IntArray;
typedef GPUArray<const int *> IntPointerArray;

struct DeviceNumber {
    dtype *value = nullptr;
    dtype v = 0.0f;

    DeviceNumber() = default;
    DeviceNumber(DeviceNumber &&) {
        abort();
    }
    DeviceNumber(const DeviceNumber&) {
        abort();
    }

    void init();
    void copyFromDeviceToHost();
    ~DeviceNumber();
};

struct DeviceInt {
    int *value = nullptr;
    int v = 0;

    DeviceInt() = default;
    DeviceInt(DeviceInt &&) {
        abort();
    }
    DeviceInt(const DeviceInt&) {
        abort();
    }

    void init();
    void copyFromDeviceToHost();
    void copyFromHostToDevice();
    ~DeviceInt();
};

bool Verify(bool *host, bool *device, int len, const char* message);
bool Verify(int *host, int *device, int len, const char* message);

void Assert(bool v, const string &message = "",
        const function<void(void)> &call = []() {});
void Memset(dtype *p, int len, dtype value);
void Memset(bool *p, int len, bool value);
void *Malloc(int size);
void BatchMemset(const vector<dtype*> &vec, int count, const vector<int> &dims, dtype value);
void PrintNums(const dtype* p, int len);
void PrintInts(const int* p, int len);

void InitCuda(int device_id = 0, float memory_in_gb = 0.0f);
void EndCuda();

void CopyFromMultiVectorsToOneVector(const vector<dtype*> &src, dtype *dest, int count,
        int len);
void CopyFromOneVectorToMultiVals(const dtype *src, vector<dtype*> &vals,
        int count,
        int len);
void CopyFromHostToDevice(const vector<dtype*> &src,
        vector<dtype*> &dest, int count, int dim);
void CopyFromDeviceToHost(const vector<dtype*> &src,
        vector<dtype*> &dest, int count, int dim);

enum PoolingEnum {
    MAX,
    MIN,
    SUM,
    AVG
};

void ActivationForward(ActivatedEnum activated, const vector<const dtype*> &xs,
        int count,
        const vector<int> &dims,
        vector<dtype*> &ys);
void ActivationBackward(ActivatedEnum activated, const vector<const dtype*> &losses,
        const vector<dtype*> &vals,
        int count,
        const vector<int> &dims,
        vector<dtype*> &in_losses);
void DropoutForward(const vector<dtype*> &xs, int count, int dim,
        bool is_training,
        const dtype *drop_mask,
        dtype drop_factor,
        vector<dtype*> &ys);
void DropoutBackward(const vector<dtype*> &losses,
        const vector<dtype*> &vals,
        int count,
        int dim,
        bool is_training,
        const dtype *drop_mask,
        dtype drop_factor,
        vector<dtype*> &in_losses);
void BucketForward(const vector<dtype> input, int count, int dim, vector<dtype*> &ys);
void CopyForUniNodeForward(const vector<dtype*> &xs, const dtype* b,
        dtype* xs_dest,
        dtype* b_dest,
        int count,
        int x_len,
        int b_len,
        bool use_b);
void MatrixMultiplyMatrix(dtype *W, dtype *x, dtype *y, int row, int col,
        int count,
        bool useb,
        bool should_x_transpose = false,
        bool should_W_transpose = false);
void AddLtyToParamBiasAndAddLxToInputLossesForUniBackward(const dtype *lty,
        const dtype *lx, dtype *b, vector<dtype*> &losses, int count,
        int out_dim, int in_dim, bool use_b);
void AddLtyToParamBiasAndAddLxToInputLossesForBiBackward(const dtype *lty,
        const dtype *lx1,
        const dtype *lx2,
        dtype *b,
        vector<dtype*> &losses1,
        vector<dtype*> &losses2,
        int count,
        int out_dim,
        int in_dim1,
        int in_dim2,
        bool use_b);
void CalculateDropoutMask(dtype dropout_ratio, int count, int dim, dtype *mask);
void ConcatForward(const vector<dtype*> &in_vals,
        const vector<int> &in_dims,
        vector<dtype*> &vals,
        int count,
        int in_count,
        int out_dim);
void ConcatBackward(const vector<dtype*> &in_losses,
        const vector<int> &in_dims,
        vector<dtype*> &losses,
        int count,
        int in_count,
        int out_dim);
void ScalarConcatForward(const vector<dtype *> &ins, int count, const vector<int> &dims,
        int max_dim,
        const vector<dtype *> &results);
void ScalarConcatBackward(const vector<dtype *> &losses, int count, const vector<int> &dims,
        int max_dim,
        const vector<dtype *> in_losses);
void LookupForward(const vector<int> &xids, const dtype *vocabulary,
        int count,
        int dim,
        vector<dtype*> &vals);
void LookupBackward(const vector<int> &xids,
        const vector<int> &should_backward,
        const vector<dtype*> &losses,
        int count,
        int dim,
        dtype *grad,
        bool *indexers);
void LookupBackward(const vector<int> &xids, const vector<int> &should_backward,
        const vector<dtype*> &losses,
        int count,
        int dim,
        dtype *grad);
void ParamRowForward(const dtype *param, int row_index, int param_row_count, int count, int dim,
        vector<dtype*> &vals);
void PoolForward(PoolingEnum pooling, const vector<dtype*> &in_vals,
        vector<dtype*> &vals,
        int count,
        const vector<int> &in_counts,
        int dim,
        int *hit_inputs);
void PoolBackward(const vector<dtype*> &losses,
        vector<dtype*> &in_losses,
        const vector<int> &in_counts,
        const int *hit_inputs,
        int count,
        int dim);
void SumPoolForward(PoolingEnum pooling, const vector<dtype*> &in_vals,
        int count,
        int dim,
        const vector<int> &in_counts,
        vector<dtype*> &vals);
void SumPoolBackward(PoolingEnum pooling, const vector<dtype*> &losses,
        const vector<int> &in_counts,
        int count,
        int dim,
        vector<dtype*> &in_losses);
void MaxtrixConcatForward(const vector<dtype*> &in_vals, int count, int in_dim,
        const vector<int> &in_counts,
        vector<dtype*> &vals);
void PMultiForward(const vector<dtype*> &ins1,
        const vector<dtype*> &ins2,
        int count,
        int dim,
        vector<dtype*> &vals);
void DivForward(const vector<const dtype*> numerators, const vector<const dtype*> denominators,
        int count,
        const vector<int> &dims,
        vector<dtype*> &results);
void DivBackward(const vector<const dtype*> &losses, const vector<const dtype*> &denominator_vals,
        const vector<const dtype*> &numerator_vals,
        int count,
        const vector<int> &dims,
        vector<dtype*> &numerator_losses,
        vector<dtype*> &denominator_losses);
void FullDivForward(const vector<const dtype*> numerators,
        const vector<const dtype*> denominators,
        int count,
        int dim,
        vector<dtype*> &results);
void FullDivBackward(const vector<const dtype*> &losses,
        const vector<const dtype*> &denominator_vals,
        const vector<const dtype*> &numerator_vals,
        int count,
        int dim,
        vector<dtype*> &numerator_losses,
        vector<dtype*> &denominator_losses);
void SplitForward(const vector<const dtype*> &inputs, const vector<int> &offsets,
        int count,
        int dim,
        vector<dtype*> &results);
void SplitBackward(const vector<const dtype*> &losses, const vector<int> offsets,
        int count,
        int dim,
        const vector<dtype*> &input_losses);
void SubForward(const vector<const dtype*> &minuend,
        const vector<const dtype*> &subtrahend,
        int count,
        const vector<int> &dims,
        vector<dtype*> &results);
void SubBackward(const vector<const dtype*> &losses, int count, const vector<int> &dims,
        vector<dtype*> &minuend_losses,
        vector<dtype*> &subtrahend_losses);
void PMultiBackward(const vector<dtype*> &losses,
        const vector<dtype*> &in_vals1,
        const vector<dtype*> &in_vals2,
        int count,
        int dim,
        vector<dtype*> &in_losses1,
        vector<dtype*> &in_losses2);
void PAddForward(const vector<vector<dtype*>> &ins, int count,
        int dim,
        int in_count,
        vector<dtype*> &vals);
void PAddBackward(const vector<dtype*> &losses, int count, int dim,
        int in_count,
        vector<vector<dtype*>> &in_losses);
dtype CrossEntropyLoss(const vector<dtype *> &vals, const vector<int> &answers, int count,
        dtype batchsize,
        vector<dtype *> &losses);
dtype MultiCrossEntropyLoss(const vector<dtype*> &vals, const vector<vector<int>> &answers,
        int count,
        int dim,
        dtype factor,
        const vector<dtype*> &losses);
dtype KLCrossEntropyLoss(const vector<dtype*> &vals,
        const vector<shared_ptr<vector<dtype>>> &answers,
        int count,
        int dim,
        dtype factor,
        const vector<dtype*> &losses);
void MaxScalarForward(const vector<const dtype*> &inputs, int count, const vector<int> &dims,
        vector<dtype*> &results,
        vector<int> &max_indexes);
void MaxScalarBackward(const vector<const dtype *> &losses, const vector<int> &indexes,
        int count,
        const vector<dtype*> &input_losses);
void VectorSumForward(const vector<const dtype *> &inputs, int count, const vector<int> &dims,
        vector<dtype*> &results);
void VectorSumBackward(const vector<const dtype*> &losses, int count, const vector<int> &dims,
        vector<dtype*> &input_losses);
void ScalarToVectorForward(const vector<const dtype*> &inputs, int count, const vector<int> &dims,
        vector<dtype*> &results);
void ScalarToVectorBackward(const vector<const dtype*> &losses, int count, const vector<int> &dims,
        vector<dtype*> &input_losses);
void BiasForward(const vector<dtype*> &in_vals, const dtype *bias, int count, int dim,
        const vector<dtype *> &vals);
void BiasBackward(const vector<dtype *> &losses, int count, int dim, dtype *bias_loss,
        const vector<dtype *> input_losses);
vector<int> Predict(const vector<dtype*> &vals, int count, int dim);
int Predict(const dtype* val, int dim);
void Max(const dtype *const *v, int count, int dim, int *max_indexes, dtype *max_vals);
pair<dtype, vector<int>> SoftMaxLoss(const vector<const dtype *> &vals_vector,
        int count,
        int dim,
        const vector<int> &gold_answers,
        int batchsize,
        const vector<dtype *> &losses_vector);
dtype SquareSum(const dtype *v, int len);
dtype SquareSum(const dtype *v, const bool *indexers, int count, int dim);
void Rescale(dtype *v, int len, dtype scale);
void UpdateAdam(dtype *val, dtype *grad, int row, int col, bool is_bias, dtype *aux_mean,
        dtype *aux_square,
        int iter,
        dtype belta1,
        dtype belta2,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdam(dtype *val, dtype *grad, int row, int col, dtype *aux_mean,
        dtype *aux_square,
        const bool *indexers,
        int *iters,
        dtype belta1,
        dtype belta2,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdamW(dtype *val, dtype *grad, int row, int col, bool is_bias, dtype *aux_mean,
        dtype *aux_square,
        int iter,
        dtype belta1,
        dtype belta2,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdagrad(dtype *val, dtype *grad, int row, int col,
        dtype *aux_square,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdagrad(dtype *val, dtype *grad, int row, int col,
        dtype *aux_square,
        const bool *indexers,
        dtype alpha,
        dtype reg,
        dtype eps);
void *GraphHostAlloc();
}

#endif
