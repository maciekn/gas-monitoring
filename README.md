# Overview

Collects photos through ESP32-cam module on SD card and exposes the most recent one through HTTP interface

Intention behind this simple app is to provide external monitoring to gas heating controller.

On later stages I plan to add image recognition to digitalize state.

# Instalation and usage
App requires `app_config.cpp` file with basic WiFi netform config to be existing. File can be based on `app_config.cpp.source` template file.

The app works on basic ESP32-CAM module