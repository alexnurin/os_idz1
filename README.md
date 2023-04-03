# ИДЗ 1
## Нарин Алексей Сергеевич | БПИ-217 | Вариант 25

**Задание**

Разработать программу, которая определяет в ASCII-строке частоту встречаемости различных идентификаторов, являющихся словами, состоящими из букв и цифр, начинающихся с буквы.
Разделителями являются все другие символы. Для тестирования
можно использовать программы, написанные на различных языках программирования

**Использование**

./main input output 

**Схема решения**

Список процессов:

1) Считывает данные из файла в канал 1<br>
    -> именованный канал "pipe1"
2) Считывает данные из "pipe1", обрабатывает и записывает в "pipe2" <br>
    -> именованный канал "pipe2"
3) Считывает данные из "pipe2" и записывает в файл<br>

Алгоритм: квадратичный поиск подходящих подстрок.

**Тестирование**
test1: "Обнаружено 3 идентификаторов"<br>
test2: "Обнаружено 0 идентификаторов"<br>
test3: "Обнаружено 2 идентификаторов"<br>
test4: "Обнаружено 80 идентификаторов"<br>
test5: "Обнаружено 0 идентификаторов"<br>

Файлы входных и выходных данных расположены в папке с решением.
