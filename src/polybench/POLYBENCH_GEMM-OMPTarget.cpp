  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-18, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-738930
//
// All rights reserved.
//
// This file is part of the RAJA Performance Suite.
//
// For details about use and distribution, please read RAJAPerf/LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "POLYBENCH_GEMM.hpp"

#include "RAJA/RAJA.hpp"

#if defined(RAJA_ENABLE_TARGET_OPENMP)

#include "common/OpenMPTargetDataUtils.hpp"

#include <iostream>

namespace rajaperf 
{
namespace polybench
{

//
// Define thread block size for target execution
//
#define NUMTEAMS 256

#define POLYBENCH_GEMM_DATA_SETUP_OMP_TARGET \
  int hid = omp_get_initial_device(); \
  int did = omp_get_default_device(); \
\
  const Index_type ni = m_ni; \
  const Index_type nj = m_nj; \
  const Index_type nk = m_nk; \
\
  Real_type alpha = m_alpha; \
  Real_type beta = m_beta; \
\
  Real_ptr A; \
  Real_ptr B; \
  Real_ptr C; \
\
  allocAndInitOpenMPDeviceData(A, m_A, ni*nk, did, hid); \
  allocAndInitOpenMPDeviceData(B, m_B, nk*nj, did, hid); \
  allocAndInitOpenMPDeviceData(C, m_C, ni*nj, did, hid);


#define POLYBENCH_GEMM_TEARDOWN_OMP_TARGET \
  getOpenMPDeviceData(m_C, C, ni*nj, hid, did); \
  deallocOpenMPDeviceData(A, did); \
  deallocOpenMPDeviceData(B, did); \
  deallocOpenMPDeviceData(C, did);


void POLYBENCH_GEMM::runOpenMPTargetVariant(VariantID vid)
{
  const Index_type run_reps = getRunReps();

  if ( vid == Base_OpenMPTarget ) {

    POLYBENCH_GEMM_DATA_SETUP_OMP_TARGET;

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      #pragma omp target is_device_ptr(A,B,C) device( did )
      #pragma omp teams distribute parallel for num_teams(NUMTEAMS) schedule(static, 1) collapse(2)
      for (Index_type i = 0; i < ni; ++i ) {
        for (Index_type j = 0; j < nj; ++j ) {
          POLYBENCH_GEMM_BODY1;
          for (Index_type k = 0; k < nk; ++k ) {
             POLYBENCH_GEMM_BODY2;
          }
          POLYBENCH_GEMM_BODY3;
        }
      }

    }
    stopTimer();

    POLYBENCH_GEMM_TEARDOWN_OMP_TARGET;

  } else if (vid == RAJA_OpenMPTarget) {

    POLYBENCH_GEMM_DATA_SETUP_OMP_TARGET;

    POLYBENCH_GEMM_VIEWS_RAJA;

    using EXEC_POL =
      RAJA::KernelPolicy<
        RAJA::statement::Collapse<RAJA::omp_target_parallel_collapse_exec,
                                  RAJA::ArgList<0, 1>,
          RAJA::statement::Lambda<0>,
          RAJA::statement::For<2, RAJA::seq_exec,
            RAJA::statement::Lambda<1>
          >,
          RAJA::statement::Lambda<2>
        >
      >;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::kernel_param<EXEC_POL>(

          RAJA::make_tuple( RAJA::RangeSegment{0, ni},
                            RAJA::RangeSegment{0, nj},
                            RAJA::RangeSegment{0, nk} ),

          RAJA::tuple<double>{0.0},  // variable for dot

          [=] (Index_type i, Index_type j, Index_type /*k*/, double& dot) {
            POLYBENCH_GEMM_BODY1_RAJA;
          },
          [=] (Index_type i, Index_type j, Index_type k, double& dot) {
            POLYBENCH_GEMM_BODY2_RAJA;
          },
          [=] (Index_type i, Index_type j, Index_type /*k*/, double& dot) {
            POLYBENCH_GEMM_BODY3_RAJA;
          }
        );

      }
      stopTimer();

    POLYBENCH_GEMM_TEARDOWN_OMP_TARGET;

  } else {
      std::cout << "\n  POLYBENCH_GEMM : Unknown OMP Target variant id = " << vid << std::endl;
  }

}

} // end namespace polybench
} // end namespace rajaperf

#endif  // RAJA_ENABLE_TARGET_OPENMP
  