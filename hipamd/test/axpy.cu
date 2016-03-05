// RUN: hipify "%s" -o=%t --
// RUN: FileCheck %s -input-file=%t --match-full-lines

#include <iostream>

__global__ void axpy(float a, float* x, float* y) {
  // CHECK: y[hipThreadIdx_x] = a * x[hipThreadIdx_x];
  y[threadIdx.x] = a * x[threadIdx.x];
}

int main(int argc, char* argv[]) {
  const int kDataLen = 4;

  float a = 2.0f;
  float host_x[kDataLen] = {1.0f, 2.0f, 3.0f, 4.0f};
  float host_y[kDataLen];

  // Copy input data to device.
  float* device_x;
  float* device_y;
  cudaMalloc(&device_x, kDataLen * sizeof(float));
  cudaMalloc(&device_y, kDataLen * sizeof(float));
  cudaMemcpy(device_x, host_x, kDataLen * sizeof(float), cudaMemcpyHostToDevice);

  // Launch the kernel.
  // CHECK: hipLaunchKernel(HIP_KERNEL_NAME(axpy),  dim3(1), dim3(kDataLen), 0, 0, a, device_x, device_y);
  axpy<<<1, kDataLen>>>(a, device_x, device_y);

  // Copy output data to host.
  cudaDeviceSynchronize();
  cudaMemcpy(host_y, device_y, kDataLen * sizeof(float), cudaMemcpyDeviceToHost);

  // Print the results.
  for (int i = 0; i < kDataLen; ++i) {
    std::cout << "y[" << i << "] = " << host_y[i] << "\n";
  }

  cudaDeviceReset();
  return 0;
}
