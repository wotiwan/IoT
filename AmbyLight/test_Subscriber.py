import struct
import time
import paho.mqtt.client as mqtt

BROKER = "localhost"
MQTT_PORT = 1884
TOPIC = "led/data"

COMMAND_NAMES = {
    0: "Off",
    1: "Ambilight",
    2: "Static Color",
    3: "Rainbow",
    4: "Gradient",
    5: "Star Shooting",
    98: "Change Color",
    99: "Change Brightness"
}

def on_connect(client, userdata, flags, rc, properties=None):
    print(f"Connected to MQTT broker with result code {rc}")
    client.subscribe(TOPIC)
    print(f"Subscribed to topic '{TOPIC}'")

def on_message(client, userdata, msg):
    payload = msg.payload
    if not payload:
        print("Получен пустой payload")
        return

    cmd = payload[0]
    name = COMMAND_NAMES.get(cmd, f"Unknown({cmd})")
    ts = time.strftime('%H:%M:%S')
    if cmd in range(0, 6):
        (code,) = struct.unpack("!B", payload)
        print(f"[{ts}] Команда: {name} (код {code})")
    elif cmd == 99:
        if len(payload) >= 2:
            code, brightness = struct.unpack("!2B", payload[:2])
            print(f"[{ts}] {name}: код={code}, new_brightness={brightness}")
        else:
            print(f"[{ts}] Ошибка: недостаточно байт для яркости (ожидалось 2, получили {len(payload)})")

    elif cmd == 98:
        if len(payload) >= 4:
            code, r, g, b = struct.unpack("!4B", payload[:4])
            print(f"[{ts}] {name}: код={code}, R={r}, G={g}, B={b}")
        else:
            print(f"[{ts}] Ошибка: недостаточно байт для цвета (ожидалось 4, получили {len(payload)})")

    else:
        bytes_list = list(payload)
        hex_str = payload.hex().upper()
        print(f"[{ts}] RAW payload: bytes={bytes_list}, hex={hex_str}")

def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(BROKER, MQTT_PORT)
    client.loop_forever()

if __name__ == "__main__":
    main()
