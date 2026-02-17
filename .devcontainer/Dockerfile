FROM mcr.microsoft.com/devcontainers/cpp

ENV NRF_SDK_ROOT=/opt/nordic

RUN apt-get update \
    && apt-get install -y build-essential \
    && apt-get install -y unzip \
    && apt-get install -y wget

RUN mkdir -p ${NRF_SDK_ROOT}
RUN wget https://files.nordicsemi.com/artifactory/nRF5-SDK/external/nRF5_SDK_v17.x.x/nRF5_SDK_17.1.1_8173c8a.zip -O nordic_sdk.zip
RUN unzip nordic_sdk.zip -d ${NRF_SDK_ROOT}
    # && mv nRF5_SDK_*/*  \
    # && rm -r ${NRF_SDK_ROOT}/examples \
    # && rm -r nRF5_SDK_* \
    # && rm nordic_sdk.zip
    # # && sed -i "/GNU_INSTALL_ROOT ?=/c\GNU_INSTALL_ROOT ?= /usr/bin/" ${NRF_SDK_ROOT}/components/toolchain/gcc/Makefile.posix \

# Install ARM GCC 9.3.1 (official release)
RUN wget https://developer.arm.com/-/media/files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2 -O /tmp/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2 \
    && tar -xjf /tmp/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2 -C /opt \
    && rm /tmp/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2

# Add ARM GCC 9.3.1 to PATH
ENV PATH="/opt/gcc-arm-none-eabi-9-2020-q2-update/bin:${PATH}"

ENV GNU_INSTALL_ROOT=/opt/gcc-arm-none-eabi-9-2020-q2-update/bin/
ENV GNU_VERSION=9.3.1

RUN wget https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil -O nrfutil
RUN chmod +x nrfutil
RUN mv nrfutil /usr/local/bin/nrfutil
RUN nrfutil install nrf5sdk-tools
RUN apt install -y srecord