import faulthandler
faulthandler.enable()

import dxcam
import cv2
import time
import struct
import serial

print(dxcam.device_info())

# Самая актуальная версия этот файл + прошивка ESP8266
ser = serial.Serial('COM7', 1000000) # Бауд рейты прошивки и питон приложения не совпадают, но всё равно ->
# -> оно работает
time.sleep(2)

list_test = [255 for i in range(684)]

cv2.setUseOptimized(True)
cv2.setNumThreads(8)  # Примерно +10%
cv2.ocl.setUseOpenCL(True)

# SCREEN_WIDTH, SCREEN_HEIGHT = 800, 600  # Пока вручную, позже добавить автоопределение

# Размеры регионов
TOP_HEIGHT = 100  # Верхняя область
BOTTOM_HEIGHT = 100  # Нижняя область
SIDE_WIDTH = 100  # Левая и правая области


def fps_counter():
    frame_count = 0
    start_time = time.time()
    while True:
        yield frame_count / (time.time() - start_time + 1e-6)
        frame_count += 1
        if time.time() - start_time >= 1.0:
            frame_count = 0
            start_time = time.time()

def list_filling(resized_array, col_arr):
    for i in resized_array[0]:
        for j in i:
            col_arr.append(j)


def list_filling2(resized_array, col_arr):
    temp_arr = []
    for i in resized_array[0]:
        temp_arr.append(i)
    len_arr = len(temp_arr)
    for i in range(len_arr-1,-1,-1):
        for j in temp_arr[i]:
            col_arr.append(j)


def list_filling_side(resized_array, col_arr):
    for i in resized_array:
        for j in i[0]:
            col_arr.append(j)


def list_filling_side2(resized_array, col_arr):
    temp_arr = []
    for i in resized_array:
        for j in i:
            temp_arr.append(j)
    len_arr = len(temp_arr)
    for i in range(len_arr - 1, -1, -1):
        for j in temp_arr[i]:
            col_arr.append(j)



camera = dxcam.create(output_color="RGB", output_idx=0)  # BGR вместо BGRA +63%
camera.start(target_fps=140)

fps_gen = fps_counter()


def get_confirm(message: str):
    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode("utf-8", errors="replace").strip()
            print(f"{data}")
            if data == message:
                return True
            if "�" in data:
                print(data)


# Используется отправка пакетами вместо всего массива сразу
# Во избежание переполнения буфера arduino и потери данных
def send_array(col_arr):
    data_bytes = struct.pack(f'{1}B', 1) # Отправляем текущий режим
    ser.write(data_bytes)
    # ser.flush()

    get_confirm("start")

    cnt = 0
    # Осталось только 24 значения, отправляем
    data_bytes = struct.pack(f'{684}B', *col_arr)
    ser.write(data_bytes)
    # ser.flush()
    get_confirm("OK_ambi")

while True:

    color_array = []

    frame = camera.get_latest_frame()
    if frame is None:
        print("NOOOOOOOONE")
        continue

    top_part = frame[:TOP_HEIGHT, :, :3]  # Верхняя полоса (ширина экрана, 100px)
    bottom_part = frame[-BOTTOM_HEIGHT:, :, :3]  # Нижняя полоса (ширина экрана, 100px)
    left_part = frame[:, :SIDE_WIDTH, :3]  # Левая полоса (100px, высота экрана)
    right_part = frame[:, -SIDE_WIDTH:, :3]  # Правая полоса (100px, высота экрана)


    # Этот кусок ест 2-4 фпс
    top_resized = cv2.reduce(top_part, 0, cv2.REDUCE_AVG)
    top_resized = cv2.resize(top_resized, (73, 1), cv2.INTER_AREA)

    bottom_resized = cv2.reduce(bottom_part, 0, cv2.REDUCE_AVG)
    bottom_resized = cv2.resize(bottom_resized, (73, 1), cv2.INTER_AREA)

    left_resized = cv2.reduce(left_part, 1, cv2.REDUCE_AVG)
    left_resized = cv2.resize(left_resized, (1, 41), cv2.INTER_AREA)

    right_resized = cv2.reduce(right_part, 1, cv2.REDUCE_AVG)
    right_resized = cv2.resize(right_resized, (1, 41), cv2.INTER_AREA)


    # Этот кусок кода не ест производительность
    list_filling(top_resized, color_array)
    list_filling_side(right_resized, color_array)
    list_filling2(bottom_resized, color_array)
    list_filling_side2(left_resized, color_array)


    send_array(color_array) # Отправка пакетами

    fps = next(fps_gen)
    # print(f"FPS: {fps:.2f}")

camera.stop()
cv2.destroyAllWindows()
