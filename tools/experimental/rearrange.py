import json

with open('examples/full-incubator-demo/diagram.json', 'r') as f:
    data = json.load(f)

positions = {
    "esp32": {"top": 0, "left": 0},
    "fan": {"top": -150, "left": -250},
    "humidifier": {"top": -50, "left": -250},
    "heater": {"top": 100, "left": -250},
    "buzzer": {"top": 200, "left": -250},
    "door": {"top": 250, "left": -250},
    "encoder": {"top": 300, "left": -250},
    "ntc": {"top": -150, "left": 300},
    "sts35a": {"top": -50, "left": 300},
    "sts35b": {"top": 50, "left": 300},
    "sht4x": {"top": 150, "left": 300},
    "ina0": {"top": 250, "left": 300},
    "ina1": {"top": 350, "left": 300},
    "telemetry": {"top": 450, "left": 0},
    "display": {"top": 450, "left": 200}
}

for part in data['parts']:
    if part['id'] in positions:
        part['top'] = positions[part['id']]['top']
        part['left'] = positions[part['id']]['left']

with open('examples/full-incubator-demo/diagram.json', 'w') as f:
    json.dump(data, f, indent=2)

print("Done")
