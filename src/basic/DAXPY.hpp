//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-21, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

///
/// DAXPY kernel reference implementation:
///
/// for (Index_type i = ibegin; i < iend; ++i ) {
///   y[i] += a * x[i] ;
/// }
///

#ifndef RAJAPerf_Basic_DAXPY_HPP
#define RAJAPerf_Basic_DAXPY_HPP

#define DAXPY_DATA_SETUP \
  Real_ptr x = m_x; \
  Real_ptr y = m_y; \
  Real_type a = m_a;

#define DAXPY_BODY  \
  y[i] += a * x[i] ;


#include "common/KernelBase.hpp"

namespace rajaperf
{
class RunParams;

namespace basic
{

class DAXPY : public KernelBase
{
public:

  DAXPY(const RunParams& params);

  ~DAXPY();

  void setUp(VariantID vid);
  void updateChecksum(VariantID vid, size_t tid);
  void tearDown(VariantID vid);

  void runSeqVariant(VariantID vid, size_t tid);
  void runOpenMPVariant(VariantID vid, size_t tid);
  void runCudaVariant(VariantID vid, size_t tid);
  void runHipVariant(VariantID vid, size_t tid);
  void runOpenMPTargetVariant(VariantID vid, size_t tid);

  void setCudaTuningDefinitions(VariantID vid);
  template < size_t block_size >
  void runCudaVariantImpl(VariantID vid);
  template < size_t block_size >
  void runHipVariantImpl(VariantID vid);

private:
  static const size_t default_gpu_block_size = 256;
  using gpu_block_sizes_type = gpu_block_size::list_type<default_gpu_block_size>;

  Real_ptr m_x;
  Real_ptr m_y;
  Real_type m_a;
};

} // end namespace basic
} // end namespace rajaperf

#endif // closing endif for header file include guard
