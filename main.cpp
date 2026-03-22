#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "SDL3/SDL.h" // IWYU pragma: keep
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "glad/glad.h"
#include "glm/ext/matrix_clip_space.hpp"
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

  glm::vec3 white{1.0f, 1.0f, 1.0f};

  // TODO: Play with math here to understand ins and outs of how and why this
  // works.
  std::vector<Face> faces = {
      {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}, white},   // Front
      {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, white}, // Back
      {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}, white},  // Top
      {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}, white},  // Bottom
      {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}, white},  // Right
      {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, white}   // Left
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

MeshData generateSphere(const int latitudeBands, const int longitudeBands,
                        const float r = 1,
                        const glm::vec3 &color = {1.0f, 1.0f, 1.0f}) {
  MeshData sphereMesh;

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
          .color = color,
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

  quad.vertices = {
      createVertex({-edge, 0, -edge}), //
      createVertex({edge, 0, -edge}),  //
      createVertex({edge, 0, edge}),   //
      createVertex({-edge, 0, edge}),  //
  };

  quad.indices = {0, 1, 2, 2, 3, 0};

  return quad;
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

class AbstractShader {
public:
  virtual ~AbstractShader() = default;
  virtual AbstractShader &replace(const std::string &, const std::string &) = 0;
};

class Shader final : public AbstractShader {
private:
  std::string m_src;

public:
  Shader(std::string src,
         const std::unordered_map<std::string, std::string> &replacements = {})
      : m_src{std::move(src)} {
    for (const auto &[key, value] : replacements) {
      this->replace(key, value);
    }
  }
  Shader &replace(const std::string &key, const std::string &value) override {
    const std::string searchKey{"{{" + key + "}}"};
    size_t pos{0};
    while ((pos = m_src.find(searchKey, pos)) != std::string::npos) {
      m_src.replace(pos, searchKey.length(), value);
      pos += value.length();
    }
    return *this;
  }
  std::string src() const { return m_src; }
};

const std::string computeShadow{R"(
float computeShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2DShadow shadowMap) {
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

  // Normalize to [0, 1]
  projCoords = projCoords * 0.5 + 0.5;

  float currentDepth = projCoords.z;

  // https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
  float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

  float shadow = 0.0;

  vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

  for(int x = -1; x <= 1; ++x) {
    for(int y = -1; y <= 1; ++y) {
      // The z-component is the reference depth to compare against
      vec3 UVC = vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth - bias);

      // This returns a smoothly interpolated value between 0.0 and 1.0 automatically
      shadow += texture(shadowMap, UVC); 
    }    
  }

  // Average out the shadow for this pixel
  shadow /= 9.0;

  return shadow;
}
)"};

