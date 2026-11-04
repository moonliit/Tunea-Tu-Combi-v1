# Justfile for Raylib C++ Project (Native Linux + WebAssembly via CMake)

build_dir := "build"
native_dir := build_dir + "/native"
web_dir := build_dir + "/web"
native_preset := "native-debug"
web_preset := "web-debug"

default: build

# Configure and build the default binary
build:
    cmake --preset {{native_preset}}
    cmake --build --preset {{native_preset}}
    @ln -sf {{native_dir}}/compile_commands.json compile_commands.json

# Explicitly build with Debug configuration (ensures -O0 and -g symbols)
debug:
    cmake -S . -B {{native_dir}} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_COLOR_DIAGNOSTICS=ON
    cmake --build {{native_dir}}
    @ln -sf {{native_dir}}/compile_commands.json compile_commands.json

# Explicitly build with Release configuration (optimized with -O3)
release:
    cmake -S . -B {{build_dir}}/native-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_COLOR_DIAGNOSTICS=ON
    cmake --build {{build_dir}}/native-release

# Builds and runs the native binary instantly if up-to-date
run: build
    ./{{native_dir}}/ray

# Builds and runs the native binary in GDB for debugging
gdb: debug
    gdb ./{{native_dir}}/ray

# Builds for WebAssembly. Use the preset with an activated emsdk, or emcmake
# when Emscripten is installed as a system package.
web:
    if printenv EMSDK >/dev/null 2>&1; then \
        cmake --preset {{web_preset}}; \
    else \
        emcmake cmake -S . -B {{web_dir}} -G "Unix Makefiles" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -DCMAKE_COLOR_DIAGNOSTICS=ON; \
    fi
    cmake --build {{web_dir}}

# Builds for Web and launches local test server via emrun on http://localhost:6931
serve: web
    @echo "Serving ray.html on http://localhost:6931..."
    emrun --port 6931 {{web_dir}}/ray.html

# Removes all build artifacts
clean:
    rm -rf {{build_dir}} {{build_dir}}/native-release compile_commands.json
