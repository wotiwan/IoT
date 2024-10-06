import serial
import time

responses = {
    'd': 7,
    'u': 6,
    'p': 4
}

port_led = "COM7"
port_photo = "COM5"
connection_led = serial.Serial(port_led, timeout=1)
connection_photo = serial.Serial(port_photo, timeout=1)


def send_command(cmd: str, response_len: int, connection: serial.Serial) -> str:
    str_resp = None
    connection.reset_input_buffer()
    connection.write(cmd.encode())
    if response_len > 0:
        resp = connection.readline(response_len)
        str_resp = resp.decode()
    return str_resp.strip()


def recieve_bytes(cmd: str, connection: serial.Serial) -> int:
    connection.reset_input_buffer()
    connection.write(cmd.encode())
    resp = connection.read(1)
    if resp:
        return int.from_bytes(resp, "big") * 4
    return 0


while True:
    photo_val = recieve_bytes('p', connection_photo)
    print(photo_val)
    if photo_val and int(photo_val) > 500:
        resp = send_command('u', responses['u'], connection_led)
    elif photo_val:
        resp = send_command('d', responses['d'], connection_led)
    print(photo_val)
    time.sleep(0.1)
