# Переменная CC задает используемый компилятор.
CC=gcc
# Переменная CFLAGS содержит флаги для компилятора. 
# -I. означает, что заголовочные файлы (.h) нужно искать в текущей директории.
CFLAGS=-I.

# Цель all стоит первой. При вызове команды make без аргументов, 
# будут собраны все три программы, перечисленные в зависимостях.
all : sequential_min_max parallel_min_max exec_min_max

# Сборка последовательной версии. Зависит от объектных файлов и заголовков.
sequential_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

# Сборка параллельной версии.
parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Сборка программы, вызывающей exec. 
# Зависит только от своего исходного файла, так как не использует внешние модули.
exec_min_max : exec_min_max.c
	$(CC) -o exec_min_max exec_min_max.c $(CFLAGS)

# Компиляция вспомогательного модуля в объектный файл (без создания .exe).
utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# Компиляция модуля поиска в объектный файл.
find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Цель для очистки папки от сгенерированных бинарных файлов. 
# Вызывается вручную: make clean
clean :
	rm utils.o find_min_max.o sequential_min_max parallel_min_max exec_min_max