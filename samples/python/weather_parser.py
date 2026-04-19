def extract(json, key):
    i = json.find(key)
    if i < 0:
        return "?"
    i += len(key)
    if json[i] == ':':
        i += 1
    end = i
    while end < len(json) and json[end] not in (',', '}'):
        end += 1
    return json[i:end].strip().strip('"')

def handler():
    json = INPUT  # provided by MicroPython runtime from stdin
    temp = extract(json, '"temperature"')
    wind = extract(json, '"windspeed"')
    code = extract(json, '"weathercode"')
    print("The weather in Tokyo is %s°C, wind %s km/h (code %s)" % (temp, wind, code))
