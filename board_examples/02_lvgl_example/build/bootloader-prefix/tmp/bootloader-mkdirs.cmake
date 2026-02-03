# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/rythmn/esp/v5.5.2/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/Users/rythmn/esp/v5.5.2/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader"
  "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader-prefix"
  "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader-prefix/tmp"
  "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader-prefix/src/bootloader-stamp"
  "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader-prefix/src"
  "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Volumes/rythmn103/code/esp/ESP32-S3-Touch-LCD-3.5-Demo/ESP-IDF/02_lvgl_example/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
