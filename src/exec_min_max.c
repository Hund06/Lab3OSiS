#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Запускает скомпилированную программу sequential_min_max в отдельном процессе
int main(int argc, char **argv) {
    pid_t pid = fork();

    if (pid == 0) {
        // Мы в дочернем процессе. Массив аргументов для передаваемой программы.
        // Первый аргумент - всегда имя самой программы, затем идут seed и array_size.
        char *args[] = {"./sequential_min_max", "42", "1000", NULL};
        
        // execv заменит текущий процесс программой sequential_min_max
        execv("./sequential_min_max", args);
        
        // Если execv сработал успешно, код ниже никогда не выполнится. 
        // Если выполнился - значит файл программы не был найден.
        perror("Ошибка при вызове execv");
        return 1;
    } else if (pid > 0) {
        // Мы в родительском процессе, ждем завершения дочернего
        wait(NULL);
    } else {
        perror("Ошибка при вызове fork");
        return 1;
    }

    return 0;
}