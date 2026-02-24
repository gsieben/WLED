curl.exe -F "update=@C:\Users\gsieb\Dropbox\DEV\ESP\WLED\.pio\build\wled-wohnzimmer\firmware.bin" http://10.2.2.63/update


rem esptool.py --port COM3 --baud 921600 --before default_reset --after hard_reset write_flash --flash_mode opi --flash_size 16MB --flash_freq 80m 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
