def extract(data, key):
    start = data.find('"current_weather":{')
    if start < 0:
        start = 0
    i = data.find(key, start)
    if i < 0:
        return "?"
    i = i + len(key)
    if data[i] == ':':
        i = i + 1
    while data[i] == ' ':
        i = i + 1
    result = ''
    while data[i] != ',' and data[i] != '}':
        result = result + data[i]
        i = i + 1
    return result

def handler():
    data = input()
    temp = extract(data, '"temperature"')
    wind = extract(data, '"windspeed"')
    code = extract(data, '"weathercode"')
    print("The weather in Tokyo is %s C, wind %s km/h (code %s)" % (temp, wind, code))
