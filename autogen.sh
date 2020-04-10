#!/bin/bash
touch README.md AUTHORS ChangeLog
autoreconf --force --install
./configure

