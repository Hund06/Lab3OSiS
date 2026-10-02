#include <stdio.h>
#include <stdlib.h>
#include "find_min_max.h"
#include "utils.h"

// входные данные напрямую через аргументы командной строки.
int main(int argc, char **argv) {
  // argv[0] - имя самой программы, argv[1] - seed, argv[2] - размер массива.
  if (argc != 3) {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  // Функция atoi (ASCII to integer) конвертирует строку текста в целое число.
  int seed = atoi(argv[1]);
  if (seed <= 0) {
    printf("seed is a positive number\n");
    return 1;
  }

  int array_size = atoi(argv[2]);
  if (array_size <= 0) {
    printf("array_size is a positive number\n");
    return 1;
  }
  
  int *array = malloc(array_size * sizeof(int));
  
  // Заполняем выделенный участок памяти псевдослучайными числами
  GenerateArray(array, array_size, seed);
  
  // Вызываем нашу функцию поиска из файла find_min_max.c. 
  // Ищем сразу по всему массиву: от индекса 0 до array_size.
  struct MinMax min_max = GetMinMax(array, 0, array_size);
  
  free(array);

  printf("min: %d\n", min_max.min);
  printf("max: %d\n", min_max.max);

  return 0; 
}