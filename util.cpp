#include "util.h"

#include <random>
#include <ctime>

void FillRandomFloat(float *arr, size_t size) {
  std::default_random_engine eng;
  std::uniform_real_distribution<float> dist(-1, 1);
  for (size_t i = 0; i < size; ++i) {
    arr[i] = dist(eng);
  }
}

void CompareFloatArrays(float *arr1, float *arr2, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    if (arr1[i] != arr2[i]) {
      printf("Different! (arr1[%ld]=%f, arr2[%ld]=%f)\n", i, arr1[i], i, arr2[i]);
      break;
    }
  }
  printf("Same!\n");
}

double GetTime() {
  timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}
