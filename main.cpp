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
#include "SDL3/SDL_video.h"
#include "glad/glad.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/mat4x4.hpp" // IWYU pragma: keep
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp" // IWYU pragma: keep

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/rotate_vector.hpp" // This API is supposedly "experimental" for the past 10 years.

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
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
    mesh.vertices.push_back({.pos = center - (f.right * h) - (f.up * h),
                             .color = f.color,
                             .normal = f.normal}); // BottomLeft
    mesh.vertices.push_back({.pos = center + (f.right * h) - (f.up * h),
                             .color = f.color,
                             .normal = f.normal}); // TopLeft
    mesh.vertices.push_back({.pos = center + (f.right * h) + (f.up * h),
                             .color = f.color,
                             .normal = f.normal}); // TopRight
    mesh.vertices.push_back({.pos = center - (f.right * h) + (f.up * h),
                             .color = f.color,
                             .normal = f.normal}); // BottomRight

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

glm::vec3 sphericalCoord(const float yaw, const float pitch,
                         const float r = 1) {
  glm::vec3 coord{
      glm::sin(yaw) * glm::cos(pitch), //
      glm::sin(pitch),                 //
      -glm::cos(yaw) * glm::cos(pitch) //
  };
  return r * coord;
}

struct SphereMesh {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
};

SphereMesh generateSphere(const int latitudeBands, const int longitudeBands,
                          const float r = 1) {
  SphereMesh sphereMesh;

  const float pi{glm::pi<float>()};

  for (int i{0}; i <= latitudeBands; ++i) {
    const float floatI{static_cast<float>(i)};
    const float floatLatitudeBands{static_cast<float>(latitudeBands)};

    const float pitch{pi / 2 - (floatI / floatLatitudeBands) * pi};

    for (int j{0}; j <= longitudeBands; ++j) {
      const float floatJ{static_cast<float>(j)};
      const float floatLongitudeBands{static_cast<float>(longitudeBands)};

      const float yaw{(floatJ / floatLongitudeBands) * 2 * pi};

      const glm::vec3 normal{sphericalCoord(yaw, pitch, 1)};

      const glm::vec3 white{1.0f, 1.0f, 1.0f};

      sphereMesh.vertices.push_back({
          .pos = r * normal,
          .color = white,
          .normal = normal,
      });
    }
  }

  for (int i{0}; i < latitudeBands; ++i) {
    for (int j{0}; j < longitudeBands; ++j) {
      const int rowStride{longitudeBands + 1};

      const int topLeft{i * rowStride + j};
      const int topRight{topLeft + 1};
      const int bottomLeft{(i + 1) * rowStride + j};
      const int bottomRight{bottomLeft + 1};

      sphereMesh.indices.push_back(topLeft);
      sphereMesh.indices.push_back(topRight);
      sphereMesh.indices.push_back(bottomLeft);

      sphereMesh.indices.push_back(topRight);
      sphereMesh.indices.push_back(bottomRight);
      sphereMesh.indices.push_back(bottomLeft);
    }
  }

  return sphereMesh;
}

// TODO: Figure out if light source should light the quad plane accordingly?
// Because i want to use quad as a floor plane
MeshData generateQuad() {
  MeshData quad;

  float edge{0.5f};

  auto createVertex{[](const glm::vec3 &pos) -> Vertex {
    const glm::vec3 color{1.0f, 1.0f, 1.0f};
    const glm::vec3 normal{0.0f, 1.0f, 0.0f};

    return {.pos = pos, .color = color, .normal = normal};
  }};

  /*
    -0.5, 0, -0.5   left, front
    0.5, 0, -0.5    right, front
    0.5, 0, 0.5     right, back
    -0.5, 0, 0.5    left, back
  */

  quad.vertices = {
      createVertex({-edge, 0, -edge}), //
      createVertex({edge, 0, -edge}),  //
      createVertex({edge, 0, edge}),   //
      createVertex({-edge, 0, edge}),  //
  };

  quad.indices = {0, 1, 2, 2, 3, 0};

  return quad;
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
layout (location = 2) in vec3 aNormal;

out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;

// Does it matter if we output this and we don't have geometry shader in the next stage?
// Do we have to match VS_OUT and vs_out to be exactly same with different casing?
out VS_OUT {
  vec3 worldPos;
  vec3 vNormal;
  vec3 vColor;
} vs_out;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_model;

void main() {
  // TODO: Rotate a cube and inspect its sides

  vec4 worldPosition = u_model * vec4(aPos, 1.0);

  // To make the lighting color math to work, the computation must happen in the same space. We must not mix spaces.
  vFragPos = vec3(worldPosition);

  gl_Position = u_projection * u_view * worldPosition;

  vColor = aColor;

  // Rotate and move normal with the vertex, but prevent scaling from messing
  // up the normals perpendicularity.
  vNormal = mat3(transpose(inverse(u_model))) * aNormal;

  vs_out.worldPos = worldPosition.xyz;
  // Keep the outputs to keep the compatibility with fragment shader.
  // Is it fine to access vNormal like this? Since it's marked as 'out'?
  vs_out.vNormal = vNormal;
  vs_out.vColor = vColor;
}
)";

