FROM mcr.microsoft.com/devcontainers/python:1-3.12-bullseye

# Installiere Systempakete & PlatformIO nur EINMAL beim Build
RUN apt-get update && apt-get install -y \
    git \
    gcc-avr \
    avr-libc \
    avrdude && \
    pip install -U platformio
