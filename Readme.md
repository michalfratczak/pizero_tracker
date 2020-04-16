

You need to use software driver to I2c, and therefore, disable bluetooth. This is for rPI 3 and up.

[https://raspberrypi.stackexchange.com/questions/45570/how-do-i-make-serial-work-on-the-raspberry-pi3-or-later-model]

```
/boot/config.txt
dtoverlay=pi3-disable-bt
enable_uart=1
dtoverlay=w1-gpio
dtparam=i2c_arm=off
dtparam=i2c=off
#dtoverlay=i2c-gpio,bus=1,i2c_gpio_sda=2,i2c_gpio_scl=3
dtoverlay=i2c-gpio,i2c_gpio_sda=2,i2c_gpio_scl=3
```


dependencies

boost program_options

https://github.com/zeromq/cppzmq/

pip install zmq
