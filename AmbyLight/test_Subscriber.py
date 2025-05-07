import struct
import paho.mqtt.client as mqtt
# является тестовым подписчиком для проверки работоспособности, в будущем может быть удалён

BROKER = "localhost"
MQTT_PORT = 1884
TOPIC = "led/data"

received_packets = 0

def on_connect(client, userdata, flags, rc, properties=None):
    print(f"Подключено с кодом результата {rc}")
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    global received_packets
    payload = msg.payload
    leds = [struct.unpack("BBB", payload[i:i+3]) for i in range(0, len(payload), 3)]
    received_packets += 1
    print(f"Пакетов получено: {received_packets}, первые LED данные: {leds[:5]}")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, MQTT_PORT, 60)
client.loop_forever()

