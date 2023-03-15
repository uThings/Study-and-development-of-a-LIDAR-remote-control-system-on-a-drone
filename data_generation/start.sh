#!/bin/bash

cd "${0%/*}"

if [ "$#" -ne 1 ]; then
    echo "Provide Timestamp"
    exit 1
fi

./voxl-logger -d $1 &

