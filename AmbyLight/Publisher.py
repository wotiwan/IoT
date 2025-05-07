import random
import struct
import threading
import time
from fastapi import FastAPI
from fastapi.responses import PlainTextResponse
import paho.mqtt.client as mqtt

NUM_LEDS = 228

BROKER = "localhost"   
MQTT_PORT = 1884
TOPIC = "led/data"

app = FastAPI()

mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.connect(BROKER, MQTT_PORT)
mqtt_client.loop_start()

publishing_flag = False
publish_thread = None

def publish_led_data(led_colors):
    flat_data = [component for color in led_colors for component in color]
    payload = struct.pack("BBB" * NUM_LEDS, *flat_data)
    mqtt_client.publish(TOPIC, payload)

def continuous_publisher(mode, color_tuple=None):
    global publishing_flag
    publishing_flag = True
    colors = {
        "red": (255, 0, 0),
        "green": (0, 255, 0),
        "blue": (0, 0, 255),
        "yellow": (255, 255, 0),
        "purple": (128, 0, 128),
        "orange": (255, 165, 0)
    }
    while publishing_flag:
        if mode == "rainbow":
            led_colors = [(random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
                          for _ in range(NUM_LEDS)]
        elif mode == "fixed":
            led_colors = [color_tuple] * NUM_LEDS if color_tuple else [(0, 0, 0)] * NUM_LEDS
        else:
            if mode in colors:
                led_colors = [colors[mode]] * NUM_LEDS
            else:
                led_colors = [(0, 0, 0)] * NUM_LEDS
        publish_led_data(led_colors)
        time.sleep(1/40)

@app.get("/start/{mode}")
async def start(mode: str):
    global publish_thread, publishing_flag
    if publish_thread and publish_thread.is_alive():
        publishing_flag = False
        publish_thread.join()

    if mode.startswith("fixed-"):
        parts = mode.split("-")
        if len(parts) == 4:
            try:
                r = int(parts[1])
                g = int(parts[2])
                b = int(parts[3])
                mode_actual = "fixed"
                color_tuple = (r, g, b)
            except ValueError:
                return PlainTextResponse("\nНеверный формат цвета. Используйте цифры для R, G, B.", status_code=400)
        else:
            return PlainTextResponse("\nНеверный формат для fixed режима. Пример: fixed-255-100-50", status_code=400)
    else:
        mode_actual = mode
        color_tuple = None

    publish_thread = threading.Thread(target=continuous_publisher, args=(mode_actual, color_tuple))
    publish_thread.start()
    return PlainTextResponse(f"\nНачато постоянное отправление данных в режиме {mode}")

@app.get("/stop")
async def stop():
    global publishing_flag, publish_thread
    publishing_flag = False
    if publish_thread:
        publish_thread.join()
    return PlainTextResponse("Постоянная отправка данных остановлена")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)

