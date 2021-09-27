Copy the Device Credentials to this folder.

Follow these steps:
- Copy the credentials in this folder and rename them according to mfg_config.csv.
    - device.crt: This is the device certificate.
    - device.key: This is the device private key.
    - server.crt: This is the AWS root CA certificate.
- (Optional) You can modify mfg_config.csv add other details also in the NVS such as AWS account ID, AWS Endpoint URL, Device ID. You can take a look at app_main.c for reference.
- Modify the file mfg_config.csv and add the relevant paths like: /Users/.../esp-alexa/examples/amazon_aia/main/certs/device.crt
- Run the command: python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py mfg_config.csv mfg.bin 0x6000
- Now flash this mfg.bin on the device using the command: python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 --port $ESPPORT write_flash 0x10000 mfg.bin
- Now flash the board normally as you would using: make flash OR make spiffs_flash
