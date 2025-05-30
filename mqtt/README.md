## Пояснение к коду файла Publisher.py
### Функция для получения данных от микроконтроллера
Функция (`def recieve_bytes`) отправляет команду cmd на устройство через COM-порт и читает один байт ответа, который интерпретируется как целое число.

### MQTT подключение
Используется paho.mqtt.client для подключения к брокеру MQTT. Для создания уникального идентификатора клиента используется хэш от MAC-адреса устройства, что делает идентификатор уникальным для каждого устройства.

MQTT клиент подключается к брокеру (в данном случае broker.emqx.io) и начинает публиковать данные в формате lab/<ID>/led/state для состояния светодиода и в другие топики для минимума и максимума освещенности.

### Работа с минимальными и максимальными значениями освещенности
Переменные (`min_value`), (`max_value`) и (`def update_min_max`) функция отслеживают минимальные и максимальные значения освещенности, обновляя их с каждым новым измерением.

### Определение убывающего тренда
Функция (`def is_decreasing`) проверяет, убывающие ли значения в массиве измерений. Если все элементы массива следуют в порядке убывания, считается, что это убывающий тренд.

### Публикация данных каждые 5 секунд (PUBLISH_INTERVAL)
  ```
      if time.time() - start_time >= PUBLISH_INTERVAL:
        client.publish(f"lab/{pub_id}/sensor/min", min_value)
        client.publish(f"lab/{pub_id}/sensor/max", max_value)
        start_time = time.time() ```
