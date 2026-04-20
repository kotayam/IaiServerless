import socket

HOST = "api.open-meteo.com"
PATH = "/v1/forecast?latitude=35.6895&longitude=139.6917&current_weather=true"

def strip_chunk(body):
    # If chunked, first line is hex size — skip it
    i = body.find("\n")
    if i < 0:
        return body
    # check if first line looks like a hex number
    line = body[0:i].strip()
    is_hex = len(line) > 0
    for c in line:
        if not ((c >= '0' and c <= '9') or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F')):
            is_hex = False
            break
    if is_hex:
        return body[i+1:]
    return body

def http_get(host, path):
    req = bytes("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n" % (path, host), "utf-8")
    s = socket.socket()
    s.connect((host, 80))
    s.send(req)
    resp = ""
    while True:
        data = s.recv(1024)
        if not data:
            break
        resp = resp + str(data, "utf-8")
    s.close()
    i = resp.find("\r\n\r\n")
    return resp[i+4:] if i >= 0 else resp

def http_post(port, path, body):
    header = "POST %s HTTP/1.1\r\nHost: localhost\r\nContent-Length: %d\r\nConnection: close\r\n\r\n" % (path, len(body))
    req = bytes(header + body, "utf-8")
    s = socket.socket()
    s.connect(("127.0.0.1", port))
    s.send(req)
    resp = ""
    while True:
        data = s.recv(1024)
        if not data:
            break
        resp = resp + str(data, "utf-8")
    s.close()
    i = resp.find("\r\n\r\n")
    return resp[i+4:] if i >= 0 else resp

def handler():
    port_str = input()
    port = 0
    for c in port_str:
        if c >= '0' and c <= '9':
            port = port * 10 + (ord(c) - ord('0'))
    if port == 0:
        port = 8082

    json = strip_chunk(http_get(HOST, PATH))
    result = http_post(port, "/python/weather_parser", json)
    print(result)