const std::string computeColor{R"(
// -----------------------
// Phong & Lambert shading

vec3 computeAmbient(vec3 lightColor) {
  float ambientStrength = 0.15;
  vec3 ambient = ambientStrength * lightColor;

  return ambient;
}

vec3 computeDiffuse(vec3 lightColor, vec3 lightDirection, vec3 normal) {
  float diffuseIntensity = max(dot(lightDirection, normal), 0.0);
  vec3 diffuse = diffuseIntensity * lightColor;

  return diffuse;
}

vec3 computeSpecular(vec3 lightColor, vec3 eyeDirection, vec3 lightReflection) {
  float specularStrength = 0.2;
  float specularIntensity = pow(max(dot(eyeDirection, lightReflection), 0.0), 128);
  vec3 specular = specularStrength * specularIntensity * lightColor;

  return specular;
}

struct LightingComponent {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

vec3 computeFragColor(LightingComponent light, vec3 baseColor, float shadow) {
  return baseColor * (light.ambient + shadow * (light.diffuse + light.specular));
}
)"};

const std::string vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;
out vec4 vFragPosLightSpace;

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
uniform mat4 u_lightProjection;
uniform mat4 u_lightView;

void main() {
  vec4 worldPosition = u_model * vec4(aPos, 1.0);

  // To make the lighting color math to work, the computation must happen in the same space. We must not mix spaces.
  vFragPos = vec3(worldPosition);

  gl_Position = u_projection * u_view * worldPosition;

  vColor = aColor;

  // Rotate and move normal with the vertex, but prevent scaling from messing
  // up the normals perpendicularity.
  vNormal = mat3(transpose(inverse(u_model))) * aNormal;
  vFragPosLightSpace = u_lightProjection * u_lightView * worldPosition;

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

const std::string basicFragmentShaderSource{R"(
#version 330 core

out vec4 FragColor;

in vec3 vColor;

void main() {
  FragColor = vec4(vColor, 1.0);
}
)"};

const Shader fragmentShaderSource{
    R"(
#version 330 core

in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;
in vec4 vFragPosLightSpace;

out vec4 FragColor;

uniform vec3 u_eyePosition;
uniform vec3 u_lightPosition;
uniform sampler2DShadow u_shadowMap;

{{computeShadow}}

{{computeColor}}

void main() {
  vec3 normal = normalize(vNormal);

  vec3 lightColor = vec3(1.0, 0.98, 0.9);

  vec3 lightDirection = normalize(u_lightPosition - vFragPos);
  vec3 lightReflection = reflect(-lightDirection, normal);
  vec3 eyeDirection = normalize(u_eyePosition - vFragPos);

  LightingComponent lightComponent;
  lightComponent.ambient = computeAmbient(lightColor);
  lightComponent.diffuse = computeDiffuse(lightColor, lightDirection, normal);
  lightComponent.specular = computeSpecular(lightColor, eyeDirection, lightReflection);

  float shadow = computeShadow(vFragPosLightSpace, normal, lightDirection, u_shadowMap);

  vec3 fragmentColor = computeFragColor(lightComponent, vColor, shadow);

  FragColor = vec4(fragmentColor, 1.0);
}
)",
    {{"computeColor", computeColor}, //
     {"computeShadow", computeShadow}}};

namespace ShaderSource {

const Shader vertFloor{R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vColor;
out vec3 vFragPos;
out vec3 vNormal;
out vec4 vFragPosLightSpace;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_lightProjection;
uniform mat4 u_lightView;

void main() {
  vec4 worldPosition = u_model * vec4(aPos, 1.0);

  gl_Position = u_projection * u_view * worldPosition;

  vFragPos = vec3(worldPosition.xyz);
  vColor = aColor;
  vNormal = mat3(transpose(inverse(u_model))) * aNormal;
  vFragPosLightSpace = u_lightProjection * u_lightView * worldPosition;
}
)"};

// TODO: Change computing light direction to accepting uniform light direction.
// Global illumination is a directional light, not a point light or spotlight.
// Global light rays do not have a position, only a direction.

const Shader fragFloor{
    R"(
#version 330 core

in vec3 vColor;
in vec3 vFragPos;
in vec3 vNormal;
in vec4 vFragPosLightSpace;

out vec4 FragColor;

uniform vec3 u_eyePosition;
uniform vec3 u_lightPosition;
uniform sampler2DShadow u_shadowMap;

{{computeShadow}}

{{computeColor}}

vec3 computeChecker(vec3 fragPos) {
  // Make each checker sides 1 unit long
  vec2 pos = fragPos.xz * 0.5;

  vec2 f = fract(pos);

  // The step() is faster if (f.x < 0.5) on GPU
  float sX = step(0.5, f.x);
  float sY = step(0.5, f.y);

  // XOR
  float checker = abs(sX - sY);

  return vec3(checker);
}

void main() {
  vec3 normal = normalize(vNormal);

  vec3 lightColor = vec3(1.0, 0.98, 0.9);

  vec3 lightDirection = normalize(u_lightPosition - vFragPos);
  vec3 lightReflection = reflect(-lightDirection, normal);
  vec3 eyeDirection = normalize(u_eyePosition - vFragPos);

  LightingComponent lightComponent;
  lightComponent.ambient = computeAmbient(lightColor);
  lightComponent.diffuse = computeDiffuse(lightColor, lightDirection, normal);
  lightComponent.specular = computeSpecular(lightColor, eyeDirection, lightReflection);

  vec3 checkerColor = computeChecker(vFragPos);

  float shadow = computeShadow(vFragPosLightSpace, normal, lightDirection, u_shadowMap);

  vec3 fragmentColor = computeFragColor(lightComponent, checkerColor, shadow);

  // TODO: Try out the fade factor, the further the checkerboard fragment is
  // from the eye, the more it is faded out
  FragColor = vec4(fragmentColor, 1.0);
}
)",
    {{"computeShadow", computeShadow}, //
     {"computeColor", computeColor}}};

} // namespace ShaderSource

// TODO: Render a plane and use calculus to make it more interesting. Procedural
// terrain generation?

