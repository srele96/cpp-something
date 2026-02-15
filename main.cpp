#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#include "SDL3/SDL.h"
#include "glad/glad.h"

const std::string vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

void main() {
  // Parameters baked in:
  // FOV = 60°
  // aspect = 800 / 600 = 1.3333
  // near = 0.1
  // far  = 100.0

  float f = 1.0 / tan(radians(60.0) * 0.5);
  float aspect = 800.0 / 600.0;
  float near = 0.1;
  float far  = 100.0;

  mat4 projection = mat4(
    f / aspect, 0.0,  0.0,                          0.0,
    0.0,        f,    0.0,                          0.0,
    0.0,        0.0, -(far + near) / (far - near), -1.0,
    0.0,        0.0, -(2.0 * far * near) / (far - near), 0.0
  );

  mat4 model = mat4(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    -2, 2, -10, 1
  );

  // TODO: Render parametric sphere, shade it, and check it out in 3D

  // TODO: Rotate a cube and inspect its sides

  gl_Position = projection * model * vec4(aPos, -1, 1.0);
}
)";

const std::string fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main() {
  FragColor = vec4(0.2, 0.8, 0.3, 1.0);
}
)";

// const std::string vertexShaderSource = R"(
// #version 330 core
// layout (location = 0) in vec2 aPos;
// out vec2 vUV;

// void main() {
//   vUV = aPos * 0.5 + 0.5;

//   gl_Position = vec4(aPos, 0.0, 1.0);
// }
// )";

// const std::string fragmentShaderSource = R"(
// #version 330 core
// in vec2 vUV;
// out vec4 FragColor;

// uniform float time;

// void main() {
//   // map UV to NDC-ish space
//   float x = vUV.x * 2.0 - 1.0;           // [-1,1]
//   float y = vUV.y * 2.0 - 1.0;           // [-1,1]

//   // sine curve y = A * sin(freq*x + time)
//   float A = 0.3;
//   float freq = 3.0;
//   float curveY = A * sin(freq * x);

//   // thickness in NDC units (smaller = thinner line)
//   float thickness = 0.01;

//   float d = abs(y - curveY);

//   if (d < thickness)
//       FragColor = vec4(0.2, 0.8, 0.3, 1.0);   // line color
//   else
//       FragColor = vec4(0.1, 0.1, 0.15, 1.0);  // background
//   // FragColor = vec4(vUV, 0.0, 1.0);
// }
// )";

GLuint compileShader(GLenum type, const std::string& source) {
  GLuint shader = glCreateShader(type);

  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Shader compile error:\n" << infoLog << std::endl;
  }

  return shader;
}

std::vector<float> generateSineWave(int samples) {
  std::vector<float> vertices;

  for(int i = 0; i < samples; ++i) {
    float x = -1.0f + 2.0f * i / (samples -1);
    float y = std::sin(x * 10.0f);

    vertices.push_back(x);
    vertices.push_back(y * 0.3f);
  }

  return vertices;
}

// std::vector<float> generateSineWave(int samples) {
//   std::vector<float> vertices {
//     -0.5f, -0.5f,
//     0.5f, 0.5f,
//     -0.5f, 0.5f,
//     0.5f, -0.5f,
//     -0.5f, 0.0f
//   };
//   return vertices;
// }


int main() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(
    SDL_GL_CONTEXT_PROFILE_MASK,
    SDL_GL_CONTEXT_PROFILE_CORE
  );

  SDL_Window* window = SDL_CreateWindow(
    "SDL3 Window",
    800,
    600,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
  );

  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 1;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
  }

  glViewport(0, 0, 800, 600);

  /* CREATE VERTICES */
  std::vector<float> vertices = generateSineWave(500);
  int vertexCount = vertices.size() / 2;

  /* SHADER PROGRAM */

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  GLint success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "Program link error:\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  /* Shader program variable location */

  GLint timeLocation {glGetUniformLocation(shaderProgram, "time")};

  /* VAO + VBO */

/*
  GLuint VAO, VBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER,
               vertices.size() * sizeof(float),
               vertices.data(),
               GL_STATIC_DRAW);

  glVertexAttribPointer(
    0, 
    2, 
    GL_FLOAT, 
    GL_FALSE, 
    2 * sizeof(float), 
    (void*)0
  );
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
*/



  // Quad, create vertices
  std::vector<float> quad = {
    -0.8f, -0.8f,
     0.8f, -0.8f,
    -0.8f,  0.8f,
     0.8f,  0.8f
  };

  // Quad, create vertex array object & vertex buffer object
  GLuint VAO, VBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(
    GL_ARRAY_BUFFER,
    quad.size() * sizeof(float),
    quad.data(),
    GL_STATIC_DRAW
  );
  // glVertexAttribPointer requires buffer to be bound
  // so it can asociate interpretation of data with data itself?
  glVertexAttribPointer(
    0, 
    2, 
    GL_FLOAT, 
    GL_FALSE, 
    2 * sizeof(float), 
    (void*)0
  );
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  bool running = true;
  SDL_Event event;

  while (running) {
    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        glViewport(0, 0, event.window.data1, event.window.data2);
      }
    }

    const float _time = SDL_GetTicks() / 500.0f;

    /* RENDER */
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    // glUniform1f(timeLocation, _time);
    glBindVertexArray(VAO);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // glDrawArrays(GL_LINE_STRIP, 0, vertexCount);

    /* ISSUE RENDER DIRECTIVE */
    SDL_GL_SwapWindow(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