const std::string geometryShaderSource{R"(
#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
  vec3 worldPos;
  vec3 vNormal;
  vec3 vColor;
} gs_in[];

out vec3 vColor;

// It is fine to declare uniforms with the same name, in different shaders.
uniform mat4 u_view;
uniform mat4 u_projection;

// TODO: Make arrows instead of lines.
void drawLine(int i) {
  vColor = gs_in[i].vColor;

  gl_Position = u_projection * u_view * vec4(gs_in[i].worldPos, 1.0);
  EmitVertex();

  vec3 tip = gs_in[i].worldPos + (gs_in[i].vNormal * 1.0);
  gl_Position = u_projection * u_view * vec4(tip, 1.0);
  EmitVertex();

  EndPrimitive();
}

void main() {
  // We should already have a normal? among regular input from vertex shader?
  drawLine(0);
  drawLine(1);
  drawLine(2);
}
)"};

const std::string debugFragmentShaderSource{R"(
#version 330 core

out vec4 FragColor;

in vec3 vColor;

void main() {
  FragColor = vec4(vColor, 1.0);
}
)"};

const std::string fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 u_eyePosition;
uniform vec3 u_lightPosition;

void main() {
  /////////////////////////////////////////////////////////////////////////////
  // TODO: Write a more elaborate explanation in graphics-docs repo.
  //
  // NOTE:
  //
  // In order to get the lighting computation correctly, we MUST understand the
  // concept of spaces.
  //
  // - MODEL SPACE
  // - WORLD SPACE
  // - CAMERA SPACE
  // - CLIP SPACE
  //
  // The model space lives after model has been modified by the model matrix.
  // The model matrix moves the model from model space to the world space.
  // Every object lives in a world space. Imagine 4d being observing a 3d
  // world, everything seen AS IS is a world space. Hence, for the CURRENT
  // light computation, we want every object to live in the world space.
  /////////////////////////////////////////////////////////////////////////////

  vec3 lightColor = vec3(1.0, 1.0, 1.0);

  // AMBIENT
  float ambientStrength = 0.15;
  vec3 ambient = ambientStrength * lightColor;

  // We must normalize it because interpolation might break normals.
  vec3 normal = normalize(vNormal);

  vec3 lightDirection = normalize(u_lightPosition - vFragPos);

  // DIFFUSE
  float diff = max(dot(normal, lightDirection), 0.0);
  vec3 diffuse = diff * lightColor;

  // SPECULAR
  vec3 eyeDirection = normalize(u_eyePosition - vFragPos);
  vec3 reflectionDirection = reflect(-lightDirection, normal);

  float specularStrength = 0.5;
  float spec = pow(max(dot(eyeDirection, reflectionDirection), 0.0), 32);
  vec3 specular = specularStrength * spec * lightColor;

  // FragColor = vec4(r, g, b, 1.0);

  vec3 fragmentColor = (ambient + diffuse + specular) * vColor;

  FragColor = vec4(fragmentColor, 1.0);
}
)";

namespace Shader::Source {

const std::string vert_floor{R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vColor;
out vec3 vFragPos;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_model;
uniform vec3 u_eye;

void main() {
  vec4 worldPosition = u_model * vec4(aPos, 1.0);

  vec3 anchored = worldPosition.xyz;
  anchored.x += u_eye.x;
  anchored.z += u_eye.z;
  anchored.y = worldPosition.y;

  vFragPos = anchored;

  gl_Position = u_projection * u_view * vec4(anchored, 1.0);

  vColor = aColor;
}
)"};

const std::string frag_floor{R"(
#version 330 core

in vec3 vColor;
in vec3 vFragPos;

out vec4 FragColor;

void main() {
  // Make each checker sides 1 unit long
  vec2 pos = vFragPos.xz * 0.5;

  vec2 f = fract(pos);

  // The step() is faster if (f.x < 0.5) on GPU
  float sX = step(0.5, f.x);
  float sY = step(0.5, f.y);

  // XOR
  float checker = abs(sX - sY);

  // TODO: Try out the fade factor, the further the checkerboard fragment is
  // from the eye, the more it is faded out
  FragColor = vec4(vec3(checker), 1.0);
}
)"};

} // namespace Shader::Source

