# Options (can be modified by user)
option(WITH_TESTS "Compile unit tests (can be run with 'make test')" ON)
option(WITH_INTEGRATION_TESTS "Compile integration tests" OFF)
option(WITH_AHMED "Link to the AHMED library to enable ACA mode assembly)" OFF)
option(WITH_OPENCL "Add OpenCL support for Fiber module" OFF)
mark_as_advanced(WITH_OPENCL)
option(WITH_CUDA "Add CUDA support for Fiber module" OFF)
mark_as_advanced(WITH_CUDA)
option(WITH_ALUGRID "Have Alugrid" OFF)
option(WITH_MKL "Use Intel MKL for BLAS and LAPACK functionality" OFF)
# option(WITH_GOTOBLAS "Use GotoBLAS for BLAS and LAPACK functionality" OFF)
# option(WITH_OPENBLAS "Use OpenBLAS for BLAS and LAPACK functionality" OFF)

# At present these options are unsupported (in particular, compilation of Python
# wrappers will fail if any of them is turned off).
option(ENABLE_SINGLE_PRECISION "Enable support for single-precision calculations" ON)
mark_as_advanced(ENABLE_SINGLE_PRECISION)
option(ENABLE_DOUBLE_PRECISION "Enable support for double-precision calculations" ON)
mark_as_advanced(ENABLE_DOUBLE_PRECISION)
option(ENABLE_COMPLEX_KERNELS  "Enable support for complex-valued kernel functions" ON)
mark_as_advanced(ENABLE_COMPLEX_KERNELS)
option(ENABLE_COMPLEX_BASIS_FUNCTIONS  "Enable support for complex-valued basis functions" ON)
mark_as_advanced(ENABLE_COMPLEX_BASIS_FUNCTIONS)
