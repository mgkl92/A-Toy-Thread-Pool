if [ -d "build" ]; then
    rm -rf build
fi

cmake -S. -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
