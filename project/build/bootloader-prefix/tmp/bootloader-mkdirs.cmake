# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/hss/esp/esp-idf/components/bootloader/subproject"
  "/home/hss/projetos/projeto-caixa-dagua/project/build/bootloader"
  "/home/hss/projetos/projeto-caixa-dagua/project/build/bootloader-prefix"
  "/home/hss/projetos/projeto-caixa-dagua/project/build/bootloader-prefix/tmp"
  "/home/hss/projetos/projeto-caixa-dagua/project/build/bootloader-prefix/src/bootloader-stamp"
  "/home/hss/projetos/projeto-caixa-dagua/project/build/bootloader-prefix/src"
  "/home/hss/projetos/projeto-caixa-dagua/project/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/hss/projetos/projeto-caixa-dagua/project/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
