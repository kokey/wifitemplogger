# wifitemplogger

Wifi temperature logger using the ESP8266 and ds18b20 temperature sensor

It uses the 1-wire protocol to talk to the temperature sensor on GPIO 2 and to read
the temperature once per minute.  It then submits the temperature to a URL,
e.g. http://yoursite.com/temps.php?value=__TEMPERATURE__ once a minute.

It works with the the ESP Open SDK https://github.com/pfalcon/esp-open-sdk installed into /opt
It should also be in your path:
```
export PATH=/opt/esp-open-sdk/xtensa-lx106-elf/bin:$PATH
```

build like this:
```
make SSID=yourSSID SSID_PASS=thePassword SUBMITURL="http://yoursite.com/temps.php?value="
```

then burn to the module like this (on a USB serial):
```
esptool.py -p  /dev/ttyUSB0 write_flash 0x00000 firmware/0x00000.bin 0x40000 firmware/0x40000.bin
```


