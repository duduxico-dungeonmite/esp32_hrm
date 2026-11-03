cmake_minimum_required(VERSION 3.22)

set(IDF_PATH "$ENV{HOME}/.espressif/v6.1/esp-idf")
set(ENV{IDF_PATH} "${IDF_PATH}")

set(PYTHON "$ENV{HOME}/.espressif/python_env/idf6.1_py3.14_env/bin/python")
set(ENV{PYTHON} "${PYTHON}")

include("${IDF_PATH}/tools/cmake/project.cmake")

project(esp32_hrm)