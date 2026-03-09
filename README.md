# cpp something

## Todo

- Render light source
- Add another camera (abstract camera system)
- Cylinder, Sinusoidal cylinder, Cosinusoidal cylinder
- Textures
- Shadow mapping
- A library of primitives and render more complex shapes (a world)

## Docs

Run the following comand to build the SDL3 dynamic library and copy it to the root.

```txt
python sdl3.py
```

```txt
g++ main.cpp glad/src/glad.c -std=c++20 -I sdl2/include -I glad/include -L sdl2/lib -lSDL3 -lopengl32 -o main.exe && ./main.exe
```

```txt
g++ main.cpp glad/src/glad.c \
    -std=c++20 \
    -I glm \
    -I SDL3/include -I glad/include \
    -L SDL3/build -lSDL3 -lGL -ldl \
    -Wl,-rpath,'$ORIGIN' \
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
    -Wl,-rpath,'$ORIGIN' \
    -o main && ./main
```
