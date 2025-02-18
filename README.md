# chatroom_server
Very barebones messaging service written in c for linux

dependencies:

compilation commands:
    meson setup build
    meson compile -C build

run server:
    ./epoll_server ip port

run client:
    nc ip port
