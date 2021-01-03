# - Find OpenBlas library
# 
# This module defines
#  OPENBLAS_LIBRARY, libraries to link against to use Openblas.
#  OPENBLAS_FOUND, If false, do not try to use Openblas.

FIND_LIBRARY(OPENBLAS_LIBRARY NAMES openblas blas PATHS /usr/lib/openblas-base )

# handle the QUIETLY and REQUIRED arguments and set LOKI_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenBlas  DEFAULT_MSG  OPENBLAS_LIBRARY)

find_path(BLAS_INCLUDE_DIR openblas_config.h /usr/include /usr/local/include /usr/include/openblas
/usr/include/aarch64-linux-gnu
/usr/include/arm-linux-gnueabihf
/usr/include/i386-kfreebsd-gnu
/usr/include/i386-linux-gnu
/usr/include/mips64el-linux-gnuabi64
/usr/include/openblas
/usr/include/powerpc64-linux-gnu
/usr/include/powerpc64le-linux-gnu
/usr/include/s390x-linux-gnu
/usr/include/sparc64-linux-gnu
/usr/include/x86_64-kfreebsd-gnu
/usr/include/x86_64-linux-gnu
)

message ("-- BLAS_INCLUDE_DIR=${BLAS_INCLUDE_DIR}")

MARK_AS_ADVANCED(OPENBLAS_LIBRARY)
