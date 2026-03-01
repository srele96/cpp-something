#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "SDL3/SDL.h" // IWYU pragma: keep
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_timer.h"
#include "glad/glad.h"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/mat4x4.hpp" // IWYU pragma: keep
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp" // IWYU pragma: keep

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
};

struct MeshData {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
};

MeshData generateCube(float side) {
  float h = side / 2.0f;
  MeshData mesh;

  struct Face {
    glm::vec3 normal, right, up, color;
  };

  // TODO: Play with math here to understand ins and outs of how and why this
  // works.
  std::vector<Face> faces = {
      {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {1, 0, 0}},   // Front
      {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {0, 1, 0}}, // Back
      {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}},  // Top
      {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 1, 0}},  // Bottom
      {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {0, 1, 1}},  // Right
      {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 1}}   // Left
  };

  for (int i = 0; i < faces.size(); i++) {
    const auto &f = faces[i];
    glm::vec3 center = f.normal * h;

    // Add 4 edge vertices
    mesh.vertices.push_back(
        {center - (f.right * h) - (f.up * h), f.color}); // BottomLeft
    mesh.vertices.push_back(
        {center + (f.right * h) - (f.up * h), f.color}); // TopLeft
    mesh.vertices.push_back(
        {center + (f.right * h) + (f.up * h), f.color}); // TopRight
    mesh.vertices.push_back(
        {center - (f.right * h) + (f.up * h), f.color}); // BottomRight

    // Add face indices
    unsigned int offset = i * 4;

    // Does the order of indices matter?
    mesh.indices.push_back(offset + 0);
    mesh.indices.push_back(offset + 1);
    mesh.indices.push_back(offset + 2);
    // What if we have more or less indices?
    mesh.indices.push_back(offset + 2);
    mesh.indices.push_back(offset + 3);
    mesh.indices.push_back(offset + 0);
    // How many triangles can we render from 4 vertices?
  }
  return mesh;
}

glm::mat4 lookAt(const glm::vec3 &eye, const glm::vec3 &center,
                 const glm::vec3 &worldUp) {
  // Note: For learning purposes. I know glm::lookAt exists, but i derived math
  // and left this function implementation here as an example & reminder.
  const glm::vec3 forward{glm::normalize(center - eye)};
  const glm::vec3 right{glm::normalize(glm::cross(forward, worldUp))};
  const glm::vec3 up{glm::normalize(glm::cross(right, forward))};

  glm::mat4 view{right.x,          up.x,          -forward.x,        0.0f,
                 right.y,          up.y,          -forward.y,        0.0f,
                 right.z,          up.z,          -forward.z,        0.0f,
                 -dot(right, eye), -dot(up, eye), dot(forward, eye), 1};

  return view;
}

glm::mat4 projection(const float fov, const float aspectRatio, const float near,
                     const float far) {
  // Note: For learning purposes. I know glm::lookAt exists, but i derived math
  // and left this function implementation here as an example & reminder.
  const float f{1 / glm::tan(fov / 2)};

  // clang-format off
  glm::mat4 mat{f/aspectRatio, 0.0f, 0.0f,                               0.0f,
                0.0f,          f,    0.0f,                               0.0f,
                0.0f,          0.0f, -(far + near)/(far - near),         -1.0f,
                0.0f,          0.0f, -(2 * far * near)/(far - near),     0.0f};
  // clang-format on

  return mat;
}

std::string loadShaderSource(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open shader file: " + filePath);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

const std::string vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_model;

void main() {
  // TODO: Render parametric sphere, shade it, and check it out in 3D

  // TODO: Rotate a cube and inspect its sides

  gl_Position = u_projection * u_view * u_model * vec4(aPos, 1.0); 
  vColor = aColor;
}
)";

const std::string fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 vColor;

