def extract(data, key):
    i = data.find('"current_weather":{')
    start = i if i >= 0 else 0
    i = data.find(key, start)
    if i < 0:
        return "?"
    i = i + len(key)
    if data[i] == ':':
        i = i + 1
    while data[i] == ' ':
        i = i + 1
    if data[i] == '"':
        i = i + 1
        end = i
        while data[end] != '"':
            end = end + 1
    else:
        end = i
        while data[end] != ',' and data[end] != '}':
            end = end + 1
    return data[i:end]

def handler():
    data = input()
    temp = extract(data, '"temperature"')
    wind = extract(data, '"windspeed"')
    code = extract(data, '"weathercode"')
    print("The weather in Tokyo is %s C, wind %s km/h (code %s)" % (temp, wind, code))
