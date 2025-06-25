import struct
import threading
import queue
import requests
import asyncio
import paho.mqtt.client as mqtt
import shutil
import re
import json
import zipfile

from fastapi import FastAPI, Path, BackgroundTasks, HTTPException,status
from fastapi.responses import PlainTextResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from pathlib import Path as SysPath
from typing import Literal

NUM_LEDS    = 228
BROKER      = "localhost"
MQTT_PORT   = 1884
TOPIC       = "led/data"
POLL_INTERVAL = 5 * 60

mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.connect(BROKER, MQTT_PORT)
mqtt_client.loop_start()

cmd_queue: "queue.Queue[bytes]" = queue.Queue()

weather_colors = {
    "Sunny":                     (255, 223,   0),
    "Clear":                     (135, 206, 235),
    "Partly cloudy":             (176, 196, 222),
    "Cloudy":                    (169, 169, 169),
    "Overcast":                  (105, 105, 105),
    "Mist":                      (211, 211, 211),
    "Patchy rain possible":      (173, 216, 230),
    "Patchy snow possible":      (240, 248, 255),
    "Patchy sleet possible":     (176, 224, 230),
    "Patchy freezing drizzle possible": (175, 238, 238),
    "Thundery outbreaks possible": (255, 140,   0),
    "Blowing snow":              (240, 255, 255),
    "Blizzard":                  (220, 220, 220),
    "Fog":                       (190, 190, 190),
    "Freezing fog":              (169, 169, 169),
    "Patchy light drizzle":      (176, 224, 230),
    "Light drizzle":             (135, 206, 250),
    "Freezing drizzle":          (175, 238, 238),
    "Heavy freezing drizzle":    (176, 224, 230),
    "Patchy light rain":         (135, 206, 250),
    "Light rain":                ( 30, 144, 255),
    "Moderate rain at times":    (  0,   0, 205),
    "Moderate rain":             (  0,   0, 255),
    "Heavy rain at times":       (  0,   0, 139),
    "Heavy rain":                ( 25,  25, 112),
    "Light freezing rain":       (176, 224, 230),
    "Moderate or heavy freezing rain": (175, 238, 238),
    "Light sleet":               (135, 206, 235),
    "Moderate or heavy sleet":   ( 70, 130, 180),
    "Patchy light snow":         (240, 248, 255),
    "Light snow":                (255, 250, 250),
    "Patchy moderate snow":      (230, 230, 250),
    "Moderate snow":             (245, 245, 245),
    "Patchy heavy snow":         (224, 255, 255),
    "Heavy snow":                (255, 255, 255),
    "Ice pellets":               (175, 238, 238),
    "Light rain shower":         ( 64, 164, 223),
    "Moderate or heavy rain shower": (  0,   0, 205),
    "Torrential rain shower":    (  0,   0, 139),
    "Light sleet showers":       (135, 206, 235),
    "Moderate or heavy sleet showers": ( 70, 130, 180),
    "Light snow showers":        (240, 248, 255),
    "Moderate or heavy snow showers": (224, 255, 255),
    "Light showers of ice pellets": (175, 238, 238),
    "Moderate or heavy showers of ice pellets": (176, 224, 230),
    "Patchy light rain with thunder": (255, 140,   0),
    "Moderate or heavy rain with thunder": (255, 69,   0),
    "Patchy light snow with thunder": (230, 230, 250),
    "Moderate or heavy snow with thunder": (216, 191, 216)
}

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

def update_config_field_str(mac: str, field: str, value):
    config_path = SysPath("cnfg") / mac / "config.json"
    if not config_path.is_file():
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Файл {config_path} не найден"
        )

    try:
        with config_path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Ошибка чтения config.json: {e}"
        )

    data[field] = value

    try:
        with config_path.open("w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Ошибка записи config.json: {e}"
        )

def update_config_field(mac: str, field: Literal["height", "width"], value: int):
    config_path = SysPath("cnfg") / mac / "config.json"
    if not config_path.is_file():
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Файл {config_path} не найден"
        )

    try:
        with config_path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Ошибка чтения config.json: {e}"
        )

    data[field] = value
    if field == "width":
        data['led_counts']["top"] = value
        data['led_counts']["bottom"] = value
    elif field == "height":
        data['led_counts']["right"] = value
        data['led_counts']["left"] = value

    try:
        with config_path.open("w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Ошибка записи config.json: {e}"
        )

@app.get("/direction/{mac}/{direct}")
async def update_height(
    mac: str = Path(..., description="MAC-адрес устройства"),
    direct: str = Path(..., description="Новое значение направления")
):
    update_config_field_str(mac, "direction", direct)
    return {"detail": f"Поле direction обновлено до {direct} для {mac}"}

@app.get("/update_cnfg/{mac}/height/{value}")
async def update_height(
    mac: str = Path(..., description="MAC-адрес устройства"),
    value: int = Path(..., ge=1, le=200, description="Новое значение высоты")
):
    update_config_field(mac, "height", value)
    return {"detail": f"Поле height обновлено до {value} для {mac}"}


@app.get("/update_cnfg/{mac}/width/{value}")
async def update_width(
    mac: str = Path(..., description="MAC-адрес устройства"),
    value: int = Path(..., ge=1, le=200, description="Новое значение ширины")
):
    update_config_field(mac, "width", value)
    return {"detail": f"Поле width обновлено до {value} для {mac}"}


MAC_PATTERN = r'^[0-9A-Fa-f]{2}([:-]?[0-9A-Fa-f]{2}){5}$'

