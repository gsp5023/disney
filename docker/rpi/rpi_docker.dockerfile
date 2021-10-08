FROM debian:10-slim

RUN apt-get update && apt-get install -y gcc-arm-linux-gnueabihf git build-essential

RUN git clone --progress --verbose https://github.com/raspberrypi/firmware.git --depth=1 pitools/firmware

RUN cp -a /pitools/firmware/opt/. /opt/

ARG rpi_files=RPI2

ADD $rpi_files/rpi_usr_include.tar.bz2 pitools
ADD $rpi_files/rpi_usr_lib.tar.bz2 pitools

WORKDIR /mount