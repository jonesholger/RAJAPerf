#!/usr/bin/env bash

###############################################################################
# Copyright (c) 2017-20, Lawrence Livermore National Security, LLC
# and RAJA Performance Suite project contributors.
# See the RAJAPerf/COPYRIGHT file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
#################################################################################

BUILD_SUFFIX=lc_toss3-gcc-8.1.0
RAJA_HOSTCONFIG=../tpl/RAJA$1/host-configs/lc-builds/toss3/gcc_X.cmake

rm -rf build_${BUILD_SUFFIX} 2>/dev/null
mkdir build_${BUILD_SUFFIX} && cd build_${BUILD_SUFFIX}

module load cmake/3.14.5

if [ "$1" == "orig" ]; then
    argO="On"
    argV="Off"
else
    argO="Off"
    argV="On"
fi

cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/tce/packages/gcc/gcc-8.1.0/bin/g++ \
  -C ${RAJA_HOSTCONFIG} \
  -DENABLE_OPENMP=On \
  -DENABLE_RAJA_SEQUENTIAL=$argO -DENABLE_RAJA_SEQUENTIAL_ARGS=$argV \
  -DCMAKE_INSTALL_PREFIX=../install_${BUILD_SUFFIX} \
  "$@" \
  ..
