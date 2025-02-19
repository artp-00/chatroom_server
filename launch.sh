#!/bin/sh

if [ $# -ne 2 ]; then
    echo "Wrong argument number."
    echo "expected:"
    echo "    ./launch.sh 127.0.0.1 3001"
    exit 3
fi

if [ ! -f ./build/chatroom_server ]; then
    ./compile.sh
fi

./build/chatroom_server $1 $2
