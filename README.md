# cpp something

## Docs

Download sdl3 <https://github.com/libsdl-org/SDL/releases/tag/release-3.4.0> and put it in `sdl3` directory.

This should be automated, but it's not for now.

```txt
g++ main.cpp glad/src/glad.c -I sdl2/include -I glad/include -L sdl2/lib -lSDL3 -lopengl32 -o main.exe && ./main.exe
```
