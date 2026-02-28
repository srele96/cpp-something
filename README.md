# cpp something

## Docs

Download sdl3 <https://github.com/libsdl-org/SDL/releases/tag/release-3.4.0> and put it in `sdl3` directory.

This should be automated, but it's not for now.

```txt
g++ main.cpp glad/src/glad.c -std=c++20 -I sdl2/include -I glad/include -L sdl2/lib -lSDL3 -lopengl32 -o main.exe && ./main.exe
```

```txt
g++ main.cpp glad/src/glad.c \
    -std=c++20 \
    -I glm \
    -I SDL3/include -I glad/include \
    -L SDL3/build -lSDL3 -lGL -ldl \
    -Wl,-rpath,$(pwd)/SDL3/build \
    -o main && ./main
```

Check if using local sdl3

```txt
ldd ./main | grep SDL
```

Generate compile commands on linux:

```txt
bear -- g++ main.cpp glad/src/glad.c \
    -std=c++20 \
    -I glm \
    -I SDL3/include -I glad/include \
    -L SDL3/build -lSDL3 -lGL -ldl \
    -Wl,-rpath,$(pwd)/SDL3/build \
    -o main && ./main
```
