import dxcam
import cv2
import time

cv2.setUseOptimized(True)
cv2.setNumThreads(8)  # Примерно +10%
cv2.ocl.setUseOpenCL(True)

SCREEN_WIDTH, SCREEN_HEIGHT = 1920, 1080  # Пока вручную, позже добавить автоопределение

# Размеры регионов, также добавить пропорции относительно размера экрана
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


camera = dxcam.create(output_color="BGR")  # BGR вместо BGRA +63%
camera.start(target_fps=180)  # Снижать таргет до реальных значений +20%

fps_gen = fps_counter()

while True:
    frame = camera.get_latest_frame()
    if frame is None:
        continue

    top_part = frame[:TOP_HEIGHT, :, :3]  # Верхняя полоса (ширина экрана, 100px)
    bottom_part = frame[-BOTTOM_HEIGHT:, :, :3]  # Нижняя полоса (ширина экрана, 100px)
    left_part = frame[:, :SIDE_WIDTH, :3]  # Левая полоса (100px, высота экрана)
    right_part = frame[:, -SIDE_WIDTH:, :3]  # Правая полоса (100px, высота экрана)

    top_resized = cv2.reduce(top_part, 0, cv2.REDUCE_AVG)  # (1, 1920, 3)
    bottom_resized = cv2.reduce(bottom_part, 0, cv2.REDUCE_AVG)  # (1, 1920, 3)
    left_resized = cv2.reduce(left_part, 1, cv2.REDUCE_AVG)  # (1080, 1, 3)
    right_resized = cv2.reduce(right_part, 1, cv2.REDUCE_AVG)  # (1080, 1, 3)

    # FPS вывод
    fps = next(fps_gen)
    print(f"FPS: {fps:.2f}")

    # Для отладки
    # top_display = cv2.resize(top_resized, (1500, 100), interpolation=cv2.INTER_AREA)
    # cv2.imshow("Top Part", top_display)

    # if cv2.waitKey(1) & 0xFF == ord('q'):
    #     break

camera.stop()
cv2.destroyAllWindows()
