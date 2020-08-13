## I2C

You need to use software driver to I2C, and therefore, disable bluetooth. This is for rPI 3 and up.

[https://raspberrypi.stackexchange.com/questions/45570/how-do-i-make-serial-work-on-the-raspberry-pi3-or-later-model]

```
sudo rpi-update

nano /boot/config.txt

dtoverlay=pi3-disable-bt
enable_uart=1
dtoverlay=w1-gpio
dtparam=i2c_arm=off
dtparam=i2c=off
#dtoverlay=i2c-gpio,bus=1,i2c_gpio_sda=2,i2c_gpio_scl=3
dtoverlay=i2c-gpio,i2c_gpio_sda=2,i2c_gpio_scl=3
```


## Dependencies

SSDV:

git submodule update --init --recursive

---
sudo apt-get install libboost-program-options-dev

---
git clone https://github.com/zeromq/libzmq.git

build with cmake

---
git clone https://github.com/zeromq/cppzmq.git

build with cmake

---
http://abyz.me.uk/rpi/pigpio/download.html
```
wget https://github.com/joan2937/pigpio/archive/master.zip
unzip master.zip
cd pigpio-master
make
sudo make install
```

---
pip3 install zmq exifread picamera psutil

sudo apt-get install imagemagick

## Run on one thread:

```
taskset -c 1 ./tracker
```