@app.get("/api/v2/devices/configure/{mac_address}")
async def configure_device(
    mac_address: str = Path(
        ...,
        description="MAC-адрес в любом формате (без валидации)"
    )
):
    mac = mac_address.strip()

    base_cfg_dir = SysPath("cnfg")
    target_dir   = base_cfg_dir / mac

    try:
        base_cfg_dir.mkdir(exist_ok=True)
        target_dir.mkdir(exist_ok=True)
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Не удалось создать папки: {e}"
        )

    files_to_copy = ["config.json", "dxcv-wireless-configurable.exe"]
    missing = [f for f in files_to_copy if not SysPath(f).is_file()]
    if missing:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Не найдены файлы для копирования: {missing}"
        )

    for fname in files_to_copy:
        src = SysPath(fname)
        dst = target_dir / fname
        try:
            shutil.copy2(src, dst)
        except Exception as e:
            raise HTTPException(
                status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                detail=f"Ошибка копирования {fname}: {e}"
            )
    config_path = target_dir / "config.json"
    try:
        with open(config_path, "r", encoding="utf-8") as f:
            config_data = json.load(f)
        config_data["MAC"] = mac
        with open(config_path, "w", encoding="utf-8") as f:
            json.dump(config_data, f, indent=4, ensure_ascii=False)
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Ошибка при обновлении MAC в config.json: {e}"
        )


    return {"detail": f"Папка 'cnfg/{mac}' создана и файлы скопированы."}

@app.get("/download_package_po/{mac}")
async def download_package(mac: str):
    base_dir = SysPath("cnfg") / mac
    config_file = base_dir / "config.json"
    script_file = base_dir / "dxcv-wireless-configurable.exe"

    if not config_file.exists() or not script_file.exists():
        raise HTTPException(status_code=404, detail="Файлы не найдены")

    zip_path = base_dir / f"soft_package.zip"
    if zip_path.exists():
        zip_path.unlink()

    try:
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            zipf.write(config_file, arcname="config.json")
            zipf.write(script_file, arcname="dxcv-wireless-configurable.exe")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Ошибка при создании архива: {e}")

    return FileResponse(
        path=zip_path,
        filename=zip_path.name,
        media_type='application/zip'
    )

@app.get("/off")
async def cmd_off():
    payload = struct.pack("1B", 0)
    cmd_queue.put(payload)

@app.get("/ambilight")
async def cmd_ambilight():
    payload = struct.pack("1B", 1)
    cmd_queue.put(payload)

@app.get("/static_color")
async def cmd_static_color():
    payload = struct.pack("1B", 2)
    cmd_queue.put(payload)

@app.get("/rainbow")
async def cmd_rainbow():
    payload = struct.pack("1B", 3)
    cmd_queue.put(payload)

@app.get("/gradient")
async def cmd_gradient():
    payload = struct.pack("1B", 4)
    cmd_queue.put(payload)

@app.get("/star_shooting")
async def cmd_star_shooting():
    payload = struct.pack("1B", 5)
    cmd_queue.put(payload)

@app.get("/brightness/{new_brightness}")
async def change_brightness(
    new_brightness: int = Path(..., ge=0, le=255, description="0–255")
):
    payload = struct.pack("2B", 99, new_brightness)
    cmd_queue.put(payload)

@app.get("/speed/{new_speed}")
async def change_speed(
    new_speed: int = Path(..., ge=1, le=10, description="1–10")
):
    payload = struct.pack("2B", 97, new_speed)
    cmd_queue.put(payload)

@app.get("/LED/{nnl}")
async def change_LED(
    nnl: int = Path(..., ge=60, le=300, description="60–300")
):
    payload = struct.pack("2B", 199, nnl)
    cmd_queue.put(payload)

@app.get("/color/{r}/{g}/{b}")
async def change_color(
    r: int = Path(..., ge=0, le=255),
    g: int = Path(..., ge=0, le=255),
    b: int = Path(..., ge=0, le=255),
):
    payload = struct.pack("4B", 98, r, g, b)
    cmd_queue.put(payload)


API_KEY = '856cdf58e4974d78ac5105809251006'

_last_conditions: dict[str, str] = {}

async def _poll_weather(ip_address: str):
    while True:
        await asyncio.sleep(POLL_INTERVAL)
        try:
            url = (
                f'http://api.weatherapi.com/v1/current.json'
                f'?key={API_KEY}&q={ip_address}'
            )
            r = requests.get(url, timeout=5)
            r.raise_for_status()
            cond = r.json()['current']['condition']['text']
        except Exception:
            continue

        prev = _last_conditions.get(ip_address)
        if prev is None or cond == prev:
            _last_conditions[ip_address] = cond
            continue
        _last_conditions[ip_address] = cond
        break

@app.get(
    "/weather/{ip_address}",
    response_class=PlainTextResponse,
    summary="Получить текстовое описание погоды по IP и запустить поллинг"
)
async def get_weather_by_ip(
    ip_address: str = Path(
        ...,
        pattern=r'^\d{1,3}(\.\d{1,3}){3}$',
        description="IPv4-адрес, например 93.171.103.249"
    ),
    background_tasks: BackgroundTasks = None
):
    url = f'http://api.weatherapi.com/v1/current.json?key={API_KEY}&q={ip_address}'
    try:
        resp = requests.get(url, timeout=5)
        resp.raise_for_status()
        condition = resp.json()['current']['condition']['text']
    except Exception as e:
        raise HTTPException(status_code=502, detail=f"Error fetching weather: {e}")

    _last_conditions[ip_address] = condition
    background_tasks.add_task(_poll_weather, ip_address)
    rgb = weather_colors.get(condition, (0, 0, 0))  # fallback на чёрный, если нет в словаре
    r, g, b = rgb
    payload = struct.pack("4B", 98, r, g, b)
    cmd_queue.put(payload)
    return condition


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
