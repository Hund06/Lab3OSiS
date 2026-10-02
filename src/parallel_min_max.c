#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Основная программа для параллельного поиска минимума и максимума
int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("Seed должен быть положительным числом\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("Размер массива должен быть положительным числом\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("Количество процессов должно быть положительным числом\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создаем pipe для общения между процессами, если не используем файлы
  int pipefd[2];
  if (!with_files) {
    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        return 1;
    }
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        
        // Разбиваем массив на равные части для каждого процесса
        unsigned int step = array_size / pnum;
        unsigned int begin = i * step;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * step;

        struct MinMax current_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[256];
          sprintf(filename, "temp_result_%d.bin", i);
          FILE *f = fopen(filename, "wb");
          fwrite(&current_min_max, sizeof(struct MinMax), 1, f);
          fclose(f);
        } else {
          // Записываем структуру с результатом в конец трубы, предназначенный для записи
          write(pipefd[1], &current_min_max, sizeof(struct MinMax));
        }
        return 0;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Родительский процесс ждет завершения всех дочерних процессов
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    struct MinMax current_min_max;

    if (with_files) {
      char filename[256];
      sprintf(filename, "temp_result_%d.bin", i);
      FILE *f = fopen(filename, "rb");
      fread(&current_min_max, sizeof(struct MinMax), 1, f);
      fclose(f);
      remove(filename); // Удаляем временный файл после прочтения
    } else {
      // Считываем результат работы одного из дочерних процессов из трубы
      read(pipefd[0], &current_min_max, sizeof(struct MinMax));
    }

    if (current_min_max.min < min_max.min) min_max.min = current_min_max.min;
    if (current_min_max.max > min_max.max) min_max.max = current_min_max.max;
  }

  if (!with_files) {
      close(pipefd[0]);
      close(pipefd[1]);
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}