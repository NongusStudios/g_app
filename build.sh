if [ ! -d "build" ]; then
    mkdir build
    cd build && cmake -DCMAKE_BUILD_TYPE=Debug ..
fi

 cd build && make
