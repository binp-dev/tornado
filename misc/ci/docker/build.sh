#!/usr/bin/bash

cd "$(dirname "$0")"

docker build -t agerasev/debian-psc:0.3 .
