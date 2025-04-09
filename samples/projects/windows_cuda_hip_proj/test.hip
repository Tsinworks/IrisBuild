
#if defined(_MSC_VER) && defined(__INTELLISENSE__)
#define __CLANG_HIP_RUNTIME_WRAPPER_INCLUDED__
#define __restrict__
#define __inline__ __inline
#define __no_return__ __declspec(noreturn)
#define __noinline__ __declspec(noinline)
#define __forceinline__ __forceinline
#define __align__(n) __declspec(align(n))
#define __thread__ __declspec(thread)
#define __import__ __declspec(dllimport)
#define __export__ __declspec(dllexport)
#define __annotate__(a) __declspec(a)
#define __location__(a) __annotate__(__##a##__)
#define __host__ __location__(host)
#define __device__ __location__(device)
#define __global__ __location__(global)
#define __shared__ __location__(shared)
#define __constant__ __location__(constant)
#define __attribute__(x)
#endif

#include <hip/hip_runtime.h>
#include <iostream>
#include <string>

#define HIP_CHECK(status)                                                                                     \
    if (status != hipSuccess) {                                                                               \
        fprintf(stderr, "error: '%s'(%d) at %s:%d\n", hipGetErrorString(status), status, __FILE__, __LINE__); \
        exit(0);                                                                                              \
    }

// Device (Kernel) function, it must be void
__global__ void matrixTranspose(float* out, float* in, const int width) {
  int x = blockDim.x * blockIdx.x + threadIdx.x;
  int y = blockDim.y * blockIdx.y + threadIdx.y;
  out[y * width + x] = in[x * width + y];
}

// CPU implementation of matrix transpose
void matrixTransposeCPU(float* output, float* input, const unsigned int width) {
  for (unsigned int j = 0; j < width; ++j) {
    for (unsigned int i = 0; i < width; ++i) {
      output[i * width + j] = input[j * width + i];
    }
  }
}

int hip_main() {
  hipDeviceProp_t devProp;
  HIP_CHECK(hipGetDeviceProperties(&devProp, 0));
  std::cout << "Device name " << std::string(devProp.name) << std::endl;
  const uint32_t WIDTH = 64;
  const uint32_t NUM = WIDTH * WIDTH;
  float* Matrix = (float*)malloc(NUM * sizeof(float));
  float* TransposeMatrix = (float*)malloc(NUM * sizeof(float));
  float* cpuTransposeMatrix = (float*)malloc(NUM * sizeof(float));
  // Initialize the input data
  for (uint32_t i = 0; i < NUM; ++i) {
    Matrix[i] = (float)i * 10.987654f;
  }
  // MatrixTranspose computation on CPU
  matrixTransposeCPU(cpuTransposeMatrix, Matrix, WIDTH);
  float* gpuMatrix = 0;
  float* gpuTransposeMatrix = 0;
  // Allocate the memory on device side
  HIP_CHECK(hipMalloc((void**)&gpuMatrix, NUM * sizeof(float)));
  HIP_CHECK(hipMalloc((void**)&gpuTransposeMatrix, NUM * sizeof(float)));
  // Memory transfer from host to device
  HIP_CHECK(hipMemcpy(gpuMatrix, Matrix, NUM * sizeof(float), hipMemcpyHostToDevice));
  const uint32_t THREADS_PER_BLOCK_X = 4;
  const uint32_t THREADS_PER_BLOCK_Y = 4;
  const uint32_t THREADS_PER_BLOCK_Z = 1;
  const uint32_t GRID_X = uint32_t(WIDTH / THREADS_PER_BLOCK_X);
  const uint32_t GRID_Y = uint32_t(WIDTH / THREADS_PER_BLOCK_Y);
  dim3 dimGrid(GRID_X, GRID_Y);
  dim3 dimBlock(THREADS_PER_BLOCK_X, THREADS_PER_BLOCK_Y, THREADS_PER_BLOCK_Z);
  // Lauching kernel from host
  matrixTranspose<<<dimGrid, dimBlock, 0, 0>>>(gpuTransposeMatrix, gpuMatrix, WIDTH);
  // Memory transfer from device to host
  HIP_CHECK(hipMemcpy(TransposeMatrix, gpuTransposeMatrix, NUM * sizeof(float), hipMemcpyDeviceToHost));
  for (uint32_t i = 0; i < NUM; ++i) {
    printf("Matrix[%d]: %.6f  |  TransposeMatrix[%d]: %.6f  |  cpuTransposeMatrix[%d]: %.6f\n", i, Matrix[i], i, TransposeMatrix[i], i, cpuTransposeMatrix[i]);
  }
  int errors = 0;
  const float eps = 1.0E-6;
  printf("The margin of error = %.6f\n", eps);
  // Verify the results
  for (uint32_t i = 0; i < NUM; ++i) {
    float delta = std::abs(TransposeMatrix[i] - cpuTransposeMatrix[i]);
    if (delta > eps)
    {
      printf("%.8f\n", delta);
      errors++;
    }
  }
  if (errors != 0) {
    printf("FAILED: %d errors\n", errors);
  } else {
    printf("PASSED!\n");
  }
  // Free the resources on device side
  HIP_CHECK(hipFree(gpuMatrix));
  HIP_CHECK(hipFree(gpuTransposeMatrix));
  // Free the resources on host side
  free(Matrix);
  free(TransposeMatrix);
  free(cpuTransposeMatrix);
  return errors;
}
