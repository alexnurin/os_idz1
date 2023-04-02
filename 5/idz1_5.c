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

    int p1[2], p2[2]; // Два массива для хранения файловых дескрипторов каналов.

    // Проверяем результат вызова функции pipe() для обоих каналов.
    if (pipe(p1) == -1 || pipe(p2) == -1) {
        printf("Невозможно создать канал.\n");
        exit(EXIT_FAILURE);
    }

    char* pipe_read = "prc.fifo";
    char* pipe_write = "pcw.fifo";

    mknod(pipe_read, S_IFIFO | 0666, 0);
    mknod(pipe_write, S_IFIFO | 0666, 0);

    pid_t pid1 = fork(); // Создаем первый дочерний процесс.

    // Проверяем результат вызова функции fork() для первого дочернего процесса.
    if (pid1 == -1) {
        printf("Ошибка при создании дочернего процесса 1.\n");
        return 1;
    }
    if (pid1 == 0) {
        // Считываем данные из входного файла и отправляем второму процессу.
        if((fifo_desc = open(pipe_read, O_WRONLY)) < 0){
            printf("Процесс 1: Невозможно открыть канал на запись.\n");
            exit(-1);
        }
        int input = open(argv[1], O_RDONLY);
        if (input < 0) {
            printf("Процесс 1: Невозможно открыть файл на чтение\n");
            exit(1);
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
        return 1;
    }
    if (pid2 == 0) {
        // Обрабатываем данные от первого процесса и передаём результаты третьему через каналы.
        if ((fifo_desc = open(pipe_write, O_RDONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        char buffer[BUFFER_SIZE]; // Буфер для чтения данных из канала.
        ssize_t n; // Количество байтов, прочитанных из канала.

        size_t bytesRead = read(p1[0], buffer, BUFFER_SIZE);

        int result = 0;
        // Считаем число идентификаторов
        // printf("%d\n", len);
        findIdentifier(buffer, &result);

        close(p1[0]); // Закрываем канал для чтения из p1.

        // Записываем результат в буффер
        char result_buffer[BUFFER_SIZE];
        snprintf(result_buffer, BUFFER_SIZE, "Обнаружено %d идентификаторов\n", result);

        // Отправляем результаты в третий процесс через p2.
        write(p2[1], result_buffer, strlen(result_buffer));
        close(p2[1]); // Закрываем канал для записи в p2.

        return 0; // Завершение второго дочернего процесса.
    }

    pid_t pid3 = fork(); // Создание третьего дочернего процесса.

    // Проверка на ошибку при создании процесса.
    if (pid3 == -1) {
        printf("Ошибка при создании дочернего процесса 3.\n");
        return 1;
    }
    if (pid3 == 0) {
        close(p1[0]); // Закрываем чтение из p1.
        close(p1[1]); // Закрываем заипсь в p1.
        close(p2[1]); // Закрываем запись в p2.

        // Открываем выходной файл для записи.
        int output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);

        char buffer[BUFFER_SIZE];
        ssize_t charCount;
        while ((charCount = read(p2[0], buffer, BUFFER_SIZE)) > 0) {
            write(output_fd, buffer, charCount); // Записываем данные из p2 в выходной файл.
        }

        close(p2[0]); // Закрываем конец канала для чтения, поскольку он больше не нужен.
        close(output_fd); // Закрываем файловый дескриптор для выходного файла.
        return 0;
    }

    // Закрываем все концы каналов, которые были открыты родительским процессом.
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);

    // Ожидаем завершения выполнения всех дочерних процессов.
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    return 0;
}