# ShiftRegisters

## Видео демонстрация работы
https://drive.google.com/file/d/1Tl6Kir0T8RghbPz2A4XQCTR5hyXz475a/view?usp=sharing

## Описание схемы
### Подключение светодиодов и регистра 74HC595
подключение пинов регистра следующее:
1. светодиодный дисплей подключен к пинам `1-7`.
2. Пин `16 VCC` подключён к 5V, от него подключён пин `10`
3. Пин `8 GND` подключён к GND, от него также подключён пин `13`.
4. К ардуино подключены пины `data`, `clock` и `latch` для передачи битов в регистр.
5. С пина `9` первого регистра передаётся информация на вход второму регистру в пин `data`.
6. пины второго регистра `clock` и `latch` соединены параллельно с соответствующими пинами первого регистра.

![Pinout-74HC595-Shift-Register](https://github.com/user-attachments/assets/49ad344b-e8d1-4b90-86b6-e5350564df41)

# Описание скрипта `ShiftRegisters.ino`
### Программа осуществляет работу со светодиодным дисплеем двух регистров 74HC595. Данные передаются побитово с ардуино на регистры соединённые цепочкой.
Программа по сути является таймером, отсчитывающим секунды от заданного значения. В программе хранится массив булевых значений `digits`. За каждый такт в регистры загоняется один бит, всего тактов 16, после чего происходит
latch и цифры выводятся на дисплеи, далее цикл повторяется, но выводимое число будет на единицу меньше, благодаря уменьшению значения переменной `cur_seconds`. 16 тактов занимают ~1 секунду, так что можно считать это таймером
обратного отсчёта.
После того как значение доходит до 0, отсчёт начинается заново с того же числа, которое хранится в переменной `recieved_value`. В эту переменную через серийный порт также можно задать новое значение, в таком случае
с помощью функции `check_updates()` будет установлено новое число с которого будет вестить отсчёт, а также таймер сразу переключится на начало отсчёта с этого числа, по завершению текущей секунды (16 тактов).

# Рекомендации к применению
1. **Собрать схему из регистра 74HC595 и дисплея в соответствие с распиновкой программы.**
2. **Запустить скрипт `ShiftRegisters`**
