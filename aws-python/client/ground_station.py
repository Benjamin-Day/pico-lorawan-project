import ssl
import time
import paho.mqtt.client as mqtt
import json
from datetime import datetime
import base64 as base64
import keyboard
import struct
import matplotlib.animation as animation
import matplotlib.pyplot as plt

alertSent = 0

# Callback function for connection success
def on_connect(client, userdata, flags, rc):
    print("=== Connected to AWS ===\nResult code " + str(rc))

def on_subscribe(client, userdata, mid, granted_qos):
    print("==== Client subscribed: ", str(mid), " with qos:", str(granted_qos), " ===")

def on_disconnect(client, userdata, rc):
    print("=== Device disconnected with result code: " + str(rc) + " ===")

def on_message(client, userdata, message):
    global alertSent
    newMsg = message.payload.decode()
    jsonData = json.loads(newMsg)
    base64Value = jsonData['PayloadData']
    binaryBase64 = base64.b64decode(base64Value)
    gasLevel = struct.unpack('f', binaryBase64)[0]
    print(gasLevel)
    if gasLevel >= 0.6 and alertSent == 0:
        msg = json.dumps({'High gas levels detected (sensor 1)': gasLevel})
        mqtt_client.publish('lorawan/alert', msg, 1)
        alertSent = 1
    elif gasLevel < 0.6:
        alertSent = 0
    time = datetime.now()
    timestamps.append(time)
    gasValues.append(gasLevel)
    

def shutdown():
    mqtt_client.loop_stop()
    mqtt_client.disconnect()

def on_publish(client, userdata, mid):
    print("=== Message Sent ===")



gasValues = []
timestamps = []

# AWS IoT Core endpoint and port number
iot_endpoint = "your-aws-iot-endpoint.iot.us-east-1.amazonaws.com"
iot_port = 8883
IoT_protocol_name = "x-amzn-mqtt-ca"

# SSL/TLS mutual authentication credentials
ca_cert = "awsPCCerts/AmazonRootCA1.pem"
client_cert = "awsPCCerts/cert-certificate.pem.crt"
client_key = "awsPCCerts/key-private.pem.key"

# MQTT client initialization
mqtt_client = mqtt.Client()

# SSL/TLS context initialization
ssl_context = ssl.create_default_context()
ssl_context.set_alpn_protocols([IoT_protocol_name])
ssl_context.load_verify_locations(cafile=ca_cert)
ssl_context.load_cert_chain(certfile=client_cert, keyfile=client_key)

# Set the on_connect callback function
mqtt_client.on_connect = on_connect
mqtt_client.on_subscribe = on_subscribe
mqtt_client.on_message = on_message

# Connect to AWS IoT Core using SSL/TLS mutual authentication
mqtt_client.tls_set_context(context=ssl_context)
mqtt_client.connect(iot_endpoint, port=iot_port)

# Sub to topic and start the MQTT client
mqtt_client.subscribe("lorawan/uplink")
mqtt_client.loop_start()

# Create figure and axes
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)

# Define animation function
def animate(i):
    ax.clear()
    ax.plot(timestamps, gasValues)
    ax.set_xlabel('Time')
    ax.set_ylabel('Gas Levels (V)')
    ax.set_title('Live Gas Levels')
    for tick in ax.get_xticklabels():
        tick.set_rotation(45)

ani = animation.FuncAnimation(fig, animate, interval=1000, save_count=len(gasValues))

plt.show()

while True:
    try:
        time.sleep(5)
        # if the keyboard r button is pressed then the downlink message will be sent
        if(keyboard.is_pressed('r')):
            msg = json.dumps({'payload':'AQ=='})
            mqtt_client.publish('lorawan/downlink', msg, 0)
        if(keyboard.is_pressed('d')):
            msg = json.dumps({'payload':'AA=='})
            mqtt_client.publish('lorawan/downlink', msg, 0)

    # Interrupt to exit the loop and stop mqtt_client
    except KeyboardInterrupt:
        shutdown()
        break

