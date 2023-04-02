#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 5000 // Размер буфера для чтения и записи данных

void findIdentifier(const char *str, int *res) {
    for (int startSubstr = 0; str[startSubstr] != '\0'; startSubstr++) {
        // Eсли текущий символ не является буквой или цифрой
        if (!isalnum(str[startSubstr])) {
            // Переходим к следующему символу
            continue;
        }

        int endSubstr = startSubstr;
        // Пока текущий символ — буква либо цифра
        while (isalnum(str[endSubstr])) {
            endSubstr++;
        }
        // Eсли длина найденной подстроки больше 1 и начинается с буквы.
        if (endSubstr - startSubstr > 1 && isalpha(str[startSubstr])) {
            // printf("%d -> %d\n", startSubstr, endSubstr);
            (*res)++;
        }
        startSubstr = endSubstr - 1;
    }
}

int main(int argc, char *argv[]) {
    // Проверяем количество аргументов.
    if (argc != 3) {
        printf("Использование: ./main <input> <output>\n");
        exit(EXIT_FAILURE);
    }
    int fifo_desc;

    char *pipe_read = "/tmp/pipe1";
    char *pipe_write = "/tmp/pipe2";

    mknod(pipe_read, S_IFIFO | 0666, 0);
    mknod(pipe_write, S_IFIFO | 0666, 0);

    pid_t pid1 = fork(); // Создаем первый дочерний процесс.

    // Проверяем результат вызова функции fork() для первого дочернего процесса.
    if (pid1 == -1) {
        printf("Ошибка при создании дочернего процесса 1.\n");
        return EXIT_FAILURE;
    }
    if (pid1 == 0) {
        int input = open(argv[1], O_RDONLY);
        if (input < 0) {
            printf("Процесс 1: Невозможно открыть файл на чтение\n");
            exit(EXIT_FAILURE);
        }
        fifo_desc = open(pipe_read, O_WRONLY);
        // Считываем данные из входного файла и отправляем второму процессу.
        if (fifo_desc < 0) {
            printf("Процесс 1: Невозможно открыть канал на запись.\n");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE]; // Буфер для чтения данных.
        ssize_t charCount; // Переменная для хранения количества считанных байтов.
        // Считываем данные из файла в буфер.
        while ((charCount = read(input, buffer, BUFFER_SIZE)) > 0) {
            write(fifo_desc, buffer, charCount); // Считываем в первый канал.
        }

        close(fifo_desc); // Закрываем запись в первый канал.
        close(input); // Закрываем чтение из файла.

        return 0; // Выходим из дочернего процесса.
    }

    pid_t pid2 = fork(); // Создание второго дочернего процесса.

    // Проверяем результат вызова функции fork() для второго дочернего процесса.
    if (pid2 == -1) {
        printf("Ошибка при создании дочернего процесса 2.\n");
        return EXIT_FAILURE;
    }
    if (pid2 == 0) {
        // Обрабатываем данные от первого процесса и передаём результаты третьему через каналы.
        fifo_desc = open(pipe_read, O_RDONLY);
        if (fifo_desc < 0) {
            printf("Процесс 2: Невозможно открыть канал на запись.\n");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE]; // Буфер для чтения данных из канала.
        size_t bytesRead = read(fifo_desc, buffer, BUFFER_SIZE);

        int result = 0;

        // Считаем число идентификаторов
        findIdentifier(buffer, &result);

        close(fifo_desc);

        fifo_desc = open(pipe_write, O_WRONLY);
        // Записываем результат в буффер
        char result_buffer[BUFFER_SIZE];
        snprintf(result_buffer, BUFFER_SIZE, "Обнаружено %d идентификаторов\n", result);

        // Отправляем результаты в третий процесс через p2.
        write(fifo_desc, result_buffer, strlen(result_buffer));
        close(fifo_desc); // Закрываем канал для записи в p2.

        return 0; // Завершение второго дочернего процесса.
    }

    pid_t pid3 = fork(); // Создание третьего дочернего процесса.

    // Проверка на ошибку при создании процесса.
    if (pid3 == -1) {
        printf("Ошибка при создании дочернего процесса 3.\n");
        return 1;
    }
    if (pid3 == 0) {
        // Открываем выходной файл для записи.
        int output = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (output < 0) {
            printf("Процесс 3: Невозможно создать файл\n");
            exit(EXIT_FAILURE);
        }
        fifo_desc = open(pipe_write, O_RDONLY);
        if (fifo_desc < 0) {
            printf("Процесс 3: Невозможно открыть канал на запись\n");
            exit(EXIT_FAILURE);
        }
        char buffer[BUFFER_SIZE];
        ssize_t charCount;
        while ((charCount = read(fifo_desc, buffer, BUFFER_SIZE)) > 0) {
            write(output, buffer, charCount); // Записываем данные из p2 в выходной файл.
        }
        close(fifo_desc); // Закрываем конец канала для чтения, поскольку он больше не нужен.
        close(output); // Закрываем файловый дескриптор для выходного файла.
        return 0;
    }
    // Ожидаем завершения выполнения всех дочерних процессов.
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    return 0;
}