import time
import paho.mqtt.client as mqtt
import random
from uuid import getnode as get_maq
import hashlib
import serial

# Константы для порога и окна скользящего среднего
SLIDING_WINDOW_SIZE = 10
PUBLISH_INTERVAL = 5

# Подключение 
port_photo = "COM6"
connection_photo = serial.Serial(port_photo, timeout=1)

def recieve_bytes(cmd: str, connection: serial.Serial) -> int:
    connection.reset_input_buffer()
    connection.write(cmd.encode())
    resp = connection.read(1)
    if resp:
        return int.from_bytes(resp, "big") * 4
    return 0

# MQTT настройки
pub_id = hashlib.new('sha256')
mac = get_maq
pub_id.update(str(mac).encode())
pub_id = pub_id.hexdigest()[:10]
broker = "broker.emqx.io"
print(f"listen me at id {pub_id}")

client = mqtt.Client(pub_id)
client.connect(broker)
client.loop_start()

# Переменные для хранения минимального и максимального значений освещенности
min_value = float('inf')
max_value = float('-inf')

# Функция обновления минимума и максимума
def update_min_max(state):
    global min_value, max_value
    if state < min_value:
        min_value = state
    if state > max_value:
        max_value = state

# Функция определения убывающего участка яркости
def is_decreasing(trend_data):
    # Определяем, убывающая ли это последовательность
    return all(x > y for x, y in zip(trend_data, trend_data[1:]))

# Переменные для хранения измерений и отслеживания тренда
state_history = []

print("Starting measurement loop...")

start_time = time.time()

while True:
    state = recieve_bytes('p', connection_photo)
    
    # Обновляем историю измерений
    state_history.append(state)
    if len(state_history) > SLIDING_WINDOW_SIZE:
        state_history.pop(0)

    # Обновляем минимум и максимум
    update_min_max(state)
    
    # Рассчитываем порог как среднее между минимальным и максимальным
    threshold = (min_value + max_value) / 2

    # Проверяем тренд: если участок убывающий, зажигаем светодиод
    if len(state_history) == SLIDING_WINDOW_SIZE and is_decreasing(state_history):
        command = "d"  # Убывающий тренд - включаем светодиод
    else:
        command = "u"  # Иначе выключаем
    
    print(f"Current state: {state}, Command: {command}, Min: {min_value}, Max: {max_value}, Threshold: {threshold}")
    
    # Публикуем состояние светодиода
    client.publish(f"lab/{pub_id}/led/state", command)
    
    # Отправляем минимальное и максимальное значение каждые PUBLISH_INTERVAL секунд
    if time.time() - start_time >= PUBLISH_INTERVAL:
        client.publish(f"lab/{pub_id}/sensor/min", min_value)
        client.publish(f"lab/{pub_id}/sensor/max", max_value)
        start_time = time.time()
    
    time.sleep(1)

client.disconnect()
client.loop_stop()
