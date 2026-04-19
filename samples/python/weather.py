import socket

HOST = "open-meteo.com"
PATH = "/v1/forecast?latitude=35.6895&longitude=139.6917&current_weather=true"

def handler():
    s = socket.socket()
    s.connect((HOST, 80))
    s.send(b"GET " + PATH.encode() + b" HTTP/1.1\r\nHost: " + HOST.encode() + b"\r\nConnection: close\r\n\r\n")
    while True:
        data = s.recv(1024)
        if data is None:
            break
        print(data.decode(), end="")
    s.close()
