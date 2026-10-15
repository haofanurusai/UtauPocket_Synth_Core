#ifndef UTAU_POCKET_CONFIG_H
#define UTAU_POCKET_CONFIG_H
/* All translation units and clients must use this same precision. */
#ifndef FP_TYPE
#define FP_TYPE float
#endif
#if defined(USE_PTHREAD) || defined(_OPENMP) || defined(USE_FFTW_THREADS)
#error UtauPocket core requires a serial build
#endif
#endif
