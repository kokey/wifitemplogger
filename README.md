# wifitemplogger

Wifi temperature logger using the ESP8266 and ds18b20 temperature sensor

It works with the the ESP Open SDK https://github.com/pfalcon/esp-open-sdk installed into /opt
It should also be in your path:
```
export PATH=/opt/esp-open-sdk/xtensa-lx106-elf/bin:$PATH
```

build like this:
```
make SSID=yourSSID SSID_PASS=thePassword
```

then burn to the module like this (on a USB serial):
```
esptool.py -p  /dev/ttyUSB0 write_flash 0x00000 firmware/0x00000.bin 0x40000 firmware/0x40000.bin
```


