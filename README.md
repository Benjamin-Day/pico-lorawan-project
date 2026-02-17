# Rpi Pico IoT LoRaWAN Gas Monitoring System

**Authors:** Benjamin Day, Gurkirat Singh 
**Created:** 2022

An IoT gas level monitoring system. Implemented using a Raspberry Pi Pico, MQ2 gas sensor, and SX1276 LoRaWAN transmitter to detect hazardous gases in industrial settings. The LoRaWAN gateway was a Laird Sentrius RG1xx. Sensor data is transmitted over LoRaWAN to the gateway and then to AWS IoT Core over the internet. Messaging is done via AWS IoT Core (MQTT). 

The [ground_station](/aws-python/client/ground_station.py) Python client graphs the data from the LoRaWAN Pico endpoint in real-time. It is intended to be run on a computer with direct internet access. When gas levels are potentially hazardous, the ground station triggers an alert by publishing to a separate MQTT topic linked with AWS SNS. To demonstrate downlink functionality, the ground station can publish messages that are forwarded to the Pico endpoint to turn on the onboard LED via the [lorawan_downlink](/aws-python/lambda/lorawan_downlink.py) Lambda function.

## Architecture

![Architecture](/images/lorawan-architecture.png)

## Hardware

| Component | Description |
|---|---|
| Laird Sentrius RG1xx | LoRaWAN gateway (US915) |
| Raspberry Pi Pico | RP2040 microcontroller |
| SX1276 (RFM95W) | LoRa radio module |
| MQ2 Gas Sensor | Detects CO, Butane, Alcohol, Hydrogen, Propane |
| Buzzer | Audible alarm when gas threshold exceeded |
| LED | Visual alarm indicator |

### Pin Configuration

| Function | GPIO Pin |
|---|---|
| SPI MOSI | Default SPI TX |
| SPI MISO | Default SPI RX |
| SPI SCK | Default SPI SCK |
| SX1276 NSS | 8 |
| SX1276 Reset | 9 |
| SX1276 DIO0 | 7 |
| SX1276 DIO1 | 10 |
| Buzzer | 15 |
| Alarm LED | 17 |
| MQ2 Analog Out | 26 (ADC0) |

## Project Structure

This project utilizes the [pico-lorawan](https://github.com/sandeepmistry/pico-lorawan) library by Sandeep Mistry, which wraps the [LoRaMac-node](https://github.com/Lora-net/LoRaMac-node) LoRaWAN protocol stack by Semtech. These third-party libraries handle the LoRaWAN networking protocol (join procedures, encryption, channel management, etc.).

```
├── pico-lorawan/                    # Pico firmware (C)
│   ├── firmware/                    # Our application code
│   │   ├── main.c                   #   Application entry point
│   │   ├── config.h                 #   LoRaWAN credentials (edit this)
│   │   └── lib/
│   │       ├── Gas_Sensor/          #   MQ2 sensor driver (adapted from Waveshare)
│   │       └── Config/              #   Hardware pin config (adapted from Waveshare)
│   ├── CMakeLists.txt               # Our build configuration
│   ├── src/                         # pico-lorawan library glue code (third-party)
│   └── lib/
│       └── LoRaMac-node/            # LoRaWAN protocol stack (third-party, ~1600 files)
└── aws-python/                      # Our AWS IoT Core Client and Lambda Python code
    ├── client/
    │   ├── ground_station.py        #   MQTT ground station with live graphing
    │   └── requirements.txt         #   Python dependencies
    └── lambda/
        └── lorawan_downlink.py      #   Lambda for sending downlinks to device
```

## Getting Started

### Prerequisites

- [Pico C/C++ SDK](https://github.com/raspberrypi/pico-sdk) installed
- `PICO_SDK_PATH` environment variable set
- CMake 3.12+
- A LoRaWAN gateway (AWS IoT Core for LoRaWAN)
- Python 3 (for the monitoring clients)

### Building the Firmware

1. Clone the repository:
   ```bash
   git clone https://github.com/Benjamin-Day/pico-lorawan-project.git
   cd pico-lorawan-project
   ```

2. Edit the LoRaWAN credentials in `pico-lorawan/firmware/config.h`:
   ```c
   #define LORAWAN_DEVICE_EUI  "your-device-eui"
   #define LORAWAN_APP_EUI     "your-app-eui"
   #define LORAWAN_APP_KEY     "your-app-key"
   ```

3. Build:
   ```bash
   cd pico-lorawan
   mkdir build && cd build
   cmake ..
   make
   ```

4. Flash the `.uf2` file to the Pico (hold BOOTSEL while plugging in, then copy the file).

### Running the Python Client

1. Install dependencies:
   ```bash
   pip install -r aws-python/client/requirements.txt
   ```

2. Place your AWS IoT certificates in an `awsPCCerts/` directory:
   - `AmazonRootCA1.pem`
   - `cert-certificate.pem.crt`
   - `key-private.pem.key`

3. Update the endpoint in `aws-python/client/ground_station.py` with your AWS IoT endpoint.

4. Run:
   ```bash
   python aws-python/client/ground_station.py
   ```

   The client will display a live graph of gas levels and publish alerts to `lorawan/alert` when levels exceed the threshold.

   **Keyboard controls:** `r` = LED on, `d` = LED off (sends downlink to device).

## How It Works

1. The Pico reads the MQ2 gas sensor via ADC and converts the analog value to a voltage
2. If the gas level exceeds 0.9V, the buzzer and alarm LED activate locally
3. The gas reading is sent as a float over LoRaWAN (OTAA, US915 region)
4. AWS IoT Core receives the uplink via the LoRaWAN gateway
5. The Python client subscribes to `lorawan/uplink`, decodes the base64 payload, and plots it
6. If the gas level exceeds 0.6V, an alert is published to `lorawan/alert`, which triggers an SNS email notification
7. Downlink messages can be sent to toggle the Pico's onboard LED

## Security

- **AES-128** encryption between end device and server
- **TLS** between LoRaWAN gateway and server
- **OTAA** (Over-The-Air Activation) for dynamic session key generation
- Restricted IAM policies and roles in AWS
- AWS credentials stay in the cloud (Lambda handles downlink via AWS API)

### Known Security Limitations 

The following issues could not be addressed due to time and resource limitations:
- No nonce to prevent join replay attacks (addressed in LoRaWAN 1.1)
- Root keys stored in plaintext on device (production systems should use an HSM)
- TLS used instead of IPSec between gateway and server

## Acknowledgments

- **[pico-lorawan](https://github.com/sandeepmistry/pico-lorawan)** by Sandeep Mistry / Arm Limited — LoRaWAN library for Raspberry Pi Pico (BSD 3-Clause License)
- **[Waveshare](https://www.waveshare.com/)** — Hardware configuration and device abstraction code (`firmware/lib/Config/`) (MIT License)
- **[Waveshare MQ2 Gas Sensor Wiki](https://www.waveshare.com/wiki/MQ-2_Gas_Sensor)** — MQ2 gas sensor driver code (`firmware/lib/Gas_Sensor/`)

## License

The LoRaWAN library is licensed under the BSD 3-Clause License. See [pico-lorawan/LICENSE](pico-lorawan/LICENSE).