// TODO: Render a pipe in a sinus & cosinus shape. Remove the dead code which
// used to render sinus wave.

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

void logLinkStatus(const GLuint shaderProgram, const std::string &prefix = "");

namespace ShadowMapping {

const std::string vert{R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

void main() {
  gl_Position = u_projection * u_view * u_model * vec4(aPos, 1.0);
}
)"};

GLuint createProgram() {
  GLuint depthProgram{glCreateProgram()};
  GLuint depthVertexShader{compileShader(GL_VERTEX_SHADER, vert)};
  glAttachShader(depthProgram, depthVertexShader);
  glLinkProgram(depthProgram);
  logLinkStatus(depthProgram, "(depthProgram) - ");
  glDeleteShader(depthVertexShader);

  return depthProgram;
}

struct DepthProgram {
  GLuint program;
  GLint projectionLocation;
  GLint viewLocation;
  GLint modelLocation;
};

DepthProgram createDepthProgram() {
  DepthProgram depthProgram{.program = createProgram()};

  depthProgram.projectionLocation =
      glGetUniformLocation(depthProgram.program, "u_projection");
  depthProgram.viewLocation =
      glGetUniformLocation(depthProgram.program, "u_view");
  depthProgram.modelLocation =
      glGetUniformLocation(depthProgram.program, "u_model");

  return depthProgram;
}

struct LightMatrix {
  glm::mat4 view;
  glm::mat4 projection;
};

struct CreateLightMatrixParam {
  glm::mat4 projectionMatrix;
  glm::mat4 viewMatrix;
  glm::vec3 worldUp;
  glm::vec3 lightDirection;
};

LightMatrix createLightMatrix(const CreateLightMatrixParam &param) {
  const auto &projectionMatrix{param.projectionMatrix};
  const auto &viewMatrix{param.viewMatrix};
  const auto &worldUp{param.worldUp};
  const auto &lightDirection{param.lightDirection};

  if (glm::abs(glm::dot(lightDirection, worldUp)) >= 0.9999f) {
    throw std::runtime_error(
        "Light direction and world up must not be parallel.");
  }

  auto cameraInverse{glm::inverse(projectionMatrix * viewMatrix)};
  std::vector<glm::vec3> corners;

  // Collect corner points
  for (float x = 0.0f; x < 2.0f; ++x) {
    for (float y = 0.0f; y < 2.0f; ++y) {
      for (float z = 0.0f; z < 2.0f; ++z) {
        const float xPos{2 * x - 1};
        const float yPos{2 * y - 1};
        const float zPos{2 * z - 1};

        glm::vec4 pt{cameraInverse * glm::vec4{xPos, yPos, zPos, 1.0f}};
        // Todo: Derive mathematically on paper
        corners.push_back(glm::vec3(pt / pt.w));
      }
    }
  }

  // Find frustum center
  glm::vec3 center{0.0f};
  // Note: Centroid formula, a center of a mass
  for (const auto &corner : corners) {
    center += corner;
  }
  center /= corners.size();

  // Find the largest enclosing radius as light position distance
  // Note: This is required to precisely illuminate visible area in the
  // frustum.
  float radius{0.0f};
  for (const auto &corner : corners) {
    radius = glm::max(radius, glm::length(corner - center));
  }
  glm::vec3 position{center - radius * lightDirection};

  // The light position and orientation matrix
  glm::mat4 lightView{glm::lookAt(position, center, worldUp)};

  // Move the frustum corners into light space before creating ortho
  // projection
  std::vector<glm::vec3> lightSpaceCorners(corners.size());
  std::transform(corners.begin(), corners.end(), lightSpaceCorners.begin(),
                 [&lightView](const glm::vec3 &corner) {
                   return glm::vec3{lightView * glm::vec4(corner, 1.0f)};
                 });

  // Get the ortho projection bounds
  // Note: If shadows jitter due to subpixel changes in minBound & maxBound, try
  // using spherical bounding box.
  // Note: Intentionally does not include the bounding box adjustment for
  // object right behind camera to cast the shadows.
  // Collect minimum and maximum component-wise bounds.
  glm::vec3 minBound{lightSpaceCorners.at(0)};
  glm::vec3 maxBound{lightSpaceCorners.at(0)};
  for (const auto &corner : lightSpaceCorners) {
    minBound = glm::min(minBound, corner);
    maxBound = glm::max(maxBound, corner);
  }
  // Looking down the -z axis, adjust to positive values.
  const float near{-maxBound.z};
  const float far{-minBound.z};

  glm::mat4 lightProjection{
      glm::ortho(minBound.x, maxBound.x, minBound.y, maxBound.y, near, far)};

  return {.view = lightView, .projection = lightProjection};
}
} // namespace ShadowMapping

