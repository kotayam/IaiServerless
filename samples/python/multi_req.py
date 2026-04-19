import socket

HOST = "api.open-meteo.com"
PATHS = [
    "/v1/forecast?latitude=35.6895&longitude=139.6917&current_weather=true",
    "/v1/forecast?latitude=51.5074&longitude=-0.1278&current_weather=true",
    "/v1/forecast?latitude=40.7128&longitude=-74.0060&current_weather=true",
]

def do_request(path, idx):
    req = bytes("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n" % (path, HOST), "utf-8")
    s = socket.socket()
    s.connect((HOST, 80))
    s.send(req)
    print("\n[req %d] " % (idx + 1), end="")
    while True:
        data = s.recv(1024)
        if not data:
            break
        print(str(data, "utf-8"), end="")
    s.close()

def handler():
    for i in range(len(PATHS)):
        do_request(PATHS[i], i)
