# Justfile for Raylib C++ Project (Native Linux + WebAssembly via CMake)

build_dir := "build"
native_dir := build_dir + "/native"
web_dir := build_dir + "/web"
native_preset := "native-debug"
web_preset := "web-debug"

default: build

# Configure and build the native Linux binary (with incremental compilation)
build:
    cmake --preset {{native_preset}}
    cmake --build --preset {{native_preset}}
    @ln -sf {{native_dir}}/compile_commands.json compile_commands.json

# Builds and runs the native binary instantly if up-to-date
run: build
    ./{{native_dir}}/ray

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
