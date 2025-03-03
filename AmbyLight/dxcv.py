import dxcam
import cv2
import time

cv2.setUseOptimized(True)
cv2.setNumThreads(8)  # Примерно +10%
cv2.ocl.setUseOpenCL(True)

SCREEN_WIDTH, SCREEN_HEIGHT = 1920, 1080  # Пока вручную, позже добавить автоопределение

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


camera = dxcam.create(output_color="BGR")  # BGR вместо BGRA +63%
camera.start(target_fps=180)

fps_gen = fps_counter()

while True:
    frame = camera.get_latest_frame()
    if frame is None:
        continue

    top_part = frame[:TOP_HEIGHT, :, :3]  # Верхняя полоса (ширина экрана, 100px)
    bottom_part = frame[-BOTTOM_HEIGHT:, :, :3]  # Нижняя полоса (ширина экрана, 100px)
    left_part = frame[:, :SIDE_WIDTH, :3]  # Левая полоса (100px, высота экрана)
    right_part = frame[:, -SIDE_WIDTH:, :3]  # Правая полоса (100px, высота экрана)

    # plt.imshow(top_part)
    # plt.show()

    top_resized = cv2.reduce(top_part, 0, cv2.REDUCE_AVG)
    top_resized = cv2.resize(top_resized, (73, 1), cv2.INTER_AREA)

    bottom_resized = cv2.reduce(bottom_part, 0, cv2.REDUCE_AVG)
    bottom_resized = cv2.resize(bottom_resized, (73, 1), cv2.INTER_AREA)

    left_resized = cv2.reduce(left_part, 1, cv2.REDUCE_AVG)
    left_resized = cv2.resize(left_resized, (1, 41), cv2.INTER_AREA)

    right_resized = cv2.reduce(right_part, 1, cv2.REDUCE_AVG)
    right_resized = cv2.resize(right_resized, (1, 41), cv2.INTER_AREA)

    # plt.imshow(top_resized)
    # plt.show()

    # FPS вывод
    fps = next(fps_gen)
    print(f"FPS: {fps:.2f}")
    #
    # # Для отладки
    # top_display = cv2.resize(top_resized, (1920, 100), interpolation=cv2.INTER_AREA)
    # cv2.imshow("Top Part", top_display)
    #
    # if cv2.waitKey(1) & 0xFF == ord('q'):
    #     break

camera.stop()
cv2.destroyAllWindows()