void main() {
  // FragColor = vec4(0.2, 0.8, 0.3, 1.0);
  FragColor = vec4(vColor, 1.0);
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

GLuint compileShader(GLenum type, const std::string &source) {
  GLuint shader = glCreateShader(type);

  const char *src = source.c_str();
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

  for (int i = 0; i < samples; ++i) {
    float x = -1.0f + 2.0f * i / (samples - 1);
    float y = glm::sin(x * 10.0f);

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
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  float window_width = 800;
  float window_height = 600;
  SDL_Window *window =
      SDL_CreateWindow("SDL3 Window", window_width, window_height,
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 1;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
  }

  glEnable(GL_DEPTH_TEST);

  glViewport(0, 0, window_width, window_height);

  /* CREATE VERTICES */
  std::vector<float> vertices = generateSineWave(500);
  int vertexCount = vertices.size() / 2;

  /* SHADER PROGRAM */

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader =
      compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

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

  GLint timeLocation{glGetUniformLocation(shaderProgram, "time")};
  GLint viewLocation{glGetUniformLocation(shaderProgram, "u_view")};
  GLint projectionLocation{glGetUniformLocation(shaderProgram, "u_projection")};
  GLint modelLocation{glGetUniformLocation(shaderProgram, "u_model")};

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

  // TODO: Stop thinking in terms of [-1, 1] NDC and use world or model space
  // coordinates to create models? Quad, create vertices
  // clang-format off
  std::vector<float> quad = {-0.8f, -0.8f, 0.8f, -0.8f,
                             -0.8f, 0.8f,  0.8f, 0.8f};
  // clang-format on

  // Quad, create vertex array object & vertex buffer object
  GLuint VAO, VBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  glBufferData(GL_ARRAY_BUFFER, quad.size() * sizeof(float), quad.data(),
               GL_STATIC_DRAW);
  // glVertexAttribPointer requires buffer to be bound
  // so it can asociate interpretation of data with data itself?
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  struct Wtf {
    GLuint VAO, VBO, EBO;
  };

  MeshData cubeMesh{generateCube(4.0f)};

  Wtf wtf;

  glGenVertexArrays(1, &wtf.VAO);
  glGenBuffers(1, &wtf.VBO);
  glGenBuffers(1, &wtf.EBO);

  glBindVertexArray(wtf.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, wtf.VBO);
  glBufferData(GL_ARRAY_BUFFER, cubeMesh.vertices.size() * sizeof(Vertex),
               cubeMesh.vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wtf.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               cubeMesh.indices.size() * sizeof(unsigned int),
               cubeMesh.indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  bool running = true;
  SDL_Event event;

  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "1");

  if (!SDL_SetWindowRelativeMouseMode(window, true)) {
    std::cerr << "Relative mouse mode failed: " << SDL_GetError() << "\n";
  }

  float yaw = 0.0f;
  float pitch = 0.0f;

  glm::vec3 eye{0.0f, 0.0f, 0.0f};

  const float sensitivity = 0.01f;

  // clang-format off
  glm::mat4 translateMatrix{
      1.0f, 0.0f,  0.0f,  0.0f,
      0.0f, 1.0f,  0.0f,  0.0f,
      0.0f, 0.0f,  1.0f,  0.0f,
      0.0f, 0.0f, -10.0f, 1.0f,
  };
  // clang-format on

  const float angle = glm::pi<float>() / 6;

  // clang-format off
  glm::mat4 rotateMatrix{
      1.0f, 0.0f,              0.0f,             0.0f,
      0.0f, glm::cos(angle),  -glm::sin(angle),  0.0f,
      0.0f, glm::sin(angle),   glm::cos(angle),  0.0f,
      0.0f, 0.0f,             -0.0f,             1.0f,
  };
  // clang-format on

  glm::mat4 modelMatrix = translateMatrix * rotateMatrix;

  Uint64 lastTime{SDL_GetTicks()};
  float deltaTime{0.0f};

  const float velocity{10.0f};

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        window_width = event.window.data1;
        window_height = event.window.data2;
        glViewport(0, 0, window_width, window_height);
      }
      if (event.type == SDL_EVENT_MOUSE_MOTION) {
        // TODO: Ues recommended way to extract mouse movement coordinates.
        // SDL_GetMouseState
        float xrel{event.motion.xrel};
        float yrel{event.motion.yrel};

        yaw += xrel * sensitivity;
        pitch -= yrel * sensitivity;

        if (pitch > 89.0f) {
          pitch = 89.0f;
        }
        if (pitch < -89.0f) {
          pitch = -89.0f;
        }
      }
    }

    const Uint64 currentTime{SDL_GetTicks()};
    deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    const float radYaw{glm::radians(yaw)};
    const float radPitch{glm::radians(pitch)};

    glm::vec3 direction;
    direction.x = glm::sin(radYaw) * glm::cos(radPitch);
    direction.y = glm::sin(radPitch);
    direction.z = -glm::cos(radYaw) * glm::cos(radPitch);

    // TODO: Use quaternions (after fully understanding them)
    // Still prone to gimbal lock problem, hence should use quaternions
    // eventually
    glm::vec3 forward{glm::normalize(direction)};
    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
    glm::vec3 right{glm::normalize(glm::cross(forward, worldUp))};
    glm::vec3 up{glm::normalize(glm::cross(right, forward))};

    const float speed{velocity * deltaTime};

    // Exhaust latest events
    // SDL_PumpEvents();
    const bool *keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_W]) {
      eye += forward * speed;
    }
    if (keystate[SDL_SCANCODE_S]) {
      eye -= forward * speed;
    }
    if (keystate[SDL_SCANCODE_A]) {
      eye -= right * speed;
    }
    if (keystate[SDL_SCANCODE_D]) {
      eye += right * speed;
    }
    if (keystate[SDL_SCANCODE_SPACE]) {
      eye += up * speed;
    }
    if (keystate[SDL_SCANCODE_LCTRL]) {
      eye -= up * speed;
    }

    glm::mat4 viewMatrix{
        lookAt(eye,
               // Cancel out the eye vector to form a free look at matrix
               eye + forward, worldUp)};

    // const float _time = SDL_GetTicks() / 500.0f;

    /* RENDER */
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float fov{glm::radians(60.0f)};
    const float aspectRatio{window_width / window_height};
    const float near{0.1f};
    const float far{100.0f};
    glm::mat4 projectionMatrix{projection(fov, aspectRatio, near, far)};

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    // glUniform1f(timeLocation, _time);
    // glBindVertexArray(VAO);

    // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // glDrawArrays(GL_LINE_STRIP, 0, vertexCount);

    glBindVertexArray(wtf.VAO);
    glDrawElements(GL_TRIANGLES, cubeMesh.indices.size(), GL_UNSIGNED_INT, 0);

    /* ISSUE RENDER DIRECTIVE */
    SDL_GL_SwapWindow(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
