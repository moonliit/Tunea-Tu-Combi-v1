# Justfile for Raylib C++ Project (Native Linux + WebAssembly via CMake)

build_dir := "build"
native_dir := build_dir + "/native"
web_dir := build_dir + "/web"
native_preset := "native-debug"
web_preset := "web-debug"
parallel_jobs := "8"

default: build

# Configure and build the default binary
build:
    cmake --preset {{native_preset}}
    cmake --build --preset {{native_preset}} --parallel {{parallel_jobs}}
    @ln -sf {{native_dir}}/compile_commands.json compile_commands.json

# Explicitly build with Debug configuration
debug:
    cmake -S . -B {{native_dir}} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_COLOR_DIAGNOSTICS=ON
    cmake --build {{native_dir}} --parallel {{parallel_jobs}}
    @ln -sf {{native_dir}}/compile_commands.json compile_commands.json

# Explicitly build with Release configuration
release:
    cmake -S . -B {{build_dir}}/native-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_COLOR_DIAGNOSTICS=ON
    cmake --build {{build_dir}}/native-release --parallel {{parallel_jobs}}

# Builds and runs the native binary instantly if up-to-date
run: build
    ./{{native_dir}}/ray

# Builds and runs the native binary in GDB for debugging
gdb: debug
    gdb ./{{native_dir}}/ray

# Builds for WebAssembly. Use the preset with an activated emsdk, or emcmake
web:
    if printenv EMSDK >/dev/null 2>&1; then \
        cmake --preset {{web_preset}}; \
    else \
        emcmake cmake -S . -B {{web_dir}} -G "Unix Makefiles" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -DCMAKE_COLOR_DIAGNOSTICS=ON; \
    fi
    cmake --build {{web_dir}} --parallel {{parallel_jobs}}

# Builds for Web and launches local test server via emrun on http://localhost:6931
serve: web
    @echo "Serving ray.html on http://localhost:6931..."
    emrun --port 6931 {{web_dir}}/ray.html

# Builds the game and creates or updates the infrastructure
aws-deploy: web aws-deploy-api aws-deploy-web

# Creates or updates only the AWS infrastructure and the Lambdas
aws-deploy-api:
    ./infra/deploy.sh backend

# Builds the game and uploads it to the deployed site
aws-deploy-web: web
    ./infra/deploy.sh web

# Shows where everything was deployed in AWS
aws-outputs:
    ./infra/deploy.sh outputs

# Deletes everything in AWS (players' data included, asks for confirmation)
aws-destroy:
    ./infra/deploy.sh destroy

# Removes all build artifacts
clean:
    rm -rf {{build_dir}} {{build_dir}}/native-release compile_commands.json
