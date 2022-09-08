Import("env")
from shutil import copyfile

def move_bin(*args, **kwargs):
    print("Copying bin output to project directory...")
    target = str(kwargs['target'][0])
    if target == ".pio/build/esp32_1ch/firmware.bin":
        copyfile(target, 'bin/esp32_rf_weather_1ch.bin')
    elif target == ".pio/build/esp32_2ch/firmware.bin":
        copyfile(target, 'bin/esp32_rf_weather_2ch.bin')
    elif target == ".pio/build/esp32_3ch/firmware.bin":
        copyfile(target, 'bin/esp32_rf_weather_3ch.bin')
    print("Done.")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", move_bin)   #post action for .bin