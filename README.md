# chatroom_server
Very barebones messaging service written in c for linux

dependencies:
    meson

compilation commands:
    meson setup build
    meson compile -C build

run server:
    ./chatroom_server ip port

run client:
    nc ip port
