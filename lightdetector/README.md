# Light detector (Детектор источника света)

## Описание схемы
### Подключение светодиодов
![circuit (1)](https://github.com/user-attachments/assets/9559d103-df59-4ca5-b660-6ca4c773d046)
1. **Принципиальная схема подключения светодиодов**
   - Каждый светодиод подключён к своему "+", на плате это пины `13` и `12`
   - Все светодиоды подключены к "-" через один резистор, котороый подключён в колодку "-" на плате, которая в свою очередь соединена с пином GND
   - Резистор номиналом 20 кОм
### Подключение Фоторезисторов
![circuit (3)](https://github.com/user-attachments/assets/ce224122-b6ea-4442-b14f-47238892224d)

2. **Принципиальная схема подключения фоторезисторов**
    - Каждый светодиод подключён к своему аналоговому пину: `A0` и `A1`
    - Каждый фоторезистор подключён к "-" через собственный резистор, котороый подключён в колодку "-" на плате, которая в свою очередь соединена с пином `GND`
    - Каждый резистор номиналом 5 кОм

###  Датчики и индикаторы  подключены независимо - изъятие любого из них не ведет к изменению работы остальных.  
Индикаторы показывают направление в котором нужно повернуть макетную плату вокруг своей оси. Мигает тот светодиод, 
при повороте в сторону которого источник света станет ближе по направлению вперёд. 
При нахождении оптимального направления светодиоды, светятся без мигания.

# Переменные

- `previous_millis`: хранит время последнего изменения состояния (используется для отслеживания интервалов).
- `interval`: задаёт интервал времени (1000 мс) для чтения данных с фоторезисторов.
- `interval_2`: задаёт интервал времени (500 мс) для мигания светодиодов
- `is_equal`: логическая переменная, которая определяет, равны ли значения двух фоторезисторов.

# Описание функций

### `loop()`
Основной цикл программы, который повторяется бесконечно. Функция управляет логикой работы светодиодов и обработки данных с двух аналоговых датчиков.

#### Основные этапы:
1. **Чтение текущего времени**: С помощью функции `millis()` считывается текущее время в миллисекундах.
2. **Управление светодиодами**:
   - Каждые 500 мс (переменная `interval_2`), после последнего включения светодиода, если значения с датчиков не равны, оба светодиода выключаются. Нужно для создания мигания светодиодов.
   - Каждую секунду (переменная `interval`), программа считывает значения с двух фоторезисторов (подключенных к пинам `A0` и `A1`).
3. **Чтение значений датчиков**: С помощью функции `analogRead()` считываются значения с двух аналоговых входов (`SENSOR_IN_0` и `SENSOR_IN_1`), которые отображают освещённость фоторезисторов.
   - Результаты выводятся в консоль.
4. **Обработка показаний датчиков**:
   - Если разница между значениями фоторезисторов больше 5, зажигается один из светодиодов:
     - Если значение первого фоторезистора (A0) больше второго (A1), включается светодиод на пине 13 и выключается на пине 12.
     - Если значение второго фоторезистора (A1) больше первого (A0), включается светодиод на пине 12 и выключается на пине 13.
   - Если показания датчиков почти равны (разница не превышает 5), оба светодиода загораются без мигания.
5. **Обновление переменной времени**: После каждой итерации обновляется переменная `previous_millis` для отсчёта времени до следующего цикла.

# Инструкция по применению
  - Пары фоторезистор + светодиод должны быть расположены на разных сторонах платы (слева и справа)
### Для правильной индикации в какую сторону поворачивать плату:
  - Фоторезистор `A0` должен быть расположен на одной стороне платы со светодиодом `13`
  - Фоторезистор `A1` должен быть расположен на одной стороне платы со светодиодом `12`