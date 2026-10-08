# Justfile for Raylib C++ Project (Native Linux + WebAssembly)

appname      := "ray"
src_dir      := "src"
obj_dir      := "obj"
dep_dir      := "dep"
include_dir  := "include"
assets_dir   := "assets"
build_dir    := "build"
native_dir   := build_dir + "/native"
web_dir      := build_dir + "/web"

# List library subdirectories here (space-separated)
include_libs := "raylib raylib-cpp"

# Expands include_libs into -I./include/raylib -I./include/raylib-cpp
lib_flags    := if include_libs != "" { "-I./" + include_dir + "/" + replace(include_libs, " ", " -I./" + include_dir + "/") } else { "" }

# Assembles all include flags
incflags     := "-I./" + include_dir + " " + lib_flags + " -I./" + src_dir

cxx          := "g++"
cxxflags     := "-std=c++23 -Wall -g -O0 " + incflags
ldflags      := "-lraylib -lGL -lm -lpthread -ldl -lrt -lX11"

# Web compilation settings
emcxx        := "em++"
emflags      := "-std=c++23 -Wall -O2 " + incflags
raylib_wasm  := "lib/web/libraylib.web.a"
emlink       := "-s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY -s FORCE_FILESYSTEM=1"

# Default target
default: build

# Ensure required output directories exist
_prepare:
	@mkdir -p {{obj_dir}} {{dep_dir}} {{assets_dir}} {{native_dir}} {{web_dir}}

# Direct native build step
build: _prepare
	@echo "Building {{appname}} (native)..."
	@find {{src_dir}} -name '*.cpp' | xargs {{cxx}} {{cxxflags}} {{ldflags}} -o {{native_dir}}/{{appname}}

# Builds and runs the native binary
run: build
	./{{native_dir}}/{{appname}}

# Optional: Explicitly generates compile_commands.json using bear
compdb: _prepare
	rm compile_commands.json
	@echo "Generating compile_commands.json with bear..."
	@bear -- just build

# Builds for WebAssembly via Emscripten into build/web/
web: _prepare
	@echo "Building {{appname}} for WebAssembly..."
	{{emcxx}} {{emflags}} `find {{src_dir}} -name '*.cpp'` {{raylib_wasm}} -o {{web_dir}}/{{appname}}.html {{emlink}} \
		$( [ -n "$$(ls -A {{assets_dir}} 2>/dev/null)" ] && echo "--preload-file {{assets_dir}}" )

# Builds and serves to web via emrun on http://localhost:6931
serve: web
	@echo "Serving {{appname}}.html..."
	emrun --port 6931 {{web_dir}}/{{appname}}.html

# Removes all build output
clean:
	rm -rf {{obj_dir}} {{dep_dir}} {{build_dir}}
