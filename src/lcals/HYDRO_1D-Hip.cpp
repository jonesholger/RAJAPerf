//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-21, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "HYDRO_1D.hpp"

#include "RAJA/RAJA.hpp"

#if defined(RAJA_ENABLE_HIP)

#include "common/HipDataUtils.hpp"

#include <iostream>

namespace rajaperf
{
namespace lcals
{

#define HYDRO_1D_DATA_SETUP_HIP \
  allocAndInitHipDeviceData(x, m_x, m_array_length); \
  allocAndInitHipDeviceData(y, m_y, m_array_length); \
  allocAndInitHipDeviceData(z, m_z, m_array_length);

#define HYDRO_1D_DATA_TEARDOWN_HIP \
  getHipDeviceData(m_x, x, m_array_length); \
  deallocHipDeviceData(x); \
  deallocHipDeviceData(y); \
  deallocHipDeviceData(z); \

template < size_t block_size >
__launch_bounds__(block_size)
__global__ void hydro_1d(Real_ptr x, Real_ptr y, Real_ptr z,
                         Real_type q, Real_type r, Real_type t,
                         Index_type iend)
{
   Index_type i = blockIdx.x * block_size + threadIdx.x;
   if (i < iend) {
     HYDRO_1D_BODY;
   }
}


template < size_t block_size >
void HYDRO_1D::runHipVariantImpl(VariantID vid)
{
  const Index_type run_reps = getRunReps();
  const Index_type ibegin = 0;
  const Index_type iend = getActualProblemSize();

  HYDRO_1D_DATA_SETUP;

  if ( vid == Base_HIP ) {

    HYDRO_1D_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

       const size_t grid_size = RAJA_DIVIDE_CEILING_INT(iend, block_size);
       hipLaunchKernelGGL((hydro_1d<block_size>), dim3(grid_size), dim3(block_size), 0, 0,  x, y, z,
                                            q, r, t,
                                            iend );
       hipErrchk( hipGetLastError() );

    }
    stopTimer();

    HYDRO_1D_DATA_TEARDOWN_HIP;

  } else if ( vid == RAJA_HIP ) {

    HYDRO_1D_DATA_SETUP_HIP;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

       RAJA::forall< RAJA::hip_exec<block_size, true /*async*/> >(
         RAJA::RangeSegment(ibegin, iend), [=] __device__ (Index_type i) {
         HYDRO_1D_BODY;
       });

    }
    stopTimer();

    HYDRO_1D_DATA_TEARDOWN_HIP;

  } else {
     getCout() << "\n  HYDRO_1D : Unknown Hip variant id = " << vid << std::endl;
  }
}

void HYDRO_1D::runHipVariant(VariantID vid, size_t tid)
{
  size_t t = 0;
  seq_for(gpu_block_sizes_type{}, [&](auto block_size) {
    if (run_params.numValidGPUBlockSize() == 0u ||
        run_params.validGPUBlockSize(block_size)) {
      if (tid == t) {
        runHipVariantImpl<block_size>(vid);
      }
      t += 1;
    }
  });
}

void HYDRO_1D::setHipTuningDefinitions(VariantID vid)
{
  seq_for(gpu_block_sizes_type{}, [&](auto block_size) {
    if (run_params.numValidGPUBlockSize() == 0u ||
        run_params.validGPUBlockSize(block_size)) {
      addVariantTuningName(vid, "block_"+std::to_string(block_size));
    }
  });
}

} // end namespace lcals
} // end namespace rajaperf

#endif  // RAJA_ENABLE_HIP
