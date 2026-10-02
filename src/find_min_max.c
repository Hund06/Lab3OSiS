#include "find_min_max.h"
#include <limits.h>

// Ищет минимальный и максимальный элементы массива на заданном промежутке от begin до end
struct MinMax GetMinMax(int *array, unsigned int begin, unsigned int end) {
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (unsigned int i = begin; i < end; i++) {
    if (array[i] < min_max.min) {
      min_max.min = array[i];
    }
    if (array[i] > min_max.max) {
      min_max.max = array[i];
    }
  }

  return min_max;
}