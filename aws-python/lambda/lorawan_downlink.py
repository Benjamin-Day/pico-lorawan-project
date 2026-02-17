import boto3
import json


def lambda_handler(event, context):
    # Access the payload data
    payload = event['payload']



    try:
        iot_wireless = boto3.client('iotwireless')
        response = iot_wireless.send_data_to_wireless_device(
            Id='<LORAWAN_DEVICE_ID>',
            TransmitMode=0,
            PayloadData=payload,
            WirelessMetadata={
                'LoRaWAN': {
                    'FPort': 2,
                },
            }
        )
        print(response)
    except Exception as e:
        print(e)
