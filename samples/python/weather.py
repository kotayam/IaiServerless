import socket

HOST = "api.open-meteo.com"
LAT = "35.6895"
LON = "139.6917"

def handler():
    path = "/v1/forecast?latitude=%s&longitude=%s&current_weather=true" % (LAT, LON)
    req = bytes("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n" % (path, HOST), "utf-8")

    s = socket.socket()
    s.connect((HOST, 80))
    s.send(req)
    while True:
        data = s.recv(1024)
        if not data:
            break
        print(str(data, "utf-8"), end="")
    s.close()
