#!/bin/bash

cppcheck --enable=warning,performance,information,style,portability,missingInclude . -Iinclude/ -j4 --platform=unix64 --std=posix --language=c++ -q -v