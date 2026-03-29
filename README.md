# cpp something

## Todo

- Render light source
- Add another camera (abstract camera system)
- Sinusoidal cylinder, Cosinusoidal cylinder
- Textures
- A library of primitives and render more complex shapes (a world)
- Post-Processing: Bloom effect to the light source
- Decide on one (or more) light rendering techniques
  - Deffered Shading
  - Forward+ (compute shaders required, the screen is split into 16x16 tiles grid)
  - Forward rendering (Simple)
  - Clustered shading (Frustum is split into 3d-like voxel clusters)

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

### Sublime Text

Open `Tools > Developer > New Syntax` and, copy paste:

The config:

```yml
%YAML 1.2
---
name: C++ (GLSL Embedded)
file_extensions:
  - cpp
  - h
  - hpp
scope: source.c++
extends: Packages/C++/C++.sublime-syntax

contexts:
  prototype:
    - match: 'R"glsl\('
      embed: scope:source.glsl
      embed_scope: source.glsl.embedded.c++
      escape: '\)glsl"'
```

Save it `CTRL + S` as: `C++GLSL.sublime-syntax`.

Then, open the `C++` file and in the bottom right corner find: `C++` and click it, then find `C++ (GLSL Embedded)` and click it.

This should enable syntax highlighting for GLSL C++ embedded strings with `glsl`.
