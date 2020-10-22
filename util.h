#pragma once

#include <cstdlib>

#define CHECK_PTR(op) \
  do { \
    const void* ptr = op; \
    if (ptr == NULL) { \
      printf("[%s:%d] NULL is returned. Error: %s (errno=%d)\n", __FILE__, __LINE__, strerror(errno), errno); \
      exit(EXIT_FAILURE); \
    } \
  } while (false)

#define CHECK_INT(op) \
  do { \
    int num = op; \
    if (num != 0) { \
      printf("[%s:%d] Non-zero value(%d) is returned. Error: %s (errno=%d)\n", __FILE__, __LINE__, num, strerror(errno), errno); \
      exit(EXIT_FAILURE); \
    } \
  } while (false)

#define CHECK_WC_STATUS(op) \
  do { \
    ibv_wc_status status = op; \
    if (status != IBV_WC_SUCCESS) { \
      printf("[%s:%d] WC status error: %s(%d)\n", __FILE__, __LINE__, ibv_wc_status_str(status), status); \
      exit(EXIT_FAILURE); \
    } \
  } while (false)

void FillRandomFloat(float *arr, size_t size);

void CompareFloatArrays(float *arr1, float *arr2, size_t size);

double GetTime();