void logLinkStatus(const GLuint shaderProgram, const std::string &prefix) {
  GLint success;

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << prefix << "Program link error:\n" << infoLog << std::endl;
  }
  std::cout << prefix << "Program link success.\n";
}

// TODO: Generate distortion over a plane. This is where we can practically
// apply calculus.

// TODO: Generate cylider, sinusoidal and cosinusoidal cylidern.

int main() {
  /////////////////////////////////////////////////////////////////////////////

  /* SDL setup */
  if (!SDL_Init(SDL_INIT_VIDEO)) {
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

  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "1");

  if (!SDL_SetWindowRelativeMouseMode(window, true)) {
    std::cerr << "Relative mouse mode failed: " << SDL_GetError() << "\n";
  }

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
  }

  /* GL setup */

  glEnable(GL_DEPTH_TEST);

  glViewport(0, 0, window_width, window_height);

  /////////////////////////////////////////////////////////////////////////////

  /* SHADER PROGRAM */

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader =
      compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.src());

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);

  glLinkProgram(shaderProgram);

  logLinkStatus(shaderProgram, "(shaderProgram) -");

  GLuint debugGeometryShader{
      compileShader(GL_GEOMETRY_SHADER, geometryShaderSource)};
  GLuint basicFragmentShader{
      compileShader(GL_FRAGMENT_SHADER, basicFragmentShaderSource)};
  GLuint debugShaderProgram{glCreateProgram()};
  glAttachShader(debugShaderProgram, vertexShader);
  glAttachShader(debugShaderProgram, debugGeometryShader);
  glAttachShader(debugShaderProgram, basicFragmentShader);

  glLinkProgram(debugShaderProgram);

  logLinkStatus(debugShaderProgram, "(debugShaderProgram) - ");

  GLuint lightSourceProgram{glCreateProgram()};
  glAttachShader(lightSourceProgram, vertexShader);
  glAttachShader(lightSourceProgram, basicFragmentShader);

  glLinkProgram(lightSourceProgram);

  logLinkStatus(lightSourceProgram, "(lightSourceProgram) - ");

  GLuint floorVertexShader{
      compileShader(GL_VERTEX_SHADER, ShaderSource::vertFloor.src())};
  GLuint floorFragmentShader{
      compileShader(GL_FRAGMENT_SHADER, ShaderSource::fragFloor.src())};

  GLuint floorProgram{glCreateProgram()};
  glAttachShader(floorProgram, floorVertexShader);
  glAttachShader(floorProgram, floorFragmentShader);

  glLinkProgram(floorProgram);

  logLinkStatus(floorProgram, "(floorProgram) - ");

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  glDeleteShader(debugGeometryShader);
  glDeleteShader(basicFragmentShader);
  glDeleteShader(floorFragmentShader);

  /* UNIFORMS */

  // TODO: Automate uniform location retrieval and throw if an uniform location
  // does not have a set value. It's repetitive and difficult to manually find a
  // shader program that matches modified shader source, find the uniform
  // location, and pass the value to it.
  GLint projectionLocation{glGetUniformLocation(shaderProgram, "u_projection")};
  GLint viewLocation{glGetUniformLocation(shaderProgram, "u_view")};
  GLint modelLocation{glGetUniformLocation(shaderProgram, "u_model")};
  GLint eyePositionLocation{
      glGetUniformLocation(shaderProgram, "u_eyePosition")};
  GLint lightPositionLocation{
      glGetUniformLocation(shaderProgram, "u_lightPosition")};
  GLint lightProjectionLocation{
      glGetUniformLocation(shaderProgram, "u_lightProjection")};
  GLint lightViewLocation{glGetUniformLocation(shaderProgram, "u_lightView")};
  GLint shadowMapLocation{glGetUniformLocation(shaderProgram, "u_shadowMap")};

  GLint debugProjectionLocation{
      glGetUniformLocation(debugShaderProgram, "u_projection")};
  GLint debugViewLocation{glGetUniformLocation(debugShaderProgram, "u_view")};
  GLint debugModelLocation{glGetUniformLocation(debugShaderProgram, "u_model")};
  // TODO: Figure out if we need to pass these uniforms to this shader program.
  // here: u_lightProjection, u_lightView, u_shadowMap

  GLint floorProjectionLocation{
      glGetUniformLocation(floorProgram, "u_projection")};
  GLint floorViewLocation{glGetUniformLocation(floorProgram, "u_view")};
  GLint floorModelLocation{glGetUniformLocation(floorProgram, "u_model")};
  GLint floorEyePositionLocation{
      glGetUniformLocation(floorProgram, "u_eyePosition")};
  GLint floorLightPositionLocation{
      glGetUniformLocation(floorProgram, "u_lightPosition")};

  GLint floorLightProjectionLocation{
      glGetUniformLocation(floorProgram, "u_lightProjection")};
  GLint floorLightViewLocation{
      glGetUniformLocation(floorProgram, "u_lightView")};
  GLint floorShadowMapLocation{
      glGetUniformLocation(floorProgram, "u_shadowMap")};

  GLint lightSourceProjectionLocation{
      glGetUniformLocation(lightSourceProgram, "u_projection")};
  GLint lightSourceViewLocation{
      glGetUniformLocation(lightSourceProgram, "u_view")};
  GLint lightSourceModelLocation{
      glGetUniformLocation(lightSourceProgram, "u_model")};
  // TODO: Figure out if we need to pass these uniforms to this shader program.
  // here: u_lightProjection, u_lightView, u_shadowMap

  /////////////////////////////////////////////////////////////////////////////

  struct CubeBuffers {
    GLuint VAO, VBO, EBO;
  };

  MeshData cubeMesh{generateCube(4.0f)};

  CubeBuffers cubeBuffers;

  glGenVertexArrays(1, &cubeBuffers.VAO);
  glGenBuffers(1, &cubeBuffers.VBO);
  glGenBuffers(1, &cubeBuffers.EBO);

  glBindVertexArray(cubeBuffers.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, cubeBuffers.VBO);
  glBufferData(GL_ARRAY_BUFFER, cubeMesh.vertices.size() * sizeof(Vertex),
               cubeMesh.vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeBuffers.EBO);
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
  MeshData sphereMesh{generateSphere(latitudeBands, longitudeBands, 8.0f)};

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

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

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

  glBindVertexArray(0);

  glm::mat4 floorModelMatrix{glm::identity<glm::mat4>()};

  // TODO: I wonder why is 100 units not enough to cover the whole frustum? The
  // far variable is 100 units, so... Update: The front of the floor is half of
  // the full length of the frustum.
  floorModelMatrix =
      glm::scale(floorModelMatrix, glm::vec3(200.0f, 1.0f, 200.0f));

  /////////////////////////////////////////////////////////////////////////////

  // TODO: Create reusable classes.
  MeshData lightSourceMesh{
      generateSphere(20.0f, 20.0f, 1.0f, glm::vec3{1.0f, 1.0f, 0.0f})};

  // I think it would be great if i had a class that handles fragment shader,
  // buffer creation, accepting and passing down uniforms, using the shader
  // during rendering, ...
  struct LightSourceBuffers {
    GLuint VAO, VBO, EBO;
  };

  LightSourceBuffers lightSourceBuffers;

  glGenVertexArrays(1, &lightSourceBuffers.VAO);
  glGenBuffers(1, &lightSourceBuffers.VBO);
  glGenBuffers(1, &lightSourceBuffers.EBO);

  glBindVertexArray(lightSourceBuffers.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, lightSourceBuffers.VBO);
  glBufferData(GL_ARRAY_BUFFER,
               lightSourceMesh.vertices.size() * sizeof(Vertex),
               lightSourceMesh.vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightSourceBuffers.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               lightSourceMesh.indices.size() * sizeof(unsigned int),
               lightSourceMesh.indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  // TODO: Render light source object. Make light position movable in the world.
  // It should help me understand if light is behaving as expected.
  glm::vec3 lightPosition{0.0f, 40.0f, -10.0f};

  glm::mat4 lightSourceModelMatrix{glm::identity<glm::mat4>()};
  lightSourceModelMatrix =
      glm::translate(lightSourceModelMatrix, lightPosition);

  /////////////////////////////////////////////////////////////////////////////

  struct DepthMap {
    GLuint framebuffer;
    GLuint texture;
    const unsigned int TEXTURE_WIDTH{2048};
    const unsigned int TEXTURE_HEIGHT{2048};
    const std::array<float, 4> borderColor{1.0f, 1.0f, 1.0f, 1.0f};
  };

  // Create framebuffer
  DepthMap depthMap;
  glGenFramebuffers(1, &depthMap.framebuffer);
  glGenTextures(1, &depthMap.texture);

  // Start describing the texture
  glBindTexture(GL_TEXTURE_2D, depthMap.texture);

  // Store depth component in the texture, and tell graphics driver to give us
  // higher precision shadows
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, depthMap.TEXTURE_WIDTH,
               depthMap.TEXTURE_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

  // -- BEGIN -- Required texture parameters for smooth shadow (anti aliasing)
  // https://wikis.khronos.org/opengl/Texture
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  // -- END -- Required texture parameters for smooth shadow (anti aliasing)

  // A value thats far out gets border color assigned
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  // A border color value means a light ray hit it and should be lit
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                   depthMap.borderColor.data());

  // Stop describing the texture
  // Should be safe to unbind texture?
  glBindTexture(GL_TEXTURE_2D, 0);

  // Start describing the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, depthMap.framebuffer);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthMap.texture, 0);

  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Framebuffer is complete." << std::endl;
  } else {
    std::cout << "Framebuffer not complete!" << std::endl;
  }

  // Stop describing the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  ShadowMapping::DepthProgram depthProgram{ShadowMapping::createDepthProgram()};

  /////////////////////////////////////////////////////////////////////////////

  // TODO: Textures
  // TODO: UV Coordinates
  // TODO: Filtering: MIN_FILTER | MAG_FILTER - GL_NEAREST | GL_LINEAR
  // TODO: Wrapping

  /////////////////////////////////////////////////////////////////////////////

  bool running = true;
  SDL_Event event;

  float yaw = 0.0f;
  float pitch = 0.0f;

  glm::vec3 eye{0.0f, 5.0f, 0.0f};

  const float sensitivity = 0.05f;

  glm::mat4 modelMatrix{1.0f};
  const float angle = glm::pi<float>() / 6;

  // TRS rule of thumb
  // Position = M_Translate * M_Rotate * M_Scale * V_Position
  modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 4.0f, -10.0f));
  modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(1.0f, 0.0f, 0.0f));

  glm::mat4 sphereModelMatrix{glm::translate(glm::identity<glm::mat4>(),
                                             glm::vec3(0.0f, 8.0f, -25.0f))};

  Uint64 lastTime{SDL_GetTicks()};
  float deltaTime{0.0f};

  const float velocity{10.0f};

  // TODO: Add a way to keep multiple cameras. One which has ability to rotated
  // freely, and another one with constrained pitch.
  // TODO: Consider adding camera not capable of flying around.
  struct Look {
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
  };

  glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

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
        // euler angle flip.

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

        if (pitch >= 89.0f) {
          pitch = 89.0f;
        }
        if (pitch <= -89.0f) {
          pitch = -89.0f;
        }

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

        // NOTE: The camera is always orinted relative to static world up, right
        // and forward vectors. That way we use absolute yaw and pitch instead
        // of delta values.
        glm::vec3 forward{0.0f, 0.0f, -1.0f};
        const glm::vec3 right{1.0f, 0.0f, 0.0f};

        forward = glm::rotate(forward, glm::radians(pitch), right);
        forward = glm::rotate(forward, glm::radians(-yaw), worldUp);

        look.forward = glm::normalize(forward);
        look.right = glm::normalize(glm::cross(look.forward, worldUp));
        look.up = glm::normalize(glm::cross(look.right, look.forward));
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
        glm::lookAt(eye,
                    // Cancel out the eye vector to form a free look at matrix
                    eye + look.forward, worldUp)};

    const float fov{glm::radians(60.0f)};
    const float aspectRatio{window_width / window_height};
    const float near{0.1f};
    const float far{100.0f};
    glm::mat4 projectionMatrix{glm::perspective(fov, aspectRatio, near, far)};

    const glm::vec3 lightDirection{glm::normalize(
        glm::rotate(glm::vec3{0.0f, -1.0f, 0.0f}, -glm::pi<float>() / 6.0f,
                    glm::vec3{1.0f, 0.0f, 0.0f}))};

    const auto lightMatrix{
        ShadowMapping::createLightMatrix({.projectionMatrix = projectionMatrix,
                                          .viewMatrix = viewMatrix,
                                          .worldUp = worldUp,
                                          .lightDirection = lightDirection})};

    /* SHADOW PASS */

    // TODO: Make shadows be affected by each light source. Currently only
    // directional light affects shadows.
    glViewport(0, 0, depthMap.TEXTURE_WIDTH, depthMap.TEXTURE_HEIGHT);

    // Use depth framebuffer for this render pass
    glBindFramebuffer(GL_FRAMEBUFFER, depthMap.framebuffer);

    glClear(GL_DEPTH_BUFFER_BIT);

    // Fix shadow acne
    glCullFace(GL_FRONT);

    glUseProgram(depthProgram.program);

    glUniformMatrix4fv(depthProgram.projectionLocation, 1, GL_FALSE,
                       glm::value_ptr(lightMatrix.projection));
    glUniformMatrix4fv(depthProgram.viewLocation, 1, GL_FALSE,
                       glm::value_ptr(lightMatrix.view));

    glUniformMatrix4fv(depthProgram.modelLocation, 1, GL_FALSE,
                       glm::value_ptr(modelMatrix));

    glBindVertexArray(cubeBuffers.VAO);
    glDrawElements(GL_TRIANGLES, cubeMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUniformMatrix4fv(depthProgram.modelLocation, 1, GL_FALSE,
                       glm::value_ptr(sphereModelMatrix));

    glBindVertexArray(sphereBuffers.VAO);
    glDrawElements(GL_TRIANGLES, sphereMesh.indices.size(), GL_UNSIGNED_INT, 0);

    // Revert culling to normal one
    glCullFace(GL_BACK);

    // Unbind depth framebuffer, the render pass is done
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* LIGHT PASS */

    glViewport(0, 0, window_width, window_height);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap.texture);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glUniform3fv(eyePositionLocation, 1, glm::value_ptr(eye));
    glUniform3fv(lightPositionLocation, 1, glm::value_ptr(lightPosition));

    glUniformMatrix4fv(lightProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(lightMatrix.projection));
    glUniformMatrix4fv(lightViewLocation, 1, GL_FALSE,
                       glm::value_ptr(lightMatrix.view));

    // Texture unit 0 is reserved for color/diffuse
    glUniform1i(shadowMapLocation, 1);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    glBindVertexArray(cubeBuffers.VAO);
    glDrawElements(GL_TRIANGLES, cubeMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE,
                       glm::value_ptr(sphereModelMatrix));

    glBindVertexArray(sphereBuffers.VAO);
    glDrawElements(GL_TRIANGLES, sphereMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUseProgram(debugShaderProgram);

    glUniformMatrix4fv(debugProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(debugViewLocation, 1, GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(debugModelLocation, 1, GL_FALSE,
                       glm::value_ptr(sphereModelMatrix));

    glDrawElements(GL_TRIANGLES, sphereMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUseProgram(floorProgram);

    glUniformMatrix4fv(floorProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(floorViewLocation, 1, GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(floorModelLocation, 1, GL_FALSE,
                       glm::value_ptr(floorModelMatrix));
    glUniform3fv(floorEyePositionLocation, 1, glm::value_ptr(eye));
    glUniform3fv(floorLightPositionLocation, 1, glm::value_ptr(lightPosition));

    glUniformMatrix4fv(floorLightProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(lightMatrix.projection));
    glUniformMatrix4fv(floorLightViewLocation, 1, GL_FALSE,
                       glm::value_ptr(lightMatrix.view));

    // Texture unit 0 is reserved for color/diffuse
    glUniform1i(floorShadowMapLocation, 1);

    glBindVertexArray(floorBuffers.VAO);
    glDrawElements(GL_TRIANGLES, floorMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glUseProgram(lightSourceProgram);

    glUniformMatrix4fv(lightSourceProjectionLocation, 1, GL_FALSE,
                       glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(lightSourceViewLocation, 1, GL_FALSE,
                       glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(lightSourceModelLocation, 1, GL_FALSE,
                       glm::value_ptr(lightSourceModelMatrix));

    glBindVertexArray(lightSourceBuffers.VAO);
    glDrawElements(GL_TRIANGLES, lightSourceMesh.indices.size(),
                   GL_UNSIGNED_INT, 0);

    // Unbind texture and program
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    /* ISSUE RENDER DIRECTIVE */
    SDL_GL_SwapWindow(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
