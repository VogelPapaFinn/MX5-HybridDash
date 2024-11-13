# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/libs/esp-idf/components/bootloader/subproject"
  "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader"
  "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader-prefix"
  "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader-prefix/tmp"
  "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader-prefix/src"
  "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/finng/Documents/MX5-HybridDash/Firmware/build-debug/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
