#pragma once

#ifndef __both__
#ifdef __CUDACC__
#define __both__ __host__ __device__
#else
#define __both__
#endif
#endif

