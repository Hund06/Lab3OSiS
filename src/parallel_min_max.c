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

// Основная программа для параллельного поиска минимума и максимума в массиве.
// Распределяет вычисления между несколькими дочерними процессами с использованием fork().
// Для межпроцессного взаимодействия применяет неименованные каналы (pipe) или файлы.
int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  // Парсинг аргументов командной строки с помощью getopt_long
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
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n", argv[0]);
    return 1;
  }

  // Выделение памяти под массив и его заполнение псевдослучайными числами
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Фиксируем время начала вычислений для замера производительности
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создаем неименованный канал (pipe) для передачи данных из оперативной памяти.
  // pipefd[0] используется для чтения, pipefd[1] - для записи.
  int pipefd[2];
  if (!with_files) {
    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        return 1;
    }
  }

  // Цикл создания дочерних процессов
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      
      // Блок кода, который выполняется ТОЛЬКО внутри дочернего процесса
      if (child_pid == 0) {
        
        // Вычисляем границы участка массива для текущего процесса.
        // Последний процесс забирает весь остаток массива, если размер не делится нацело.
        unsigned int step = array_size / pnum;
        unsigned int begin = i * step;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * step;

        // Ищем локальные минимум и максимум на выделенном участке
        struct MinMax current_min_max = GetMinMax(array, begin, end);

        // Передача результатов родительскому процессу
        if (with_files) {
          // Создаем уникальный бинарный файл для каждого процесса
          char filename[256];
          sprintf(filename, "temp_result_%d.bin", i);
          FILE *f = fopen(filename, "wb");
          fwrite(&current_min_max, sizeof(struct MinMax), 1, f);
          fclose(f);
        } else {
          // Записываем структуру целиком в канал (pipe). 
          // Родитель сможет прочитать эти байты с другой стороны трубы.
          write(pipefd[1], &current_min_max, sizeof(struct MinMax));
        }
        // Дочерний процесс должен завершить работу после отправки данных
        return 0;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Блок кода родительского процесса: ожидание завершения всех рабочих процессов.
  // wait(NULL) блокирует выполнение родителя, пока любой из детей не завершится.
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  // Подготовка структуры для поиска глобального минимума и максимума
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Сбор результатов от всех дочерних процессов
  for (int i = 0; i < pnum; i++) {
    struct MinMax current_min_max;

    if (with_files) {
      // Чтение структур из временных файлов на жестком диске
      char filename[256];
      sprintf(filename, "temp_result_%d.bin", i);
      FILE *f = fopen(filename, "rb");
      fread(&current_min_max, sizeof(struct MinMax), 1, f);
      fclose(f);
      remove(filename); // Удаляем временный файл, чтобы не засорять систему
    } else {
      // Чтение данных напрямую из буфера трубы в оперативной памяти
      read(pipefd[0], &current_min_max, sizeof(struct MinMax));
    }

    // Сравниваем локальные результаты с глобальными
    if (current_min_max.min < min_max.min) min_max.min = current_min_max.min;
    if (current_min_max.max > min_max.max) min_max.max = current_min_max.max;
  }

  // Обязательно закрываем дескрипторы канала, чтобы освободить системные ресурсы
  if (!with_files) {
      close(pipefd[0]);
      close(pipefd[1]);
  }

  // Фиксируем время окончания вычислений и высчитываем разницу в миллисекундах
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