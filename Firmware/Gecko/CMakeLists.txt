cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(Gecko)

target_include_directories(app PRIVATE 
    src
    src/BLE
    Peripherals/BMA400
    Peripherals/Display
)

target_sources(app PRIVATE 
    src/main.c
    src/clock.c
    src/testing.c
    
    # Display
    src/Peripherals/Display/GC9A01A.c
    src/Peripherals/Display/LCD.c
    src/Peripherals/Display/assets.c
    
    # BMA400/Taps
    src/Peripherals/BMA400/bma400.c
    src/Peripherals/BMA400/common.c
    src/Peripherals/BMA400/taps.c

    # BLE
    src/BLE/SmartWatchService.c
    src/BLE/BLE.c

    # Power and Battery Management
    src/Peripherals/Power/battery.c
    
    # External Flash
    src/Peripherals/ExternalFlash/externalFlash.c

    # Buzzer
    src/Peripherals/Buzzer/buzzer.c
)
