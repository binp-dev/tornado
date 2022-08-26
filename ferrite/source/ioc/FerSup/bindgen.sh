#!/usr/bin/env bash

bindgen \
    --no-doc-comments \
    --no-layout-tests \
    --no-prepend-enum-name \
    --size_t-is-usize \
    --default-enum-style=rust \
    _interface.h -o sys.rs