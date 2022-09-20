#!/usr/bin/bash

echo "ferrite:" && mypy -p ferrite && \
echo "example:" && cd example && mypy -p example