// TODO: Render a plane and use calculus to make it more interesting. Procedural
// terrain generation?

// TODO: Render a pipe in a sinus & cosinus shape. Remove the dead code which
// used to render sinus wave.

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

// TODO: Maybe allow usage of #define in shader source? But that doesnt seem to
// be opengl feature but rather wittiness of c++ developers.
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

void logLinkStatus(const GLuint shaderProgram, const std::string &prefix = "") {
  GLint success;

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << prefix << "Program link error:\n" << infoLog << std::endl;
  }
  std::cout << prefix << "Program link success.\n";
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
  /////////////////////////////////////////////////////////////////////////////

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

  // Synchronize the loop with the monitor refresh rate
  // This one line reduces gpu usage from 85% to <=5%
  SDL_GL_SetSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
  }

  glEnable(GL_DEPTH_TEST);

  glViewport(0, 0, window_width, window_height);

  /* CREATE VERTICES */
  std::vector<float> vertices = generateSineWave(500);
  int vertexCount = vertices.size() / 2;

  /////////////////////////////////////////////////////////////////////////////

  /* SHADER PROGRAM */

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader =
      compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);

  glLinkProgram(shaderProgram);

  logLinkStatus(shaderProgram, "(shaderProgram) -");

  GLuint debugGeometryShader{
      compileShader(GL_GEOMETRY_SHADER, geometryShaderSource)};
  GLuint debugFragmentShader{
      compileShader(GL_FRAGMENT_SHADER, debugFragmentShaderSource)};
  GLuint debugShaderProgram{glCreateProgram()};
  glAttachShader(debugShaderProgram, vertexShader);
  glAttachShader(debugShaderProgram, debugGeometryShader);
  glAttachShader(debugShaderProgram, debugFragmentShader);

  glLinkProgram(debugShaderProgram);

  logLinkStatus(debugShaderProgram, "(debugShaderProgram) - ");

  GLuint floorVertexShader{
      compileShader(GL_VERTEX_SHADER, Shader::Source::vert_floor)};
  GLuint floorFragmentShader{
      compileShader(GL_FRAGMENT_SHADER, Shader::Source::frag_floor)};

  GLuint floorProgram{glCreateProgram()};
  glAttachShader(floorProgram, floorVertexShader);
  glAttachShader(floorProgram, floorFragmentShader);

  glLinkProgram(floorProgram);

  logLinkStatus(floorProgram, "(floorProgram) - ");

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  glDeleteShader(debugGeometryShader);
  glDeleteShader(debugFragmentShader);
  glDeleteShader(floorFragmentShader);

  /* Shader program variable location */

  GLint timeLocation{glGetUniformLocation(shaderProgram, "time")};
  GLint viewLocation{glGetUniformLocation(shaderProgram, "u_view")};
  GLint projectionLocation{glGetUniformLocation(shaderProgram, "u_projection")};
  GLint modelLocation{glGetUniformLocation(shaderProgram, "u_model")};
  GLint eyePositionLocation{
      glGetUniformLocation(shaderProgram, "u_eyePosition")};
  GLint lightPositionLocation{
      glGetUniformLocation(shaderProgram, "u_lightPosition")};

  GLint debugViewLocation{glGetUniformLocation(debugShaderProgram, "u_view")};
  GLint debugProjectionLocation{
      glGetUniformLocation(debugShaderProgram, "u_projection")};
  GLint debugModelLocation{glGetUniformLocation(debugShaderProgram, "u_model")};

  GLint floorProjectionLocation{
      glGetUniformLocation(floorProgram, "u_projection")};
  GLint floorViewLocation{glGetUniformLocation(floorProgram, "u_view")};
  GLint floorModelLocation{glGetUniformLocation(floorProgram, "u_model")};
  GLint floorEyeLocation{glGetUniformLocation(floorProgram, "u_eye")};

  /////////////////////////////////////////////////////////////////////////////

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

  /////////////////////////////////////////////////////////////////////////////

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

  /////////////////////////////////////////////////////////////////////////////

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

  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  /////////////////////////////////////////////////////////////////////////////

  // I think it would be great if i had a class that handles fragment shader,
  // buffer creation, accepting and passing down uniforms, using the shader
  // during rendering, ...
  struct SphereBuffers {
    GLuint VAO, VBO, EBO;
  };

  const int latitudeBands{30};
  const int longitudeBands{30};
  SphereMesh sphereMesh{generateSphere(latitudeBands, longitudeBands, 8.0f)};

  SphereBuffers sphereBuffers;

  glGenVertexArrays(1, &sphereBuffers.VAO);
  glGenBuffers(1, &sphereBuffers.VBO);
  glGenBuffers(1, &sphereBuffers.EBO);

  glBindVertexArray(sphereBuffers.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, sphereBuffers.VBO);
  glBufferData(GL_ARRAY_BUFFER, sphereMesh.vertices.size() * sizeof(Vertex),
               sphereMesh.vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereBuffers.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sphereMesh.indices.size() * sizeof(unsigned int),
               sphereMesh.indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(0);

  // Disable attribute 1, so we can feed it value before render call
  glDisableVertexAttribArray(1);

  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  /////////////////////////////////////////////////////////////////////////////
  struct FloorBuffers {
    GLuint VAO, VBO, EBO;
  };

  FloorBuffers floorBuffers;

  glGenVertexArrays(1, &floorBuffers.VAO);
  glGenBuffers(1, &floorBuffers.VBO);
  glGenBuffers(1, &floorBuffers.EBO);

  // All relevant calls refer to this VAO
  glBindVertexArray(floorBuffers.VAO);

  MeshData floorMesh{generateQuad()};

  glBindBuffer(GL_ARRAY_BUFFER, floorBuffers.VBO);
  glBufferData(GL_ARRAY_BUFFER,
               // TODO: Perhaps mesh itself should know the size? In this way,
               // we expose the type of the vertex data, which exposes
               // internals, and may make changes difficult.
               floorMesh.vertices.size() * sizeof(Vertex),
               floorMesh.vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorBuffers.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               floorMesh.indices.size() * sizeof(unsigned int),
               floorMesh.indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Clean up currently active VAO
  glBindVertexArray(0);

  glm::mat4 floorModelMatrix{glm::identity<glm::mat4>()};

  // TODO: I wonder why is 100 units not enough to cover the whole frustum? The
  // far variable is 100 units, so...
  floorModelMatrix =
      glm::scale(floorModelMatrix, glm::vec3(100.0f, 1.0f, 100.0f));

  /////////////////////////////////////////////////////////////////////////////

  bool running = true;
  SDL_Event event;

  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "1");

  if (!SDL_SetWindowRelativeMouseMode(window, true)) {
    std::cerr << "Relative mouse mode failed: " << SDL_GetError() << "\n";
  }

  float yaw = 0.0f;
  float pitch = 0.0f;

  glm::vec3 eye{0.0f, 0.0f, 0.0f};

  const float sensitivity = 0.05f;

  glm::mat4 modelMatrix{1.0f};
  const float angle = glm::pi<float>() / 6;

  // TRS rule of thumb
  // Position = M_Translate * M_Rotate * M_Scale * V_Position
  modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 4.0f, -10.0f));
  modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(1.0f, 0.0f, 0.0f));

  glm::mat4 sphereModelMatrix{glm::translate(glm::identity<glm::mat4>(),
                                             glm::vec3(0.0f, 8.0f, -25.0f))};

  // TODO: Make light position an uniform and movable in the world. It should
  // help me understand if light is behaving as expected.
  glm::vec3 lightPosition{0.0f, -20.0f, -10.0f};

  Uint64 lastTime{SDL_GetTicks()};
  float deltaTime{0.0f};

  const float velocity{10.0f};

  // TODO: Add a way to keep multiple cameras. One which has ability to rotated
  // freely like current one, and another one with constrained pitch.
  // TODO: Consider adding camera not capable of flying around.
  // TODO: After adding another camera, debug if floor plane still does NOT
  // follow the camera. (Edit: Nevermind, the plane does move infinitely
  // alongside the camera.)
  struct Look {
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
  };

  Look look;

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
        // TODO: Mathematically check when do the two axes collapse and cause an
        // euler angle flip. Is this a gimbal lock? (Edit: No, it's not a gimbal
        // lock, but is known phenomenon) Check visually too, etc...

        // TODO: Ues recommended way to extract mouse movement coordinates.
        // SDL_GetMouseState
        // The xrel and yrel give a relative movement since the last frame,
        // which is awesome. We don't have to compute delta values.
        float xrel{event.motion.xrel};
        float yrel{event.motion.yrel};

        const float deltaYaw{xrel * sensitivity};
        const float deltaPitch{yrel * sensitivity};
        yaw += deltaYaw;
        pitch -= deltaPitch;

        // NOTE: The forward direction vector is derived by the following linear
        // algebra multiplication, where +x is right, and -z is front:
        // Ry(pitch)*Rx(yaw)*Vec(0,0,-1)
        // The logic belows essentially implements exactly that for front
        // vector.

        // NOTE: This direction computation removes the need for static world up
        // vector, but introduces roll when mouse moves in circles across the
        // screen. Be aware of this behavior.

        // TODO: Use static UP world axis. Lock (-pi/2 < pitch < pi/2). Avoid
        // roll effect.

        look.forward =
            glm::rotate(look.forward, glm::radians(-deltaPitch), look.right);
        look.up = glm::rotate(look.up, glm::radians(-deltaPitch), look.right);

        look.forward =
            glm::rotate(look.forward, glm::radians(-deltaYaw), look.up);
        look.right = glm::rotate(look.right, glm::radians(-deltaYaw), look.up);

        look.forward = glm::normalize(look.forward);
        look.right = glm::normalize(look.right);
        look.up = glm::normalize(look.up);

        // TODO: Use quaternions because they are superior for 3d rotations and
        // avoid gimbal lock and euler angle flip problems.
      }
    }

    const Uint64 currentTime{SDL_GetTicks()};
    deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    // TODO: Use quaternions (after fully understanding them)
    // Still prone to gimbal lock problem, hence should use quaternions
    // eventually

    const float speed{velocity * deltaTime};

    // Exhaust latest events
    // SDL_PumpEvents();
    const bool *keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_W]) {
      eye += look.forward * speed;
    }
    if (keystate[SDL_SCANCODE_S]) {
      eye -= look.forward * speed;
    }
    if (keystate[SDL_SCANCODE_A]) {
      eye -= look.right * speed;
    }
    if (keystate[SDL_SCANCODE_D]) {
      eye += look.right * speed;
    }
    if (keystate[SDL_SCANCODE_SPACE]) {
      eye += look.up * speed;
    }
    if (keystate[SDL_SCANCODE_LCTRL]) {
      eye -= look.up * speed;
    }

    glm::mat4 viewMatrix{
        lookAt(eye,
               // Cancel out the eye vector to form a free look at matrix
               eye + look.forward, look.up)};

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

    glUniform3fv(eyePositionLocation, 1, glm::value_ptr(eye));
    glUniform3fv(lightPositionLocation, 1, glm::value_ptr(lightPosition));
    // glUniform1f(timeLocation, _time);
    // glBindVertexArray(VAO);

    // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // glDrawArrays(GL_LINE_STRIP, 0, vertexCount);

    glBindVertexArray(wtf.VAO);
    glDrawElements(GL_TRIANGLES, cubeMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE,
                       glm::value_ptr(sphereModelMatrix));
    // Set constant color
    glVertexAttrib3f(1, 1.0f, 1.0f, 0.0f);
    glBindVertexArray(sphereBuffers.VAO);
    glDrawElements(GL_TRIANGLES, sphereMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUseProgram(debugShaderProgram);

    glUniformMatrix4fv(debugViewLocation, 1, GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(debugProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(debugModelLocation, 1, GL_FALSE,
                       glm::value_ptr(sphereModelMatrix));

    glDrawElements(GL_TRIANGLES, sphereMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUseProgram(floorProgram);

    glUniformMatrix4fv(floorViewLocation, 1, GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(floorProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(floorModelLocation, 1, GL_FALSE,
                       glm::value_ptr(floorModelMatrix));
    glUniform3fv(floorEyeLocation, 1, glm::value_ptr(eye));

    glBindVertexArray(floorBuffers.VAO);
    glDrawElements(GL_TRIANGLES, floorMesh.indices.size(), GL_UNSIGNED_INT, 0);

    /* ISSUE RENDER DIRECTIVE */
    SDL_GL_SwapWindow(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
