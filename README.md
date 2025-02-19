# chatroom_server
Very barebones messaging service written in c for linux

dependencies:
    meson

compilation commands:
    ./compile.sh

run server:
    ./launch.sh 127.0.0.1 3001
    (compiles automatically if not already compiled)

run client:
    nc ip port
