import struct
import threading
import queue

from fastapi import FastAPI, Path, Query
from fastapi.responses import PlainTextResponse
from fastapi.middleware.cors import CORSMiddleware
import paho.mqtt.client as mqtt

NUM_LEDS = 228
BROKER = "localhost"
MQTT_PORT = 1884
TOPIC = "led/data"

mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.connect(BROKER, MQTT_PORT)
mqtt_client.loop_start()

cmd_queue: "queue.Queue[bytes]" = queue.Queue()

def publisher_worker():
    while True:
        payload = cmd_queue.get()
        if payload is None:
            break
        mqtt_client.publish(TOPIC, payload)

threading.Thread(target=publisher_worker, daemon=True).start()

app = FastAPI()

origins = [
    "http://localhost",
    "http://localhost:3000",
    "http://localhost:3001",
]
app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/off")
async def cmd_off():
    payload = struct.pack("1B", 0)
    cmd_queue.put(payload)
    return

@app.get("/ambilight")
async def cmd_ambilight():
    payload = struct.pack("1B", 1)
    cmd_queue.put(payload)
    return

@app.get("/static_color")
async def cmd_static_color():
    payload = struct.pack("1B", 2)
    cmd_queue.put(payload)
    return

@app.get("/rainbow")
async def cmd_rainbow():
    payload = struct.pack("1B", 3)
    cmd_queue.put(payload)
    return

@app.get("/gradient")
async def cmd_gradient():
    payload = struct.pack("1B", 4)
    cmd_queue.put(payload)
    return

@app.get("/star_shooting")
async def cmd_star_shooting():
    payload = struct.pack("1B", 5)
    cmd_queue.put(payload)
    return

@app.get("/brightness/{new_brightness}")
async def change_brightness(
    new_brightness: int = Path(..., ge=0, le=255, description="0–255")
):
    payload = struct.pack("2B", 99, new_brightness)
    cmd_queue.put(payload)
    return

@app.get("/color/{r}/{g}/{b}")
async def change_color(
    r: int = Path(..., ge=0, le=255),
    g: int = Path(..., ge=0, le=255),
    b: int = Path(..., ge=0, le=255),
):
    payload = struct.pack("4B", 98, r, g, b)
    cmd_queue.put(payload)
    return

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
