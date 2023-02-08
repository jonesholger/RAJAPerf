//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-23, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

///
/// Methods for CUDA kernel data allocation, initialization, and deallocation.
///


#ifndef RAJAPerf_CudaDataUtils_HPP
#define RAJAPerf_CudaDataUtils_HPP

#include "RPTypes.hpp"
#include <stdexcept>

#if defined(RAJA_ENABLE_CUDA)

#include "common/RAJAPerfSuite.hpp"
#include "common/GPUUtils.hpp"

#include "RAJA/policy/cuda/raja_cudaerrchk.hpp"


namespace rajaperf
{

/*!
 * \brief Device timer, returns a time in ns from an arbitrary starting point.
 * Note that this time is consistent across the whole device.
 */
__device__ __forceinline__ unsigned long long device_timer()
{
  unsigned long long global_timer = 0;
#if __CUDA_ARCH__ >= 300
  asm volatile ("mov.u64 %0, %globaltimer;" : "=l"(global_timer));
#endif
  return global_timer;
}

/*!
 * \brief Simple forall cuda kernel that runs a lambda.
 */
template < typename Lambda >
__global__ void lambda_cuda_forall(Index_type ibegin, Index_type iend, Lambda body)
{
  Index_type i = ibegin + blockIdx.x * blockDim.x + threadIdx.x;
  if (i < iend) {
    body(i);
  }
}
///
template < size_t block_size, typename Lambda >
__launch_bounds__(block_size)
__global__ void lambda_cuda_forall(Index_type ibegin, Index_type iend, Lambda body)
{
  Index_type i = ibegin + blockIdx.x * block_size + threadIdx.x;
  if (i < iend) {
    body(i);
  }
}

/*!
 * \brief Simple cuda kernel that runs a lambda.
 */
template < typename Lambda >
__global__ void lambda_cuda(Lambda body)
{
  body();
}
///
template < size_t block_size, typename Lambda >
__launch_bounds__(block_size)
__global__ void lambda_cuda(Lambda body)
{
  body();
}

/*!
 * \brief Getters for cuda kernel indices.
 */
template < typename Index >
__device__ inline Index_type lambda_cuda_get_index();

template < >
__device__ inline Index_type lambda_cuda_get_index<RAJA::cuda_thread_x_direct>() {
  return threadIdx.x;
}
template < >
__device__ inline Index_type lambda_cuda_get_index<RAJA::cuda_thread_y_direct>() {
  return threadIdx.y;
}
template < >
__device__ inline Index_type lambda_cuda_get_index<RAJA::cuda_thread_z_direct>() {
  return threadIdx.z;
}

template < >
__device__ inline Index_type lambda_cuda_get_index<RAJA::cuda_block_x_direct>() {
  return blockIdx.x;
}
template < >
__device__ inline Index_type lambda_cuda_get_index<RAJA::cuda_block_y_direct>() {
  return blockIdx.y;
}
template < >
__device__ inline Index_type lambda_cuda_get_index<RAJA::cuda_block_z_direct>() {
  return blockIdx.z;
}

/*!
 * \brief Copy given hptr (host) data to CUDA device (dptr).
 *
 * Method assumes both host and device data arrays are allocated
 * and of propoer size for copy operation to succeed.
 */
template <typename T>
void initCudaDeviceData(T& dptr, const T hptr, int len)
{
  cudaErrchk( cudaMemcpy( dptr, hptr,
                          len * sizeof(typename std::remove_pointer<T>::type),
                          cudaMemcpyHostToDevice ) );

  incDataInitCount();
}

/*!
 * \brief Allocate CUDA device data array (dptr).
 */
template <typename T>
void allocCudaDeviceData(T& dptr, int len)
{
  cudaErrchk( cudaMalloc( (void**)&dptr,
              len * sizeof(typename std::remove_pointer<T>::type) ) );
}

/*!
 * \brief Allocate CUDA managed data array (dptr).
 */
template <typename T>
void allocCudaManagedData(T& mptr, int len)
{
  cudaErrchk( cudaMallocManaged( (void**)&mptr,
              len * sizeof(typename std::remove_pointer<T>::type),
              cudaMemAttachGlobal ) );
}

/*!
 * \brief Allocate CUDA pinned data array (pptr).
 */
template <typename T>
void allocCudaPinnedData(T& pptr, int len)
{
  cudaErrchk( cudaHostAlloc( (void**)&pptr,
              len * sizeof(typename std::remove_pointer<T>::type),
              cudaHostAllocMapped ) );
}

/*!
 * \brief Allocate CUDA device data array (dptr) and copy given hptr (host)
 * data to device array.
 */
template <typename T>
void allocAndInitCudaDeviceData(T& dptr, const T hptr, int len)
{
  allocCudaDeviceData(dptr, len);
  initCudaDeviceData(dptr, hptr, len);
}

/*!
 * \brief Copy given dptr (CUDA device) data to host (hptr).
 *
 * Method assumes both host and device data arrays are allocated
 * and of propoer size for copy operation to succeed.
 */
template <typename T>
void getCudaDeviceData(T& hptr, const T dptr, int len)
{
  cudaErrchk( cudaMemcpy( hptr, dptr,
              len * sizeof(typename std::remove_pointer<T>::type),
              cudaMemcpyDeviceToHost ) );
}

/*!
 * \brief Free device data array.
 */
template <typename T>
void deallocCudaDeviceData(T& dptr)
{
  cudaErrchk( cudaFree( dptr ) );
  dptr = nullptr;
}

/*!
 * \brief Free managed data array.
 */
template <typename T>
void deallocCudaManagedData(T& mptr)
{
  cudaErrchk( cudaFree( mptr ) );
  mptr = nullptr;
}

/*!
 * \brief Free pinned data array.
 */
template <typename T>
void deallocCudaPinnedData(T& pptr)
{
  cudaErrchk( cudaFreeHost( pptr ) );
  pptr = nullptr;
}


/*!
 * \brief Copy given hptr (host) data to CUDA (cptr).
 *
 * Method assumes both host and device data arrays are allocated
 * and of proper size for copy operation to succeed.
 */
template <typename T>
void initCudaData(CudaDataSpace, T& cptr, const T hptr, int len)
{
  cudaErrchk( cudaMemcpy( cptr, hptr,
                          len * sizeof(typename std::remove_pointer<T>::type),
                          cudaMemcpyDefault ) );

  incDataInitCount();
}

/*!
 * \brief Allocate CUDA data array (cptr).
 */
template <typename T>
void allocCudaData(CudaDataSpace cudaDataSpace, T& cptr, int len)
{
  switch (cudaDataSpace) {
    case CudaDataSpace::Host:
    {
      allocData(cptr, len);
    } break;
    case CudaDataSpace::Pinned:
    {
      allocCudaPinnedData(cptr, len);
    } break;
    case CudaDataSpace::Managed:
    {
      allocCudaManagedData(cptr, len);
    } break;
    case CudaDataSpace::Device:
    {
      allocCudaDeviceData(cptr, len);
    } break;
    default:
    {
      throw std::invalid_argument("allocCudaData : Unknown memory type");
    } break;
  }
}

/*!
 * \brief Allocate CUDA data array (cptr) and copy given hptr (host)
 * data to CUDA array.
 */
template <typename T>
void allocAndInitCudaData(CudaDataSpace cudaDataSpace, T& cptr, const T hptr, int len)
{
  allocCudaData(cudaDataSpace, cptr, len);
  initCudaData(cudaDataSpace, cptr, hptr, len);
}

/*!
 * \brief Free Cuda data array.
 */
template <typename T>
void deallocCudaData(CudaDataSpace cudaDataSpace, T& cptr)
{
  switch (cudaDataSpace) {
    case CudaDataSpace::Host:
    {
      deallocData(cptr);
    } break;
    case CudaDataSpace::Pinned:
    {
      deallocCudaPinnedData(cptr);
    } break;
    case CudaDataSpace::Managed:
    {
      deallocCudaManagedData(cptr);
    } break;
    case CudaDataSpace::Device:
    {
      deallocCudaDeviceData(cptr);
    } break;
    default:
    {
      throw std::invalid_argument("deallocCudaData : Unknown memory type");
    } break;
  }
}

/*!
 * \brief Copy given cptr (CUDA) data to host (hptr).
 *
 * Method assumes both host and device data arrays are allocated
 * and of propoer size for copy operation to succeed.
 */
template <typename T>
void getCudaData(CudaDataSpace, T& hptr, const T cptr, int len)
{
  cudaErrchk( cudaMemcpy( hptr, cptr,
              len * sizeof(typename std::remove_pointer<T>::type),
              cudaMemcpyDefault ) );
}

}  // closing brace for rajaperf namespace

#endif // RAJA_ENABLE_CUDA

#endif  // closing endif for header file include guard
