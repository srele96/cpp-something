#include <algorithm>
#include <cstdint>
#include <fstream>
#include <functional>
#include <initializer_list>
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

template <typename> constexpr bool always_false_v{false};

// Catch unsupported types at compile time
template <typename T> struct GLIndexTraits {
  static_assert(always_false_v<T>,
                "Invalid index trait mapping type received!");
  static constexpr GLenum type{0}; // Satisfy the autocomplete
};
template <> struct GLIndexTraits<uint32_t> {
  static constexpr GLenum type{GL_UNSIGNED_INT};
};

// Catch unsupported types at compile time
template <typename T> struct GLVertexTraits {
  static_assert(always_false_v<T>,
                "Invalid Vertex trait mapping type received.");
};
template <> struct GLVertexTraits<float> {
  static constexpr GLenum type{GL_FLOAT};
};
// TODO: If we need to support GL_INT, we need to extend VertexLayout.

struct VertexAttribute {
  GLuint index;
  GLint size;
  GLenum type;
  GLboolean normalized;
  GLsizei offset;
};

class VertexLayout {
private:
  std::vector<VertexAttribute> m_attributes;
  GLsizei m_stride{0};
  GLuint m_currentIndex{0};

public:
  template <typename T>
  void push(const GLint size, GLboolean normalized = GL_FALSE) {
    m_attributes.push_back({
        .index = m_currentIndex,
        .size = size,
        .type = GLVertexTraits<T>::type,
        .normalized = normalized,
        // Why do we assign offset per each to be m_stride
        .offset = m_stride,
    });

    m_stride += size * sizeof(T);
    ++m_currentIndex;
  }

  const std::vector<VertexAttribute> &attributes() const {
    return m_attributes;
  }

  GLsizei stride() const { return m_stride; }
};

class Mesh {
private:
  // TODO: Create VAO class
  // https://gamedev.stackexchange.com/questions/204015/how-to-abstract-vao-as-a-class-in-c
  //
  // TODO: Create VBO class
  // https://gamedev.stackexchange.com/questions/204015/how-to-abstract-vao-as-a-class-in-c

  GLuint m_vertexArrayObjectId{0};
  GLuint m_vertexBufferObjectId{0};
  GLuint m_elementBufferObjectId{0};

  GLenum m_indexType{0};
  GLsizei m_indicesCount{0};

  void cleanup() {
    if (m_vertexArrayObjectId != 0) {
      glDeleteVertexArrays(1, &m_vertexArrayObjectId);
    }
    if (m_vertexBufferObjectId != 0) {
      glDeleteBuffers(1, &m_vertexBufferObjectId);
    }
    if (m_elementBufferObjectId != 0) {
      glDeleteBuffers(1, &m_elementBufferObjectId);
    }
  }

public:
  // TODO: Default constructor, no parameters
  // TODO: Add getter/setter for vertices & indices.
  template <typename VertexType, typename IndexType>
  Mesh(const std::vector<VertexType> &vertices,
       const std::vector<IndexType> &indices, const VertexLayout &layout) {
    if constexpr (!std::is_fundamental_v<VertexType>) {
      assert(layout.stride() == sizeof(VertexType) &&
             "VertexType has padded data. Layout stride does not match C++ "
             "struct size.");
    }

    // GLTypeTraits limits IndexType to fundamental type and sizeof() won't
    // return padded data
    m_indexType = GLIndexTraits<IndexType>::type;
    m_indicesCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &m_vertexArrayObjectId);
    glGenBuffers(1, &m_vertexBufferObjectId);
    glGenBuffers(1, &m_elementBufferObjectId);

    glBindVertexArray(m_vertexArrayObjectId);

    // TODO: What if sizeof() gives compiler-padded size?
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObjectId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexType),
                 vertices.data(), GL_STATIC_DRAW);

    // TODO: What if sizeof() gives compiler-padded size?
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBufferObjectId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(IndexType),
                 indices.data(), GL_STATIC_DRAW);

    for (const auto &attribute : layout.attributes()) {
      glEnableVertexAttribArray(attribute.index);
      glVertexAttribPointer(                               //
          attribute.index,                                 //
          attribute.size,                                  //
          attribute.type,                                  //
          attribute.normalized,                            //
          layout.stride(),                                 //
          reinterpret_cast<const void *>(attribute.offset) //
      );
    }

    // Unbind VAO first to protect its state
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  Mesh(const Mesh &) = delete;

  Mesh &operator=(const Mesh &) = delete;

  Mesh(Mesh &&other) noexcept
      : m_vertexArrayObjectId{std::exchange(other.m_vertexArrayObjectId, 0)},
        m_vertexBufferObjectId{std::exchange(other.m_vertexBufferObjectId, 0)},
        m_elementBufferObjectId{
            std::exchange(other.m_elementBufferObjectId, 0)},
        m_indexType{std::exchange(other.m_indexType, 0)},
        m_indicesCount{std::exchange(other.m_indicesCount, 0)} {}

  Mesh &operator=(Mesh &&other) noexcept {
    if (this != &other) {
      cleanup();

      m_vertexArrayObjectId = std::exchange(other.m_vertexArrayObjectId, 0);
      m_vertexBufferObjectId = std::exchange(other.m_vertexBufferObjectId, 0);
      m_elementBufferObjectId = std::exchange(other.m_elementBufferObjectId, 0);
      m_indexType = std::exchange(other.m_indexType, 0);
      m_indicesCount = std::exchange(other.m_indicesCount, 0);
    }
    return *this;
  }

  ~Mesh() { cleanup(); }

  GLenum indexType() const { return m_indexType; }

  GLsizei indicesCount() const { return m_indicesCount; }

  void bind() const { glBindVertexArray(m_vertexArrayObjectId); }

  void unbind() const { glBindVertexArray(0); }
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
};

Mesh generateCube(float side) {
  float h = side / 2.0f;

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

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  for (int i = 0; i < faces.size(); i++) {
    const auto &f = faces[i];
    glm::vec3 center = f.normal * h;

    // Add 4 edge vertices
    vertices.push_back({.pos = center - (f.right * h) - (f.up * h),
                        .color = f.color,
                        .normal = f.normal}); // BottomLeft
    vertices.push_back({.pos = center + (f.right * h) - (f.up * h),
                        .color = f.color,
                        .normal = f.normal}); // TopLeft
    vertices.push_back({.pos = center + (f.right * h) + (f.up * h),
                        .color = f.color,
                        .normal = f.normal}); // TopRight
    vertices.push_back({.pos = center - (f.right * h) + (f.up * h),
                        .color = f.color,
                        .normal = f.normal}); // BottomRight

    // Add face indices
    unsigned int offset = i * 4;

    // Does the order of indices matter?
    indices.push_back(offset + 0);
    indices.push_back(offset + 1);
    indices.push_back(offset + 2);
    // What if we have more or less indices?
    indices.push_back(offset + 2);
    indices.push_back(offset + 3);
    indices.push_back(offset + 0);
    // How many triangles can we render from 4 vertices?
  }

  VertexLayout layout;
  layout.push<float>(3);
  layout.push<float>(3);
  layout.push<float>(3);

  Mesh mesh{vertices, indices, layout};

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

// TODO: Figure out if it's beneficial to refactor this function to use
// generateMesh
Mesh generateSphere(const int latitudeBands, const int longitudeBands,
                    const float r = 1,
                    const glm::vec3 &color = {1.0f, 1.0f, 1.0f}) {
  std::vector<Vertex> vertices;

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

      vertices.push_back({
          .pos = r * normal,
          .color = color,
          .normal = normal,
      });
    }
  }

  std::vector<unsigned int> indices;

  for (int i{0}; i < latitudeBands; ++i) {
    for (int j{0}; j < longitudeBands; ++j) {
      const int rowStride{longitudeBands + 1};

      const int topLeft{i * rowStride + j};
      const int topRight{topLeft + 1};
      const int bottomLeft{(i + 1) * rowStride + j};
      const int bottomRight{bottomLeft + 1};

      indices.push_back(topLeft);
      indices.push_back(topRight);
      indices.push_back(bottomLeft);

      indices.push_back(topRight);
      indices.push_back(bottomRight);
      indices.push_back(bottomLeft);
    }
  }

  VertexLayout layout;

  layout.push<float>(3);
  layout.push<float>(3);
  layout.push<float>(3);

  Mesh mesh{vertices, indices, layout};

  return mesh;
}

Mesh generateQuad(const float edge = 0.5f) {
  auto createVertex{[](const glm::vec3 &pos) -> Vertex {
    const glm::vec3 color{1.0f, 1.0f, 1.0f};
    const glm::vec3 normal{0.0f, 1.0f, 0.0f};

    return {.pos = pos, .color = color, .normal = normal};
  }};

  std::vector<Vertex> vertices{
      createVertex({-edge, 0, -edge}), //
      createVertex({edge, 0, -edge}),  //
      createVertex({edge, 0, edge}),   //
      createVertex({-edge, 0, edge}),  //
  };

  std::vector<unsigned int> indices{0, 1, 2, 2, 3, 0};

  VertexLayout layout;

  // 3 floats for position, color, normal
  layout.push<float>(3);
  layout.push<float>(3);
  layout.push<float>(3);

  Mesh mesh{vertices, indices, layout};

  return mesh;
}

using UVMapCallback = std::function<void(const float, const float)>;

void generateUVMap(const int uSteps, const int vSteps,
                   const UVMapCallback &callback) {
  for (int i{0}; i < uSteps; ++i) {
    for (int j{0}; j < vSteps; ++j) {
      const float u{static_cast<float>(i) / static_cast<float>(uSteps - 1)};
      const float v{static_cast<float>(j) / static_cast<float>(vSteps - 1)};

      callback(u, v);
    }
  }
}

struct UVIndexParam {
  uint32_t topLeft;
  uint32_t topRight;
  uint32_t bottomLeft;
  uint32_t bottomRight;
};

using UVMapIndexCallback = std::function<void(UVIndexParam)>;

// NOTE: To wrap the grid, generate 1 more row or column (whichever expects
// wrapping)
void generateUVMapIndices(const int uSteps, const int vSteps,
                          const UVMapIndexCallback &callback) {
  for (int i{0}; i < uSteps - 1; ++i) {
    for (int j{0}; j < vSteps - 1; ++j) {
      const int nextI{(i + 1)};
      const int nextJ{(j + 1)};

      const uint32_t topLeft{static_cast<uint32_t>(i * vSteps + j)};
      const uint32_t topRight{static_cast<uint32_t>(i * vSteps + nextJ)};
      const uint32_t bottomLeft{static_cast<uint32_t>(nextI * vSteps + j)};
      const uint32_t bottomRight{static_cast<uint32_t>(nextI * vSteps + nextJ)};

      callback({
          .topLeft = topLeft,
          .topRight = topRight,
          .bottomLeft = bottomLeft,
          .bottomRight = bottomRight,
      });
    }
  }
}

Vertex generateCylinderVertex(const float u, const float v) {
  std::vector<Vertex> vertices;
  const float angle{u * 2.0f * glm::pi<float>()};
  const float z{v - 0.5f};

  glm::vec3 position{glm::cos(angle), glm::sin(angle), z};
  const glm::vec3 center{0.0f, 0.0f, z};
  glm::vec3 normal{glm::normalize(position - center)};
  glm::vec3 color{1.0f, 1.0f, 1.0f};

  return {
      .pos = std::move(position),
      .color = std::move(color),
      .normal = std::move(normal),
  };
}

// TODO: Scaling cylinder length reduces curvature. Allow curvature to be
// size-relative so stretching does not stretch out the curve.
// TODO: Allow larger than unit length cylinders.
Vertex generateWavyCylinderVertex(const float u, const float v) {
  std::vector<Vertex> vertices;
  const float angle{u * 2.0f * glm::pi<float>()};
  const float z{v - 0.5f};

  glm::vec3 position{
      glm::cos(angle),                                         //
      glm::sin(z * 2.0f * glm::pi<float>()) + glm::sin(angle), //
      z                                                        //
  };
  // Analytical Partial Derivative of the position parametric function
  glm::vec3 d_angle{
      -glm::sin(angle), //
      glm::cos(angle),  //
      0.0               //
  };
  glm::vec3 d_z{
      0.0f,                                                            //
      2.0f * glm::pi<float>() * glm::cos(z * 2.0f * glm::pi<float>()), //
      1.0f                                                             //
  };
  glm::vec3 normal{glm::normalize(glm::cross(d_angle, d_z))};
  glm::vec3 color{1.0f, 1.0f, 1.0f};

  return {
      .pos = std::move(position),
      .color = std::move(color),
      .normal = std::move(normal),
  };
}

Vertex generateTorusVertex(const float u, const float v) {
  const float theta{u * 2.0f * glm::pi<float>()};
  const float phi{v * 2.0f * glm::pi<float>()};
  const float z{v - 0.5f};

  // TODO: Configurable parameters, make radius configurable.
  const float ringRadius{1.0f};
  const float torusRadius{4.0f};

  glm::vec3 position{ringRadius * glm::cos(theta), ringRadius * glm::sin(theta),
                     0.0f};
  glm::mat4 model{glm::identity<glm::mat4>()};
  model = glm::rotate(model, phi, glm::vec3{0.0f, 1.0f, 0.0f});
  model = glm::translate(model, glm::vec3{torusRadius, 0.0f, 0.0f});
  position = glm::vec3{model * glm::vec4{position, 1.0f}};
  const glm::vec3 center{model * glm::vec4{0.0f, 0.0f, 0.0, 1.0f}};
  glm::vec3 normal{glm::normalize(position - center)};

  glm::vec3 color{1.0f, 1.0f, 1.0f};

  return {.pos = std::move(position),
          .color = std::move(color),
          .normal = std::move(normal)};
}

// TODO: Add disc generator

using GenerateMeshCallback = std::function<Vertex(const float, const float)>;

// TODO: Consider Finite Difference method in case of more geometry
Mesh generateMesh(const GenerateMeshCallback &callback, const int uSteps = 32,
                  const int vSteps = 32) {
  std::vector<Vertex> vertices;

  const int uCount{uSteps + 1};

  generateUVMap(uCount, vSteps,
                [&vertices, &callback](const float u, const float v) {
                  vertices.push_back(callback(u, v));
                });

  std::vector<uint32_t> indices;

  generateUVMapIndices(uCount, vSteps, [&indices](const auto idxParam) {
    indices.push_back(idxParam.topLeft);
    indices.push_back(idxParam.topRight);
    indices.push_back(idxParam.bottomLeft);
    indices.push_back(idxParam.bottomLeft);
    indices.push_back(idxParam.topRight);
    indices.push_back(idxParam.bottomRight);
  });

  VertexLayout layout;

  layout.push<float>(3);
  layout.push<float>(3);
  layout.push<float>(3);

  return {std::move(vertices), std::move(indices), std::move(layout)};
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

enum class ShaderType : GLenum {
  Vertex = GL_VERTEX_SHADER,
  Fragment = GL_FRAGMENT_SHADER,
  Geometry = GL_GEOMETRY_SHADER
};

GLenum shaderTypeToGLenum(ShaderType shaderType) {
  if (shaderType == ShaderType::Vertex) {
    return GL_VERTEX_SHADER;
  }
  if (shaderType == ShaderType::Fragment) {
    return GL_FRAGMENT_SHADER;
  }
  if (shaderType == ShaderType::Geometry) {
    return GL_GEOMETRY_SHADER;
  }

  throw std::runtime_error("Received unsupported shader type.");
}

class AbstractShader {
public:
  virtual ~AbstractShader() = default;
  virtual AbstractShader &replace(const std::string &, const std::string &) = 0;
  virtual std::string src() const = 0;
  virtual ShaderType type() const = 0;
  virtual AbstractShader &
  insertDefines(const std::vector<std::string> &defines) = 0;
  virtual AbstractShader &clearDefines() = 0;
};

template <ShaderType Type> //
class Shader final : public AbstractShader {
private:
  std::string m_src;

  struct DefinesRange {
    size_t begin{0};
    size_t length{0};
    bool valid{false};
  };

  DefinesRange getDefinesRange() const {
    const std::string definesBegin{"/*{{defines_begin}}*/"};
    const std::string definesEnd{"/*{{defines_end}}*/"};

    size_t posBegin{m_src.find(definesBegin)};
    size_t posEnd{m_src.find(definesEnd)};

    if (posBegin != std::string::npos && posEnd != std::string::npos) {
      size_t contentStart{posBegin + definesBegin.length()};
      size_t contentLength{posEnd - contentStart};
      const DefinesRange definesRange{
          .begin = contentStart, .length = contentLength, .valid = true};
      return definesRange;
    }

    return DefinesRange{.valid = false};
  }

public:
  Shader(std::string src,
         const std::unordered_map<std::string, std::string> &replacements = {},
         const std::vector<std::string> &defines = {})
      : m_src{std::move(src)} {
    for (const auto &[key, value] : replacements) {
      this->replace(key, value);
    }
    insertDefines(defines);
  }

  Shader &insertDefines(const std::vector<std::string> &defines = {}) override {
    // TODO: Maybe prevent inserting multiple defines.
    //
    // Currently allows inserting multiple defines. It makes adding method to
    // define a single define and remove it more difficult, as we have to
    // currently treat every define we want to remove, as if there are multiple
    // #defines of it.

    if (defines.empty()) {
      return *this;
    }

    const DefinesRange range{getDefinesRange()};
    if (!range.valid) {
      return *this;
    }

    std::string strDefines{"\n"};

    for (const auto &value : defines) {
      strDefines += "#define " + value + "\n";
    }

    m_src.insert(range.begin, strDefines);

    return *this;
  }

  Shader &clearDefines() override {
    const DefinesRange range{getDefinesRange()};
    if (range.valid) {
      m_src.replace(range.begin, range.length, "\n");
    }
    return *this;
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

  std::string src() const override { return m_src; }

  ShaderType type() const override { return Type; }
};

class ShaderProgram {
private:
  GLuint m_id{0};
  mutable std::unordered_map<std::string, GLint> m_uniformCache;

  static GLuint compileShader(const AbstractShader &shader) {
    GLuint shaderId{glCreateShader(shaderTypeToGLenum(shader.type()))};

    const std::string src{shader.src()};
    const char *asd{src.c_str()};

    glShaderSource(shaderId, 1, &asd, nullptr);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

    if (!success) {
      char infoLog[512];
      glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
      std::cerr << "Shader compile error:\n" << infoLog << std::endl;
    }

    return shaderId;
  }

  static void logLinkStatus(const GLuint shaderProgram,
                            const std::string &prefix = "") {
    GLint success;

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
      char infoLog[512];
      glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
      std::cerr << prefix << "Program link error:\n" << infoLog << std::endl;
    }
    std::cout << prefix << "Program link success.\n";
  }

  void finalizeProgram(
      const std::initializer_list<std::reference_wrapper<const AbstractShader>>
          shaders) {
    m_id = glCreateProgram();

    std::vector<GLuint> compiledIds;

    for (const AbstractShader &shader : shaders) {
      GLuint id{compileShader(shader)};
      glAttachShader(m_id, id);
      compiledIds.push_back(id);
    }

    glLinkProgram(m_id);
    logLinkStatus(m_id);

    for (const GLuint id : compiledIds) {
      // https://gamedev.stackexchange.com/questions/47910/after-a-succesful-gllinkprogram-should-i-delete-detach-my-shaders
      // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDetachShader.xhtml
      glDetachShader(m_id, id);
      glDeleteShader(id);
    }
  }

  GLint getUniformLocation(const std::string &name) const {
    if (auto it{m_uniformCache.find(name)}; it != m_uniformCache.end()) {
      return it->second;
    }

    GLint location{glGetUniformLocation(m_id, name.c_str())};

    m_uniformCache[name] = location;

    if (location == -1) {
      std::cerr << "Warning: Uniform '" << name
                << "' does not exist or was optimized out." << std::endl;
    }

    return location;
  }

public:
  ShaderProgram(const Shader<ShaderType::Vertex> &vertexShader) {
    finalizeProgram({vertexShader});
  }

  ShaderProgram(const Shader<ShaderType::Vertex> &vertexShader,
                const Shader<ShaderType::Fragment> &fragmentShader) {

    finalizeProgram({vertexShader, fragmentShader});
  }

  ShaderProgram(const Shader<ShaderType::Vertex> &vertexShader,
                const Shader<ShaderType::Geometry> &geometryShader,
                const Shader<ShaderType::Fragment> &fragmentShader) {
    finalizeProgram({vertexShader, geometryShader, fragmentShader});
  }

  ShaderProgram(const ShaderProgram &) = delete;

  ShaderProgram(ShaderProgram &&other) noexcept : m_id{other.m_id} {
    other.m_id = 0;
  }

  ~ShaderProgram() { glDeleteProgram(m_id); }

  ShaderProgram &operator=(const ShaderProgram &) = delete;

  ShaderProgram &operator=(ShaderProgram &&other) noexcept {
    if (this != &other) {
      if (m_id != 0) {
        glDeleteProgram(m_id);
      }
      m_id = other.m_id;
      other.m_id = 0;
      return *this;
    }
    return *this;
  }

  void use() const { glUseProgram(m_id); }

  void unuse() const { glUseProgram(0); }

  void setUniform(const std::string &name, int value) const {
    glUniform1i(getUniformLocation(name), value);
  }

  void setUniform(const std::string &name, const glm::vec3 &value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
  }

  void setUniform(const std::string &name, const glm::mat4 &value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE,
                       glm::value_ptr(value));
  }
};

namespace ShaderSource {

// Source for `glsl` in c++ raw string literals: https://open.gl/geometry

const std::string computeShadow{R"glsl(
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
)glsl"};

const std::string computeColor{R"glsl(
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
)glsl"};

Shader<ShaderType::Vertex> vertexShader{
    R"glsl(
#version 330 core

/*{{defines_begin}}*/
/*{{defines_end}}*/

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;
out vec4 vFragPosLightSpace;

#ifdef HAS_GEOMETRY_SHADER
// Does it matter if we output this and we don't have geometry shader in the next stage?
// Do we have to match VS_OUT and vs_out to be exactly same with different casing?
out VS_OUT {
  vec3 worldPos;
  vec3 vNormal;
  vec3 vColor;
} vs_out;
#endif // HAS_GEOMETRY_SHADER

uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_lightProjection;
uniform mat4 u_lightView;

// TODO: Normal, Tangent, Bitangent (Allow us to apply Finite Difference method on any position)
// We can not do vertex displacement without Tangent, Bitangent, Normal data (to compute new normal)

void main() {
  vec4 worldPosition = u_model * vec4(aPos, 1.0);

  gl_Position = u_projection * u_view * worldPosition;

  // To make the lighting color math to work, the computation must happen in the same space. We must not mix spaces.
  vFragPos = vec3(worldPosition);

  vColor = aColor;

  // Rotate and move normal with the vertex, but prevent scaling from messing
  // up the normals perpendicularity.
  vNormal = mat3(transpose(inverse(u_model))) * aNormal;
  vFragPosLightSpace = u_lightProjection * u_lightView * worldPosition;

#ifdef HAS_GEOMETRY_SHADER
  vs_out.worldPos = vec3(worldPosition);
  // Keep the outputs to keep the compatibility with fragment shader.
  // Is it fine to access vNormal like this? Since it's marked as 'out'?
  vs_out.vNormal = vNormal;
  vs_out.vColor = vColor;
#endif // HAS_GEOMETRY_SHADER
}
)glsl"};

const Shader<ShaderType::Geometry> geometryShader{
    R"glsl(
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
)glsl"};

const Shader<ShaderType::Fragment> basicFragmentShader{
    R"glsl(
#version 330 core

out vec4 FragColor;

in vec3 vColor;

void main() {
  FragColor = vec4(vColor, 1.0);
}
)glsl"};

Shader<ShaderType::Fragment> fragmentShader{
    R"glsl(
#version 330 core

/*{{defines_begin}}*/
/*{{defines_end}}*/

in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;
in vec4 vFragPosLightSpace;

out vec4 FragColor;

uniform vec3 u_eyePosition;
uniform vec3 u_lightDirection;
uniform sampler2DShadow u_shadowMap;

{{computeShadow}}

{{computeColor}}

#ifdef COMPUTE_CHECKER
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

vec3 getColor(vec3 fragPos, vec3 color) {
  return computeChecker(fragPos);
}
#else
vec3 getColor(vec3 fragPos, vec3 color) {
  return color;
}
#endif // COMPUTE_CHECKER

void main() {
  vec3 normal = normalize(vNormal);

  vec3 lightColor = vec3(1.0, 0.98, 0.9);

  // From fragment to the directional light source
  vec3 lightDirection = -u_lightDirection;
  vec3 lightReflection = reflect(-lightDirection, normal);
  vec3 eyeDirection = normalize(u_eyePosition - vFragPos);

  LightingComponent lightComponent;
  lightComponent.ambient = computeAmbient(lightColor);
  lightComponent.diffuse = computeDiffuse(lightColor, lightDirection, normal);
  lightComponent.specular = computeSpecular(lightColor, eyeDirection, lightReflection);

  float shadow = computeShadow(vFragPosLightSpace, normal, lightDirection, u_shadowMap);

  vec3 baseColor = getColor(vFragPos, vColor);

  vec3 fragmentColor = computeFragColor(lightComponent, baseColor, shadow);

  FragColor = vec4(fragmentColor, 1.0);
}
)glsl",
    {{"computeColor", computeColor}, //
     {"computeShadow", computeShadow}}};

const Shader<ShaderType::Vertex> postProcessingVert{R"glsl(
#version 330

layout(location = 0) in vec3 aPos;

out vec2 TexCoords;

void main() {
  gl_Position = vec4(aPos.x, aPos.z, 0.0, 1.0);
  TexCoords = vec2(aPos.x, aPos.z) * 0.5 + 0.5;
}
)glsl"};

const Shader<ShaderType::Fragment> postProcessingFrag{R"glsl(
#version 330

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_screenTexture;

void main() {
  vec3 color = texture(u_screenTexture, TexCoords).rgb;

  // Fade the pixels the further we are from the center of the screen
  float dist = distance(TexCoords, vec2(0.5, 0.5));
  float vignette = smoothstep(1.0, 0.2, dist);

  color *= vignette;

  FragColor = vec4(color, 1.0);
}
)glsl"};

} // namespace ShaderSource

// TODO: Render a plane and use calculus to make it more interesting. Procedural
// terrain generation?

// TODO: Render a pipe in a sinus & cosinus shape. Remove the dead code which
// used to render sinus wave.

namespace ShadowMapping {

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

class Camera {
private:
  // TODO: Add a way to keep multiple cameras. One which has ability to
  // rotated freely, and another one with constrained pitch.
  // TODO: Consider adding camera not capable of flying around.

  glm::vec3 m_forward;
  glm::vec3 m_right;
  glm::vec3 m_up;

  glm::vec3 m_eye;

public:
  Camera()
      : m_forward{glm::vec3{0.0f, 0.0f, -1.0f}}, //
        m_right{glm::vec3{1.0f, 0.0f, 0.0f}},    //
        m_up{glm::vec3{0.0f, 1.0f, 0.0f}},       //
        m_eye{glm::vec3(0.0f, 0.0f, 0.0f)}       //

  {}

  Camera(glm::vec3 eye)
      : m_forward{glm::vec3{0.0f, 0.0f, -1.0f}}, //
        m_right{glm::vec3{1.0f, 0.0f, 0.0f}},    //
        m_up{glm::vec3{0.0f, 1.0f, 0.0f}},       //
        m_eye{std::move(eye)}                    //
  {}

  Camera(glm::vec3 forward, glm::vec3 right, glm::vec3 up, glm::vec3 eye)
      : m_forward{std::move(forward)}, //
        m_right{std::move(right)},     //
        m_up{std::move(up)},           //
        m_eye{std::move(eye)}          //
  {}

  void update(const float pitch, const float yaw, const glm::vec3 &worldUp) {
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

    forward = glm::rotate(forward, pitch, right);
    // TODO: Figure out if i should keep -yaw here, or should this method
    // receive -yaw
    forward = glm::rotate(forward, yaw, worldUp);

    m_forward = glm::normalize(forward);
    m_right = glm::normalize(glm::cross(m_forward, worldUp));
    m_up = glm::cross(m_right, m_forward);
  }

  void up(const float speed) { m_eye += m_up * speed; }
  void down(const float speed) { m_eye -= m_up * speed; }
  // TODO: BUG - Sometimes camera moving sideways becomes slower than
  // forward/backward movement
  void right(const float speed) { m_eye += m_right * speed; }
  void left(const float speed) { m_eye -= m_right * speed; }
  void forward(const float speed) { m_eye += m_forward * speed; }
  void backward(const float speed) { m_eye -= m_forward * speed; }

  const glm::vec3 &eye() const { return m_eye; }

  glm::mat4 viewMatrix(const glm::vec3 &worldUp) const {
    return glm::lookAt(
        m_eye,
        // Cancel out the eye vector to form a free look at matrix
        m_eye + m_forward, worldUp);
  }
};

// TODO: Generate distortion over a plane. This is where we can practically
// apply calculus.

// TODO: Generate cylider, sinusoidal and cosinusoidal cylidern.

// TODO: Add one point light illumination.

// TODO: Add point light shadow mapping.

// TODO: Add deffered shading after one point light illumination.

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

  /////////////////////////////////////////////////////////////////////////////

  /* SHADER PROGRAM */

  ShaderProgram shaderProgram{ShaderSource::vertexShader,
                              ShaderSource::fragmentShader};

  // ----

  ShaderSource::vertexShader.insertDefines({"HAS_GEOMETRY_SHADER"});
  ShaderProgram debugShaderProgram{
      ShaderSource::vertexShader,       //
      ShaderSource::geometryShader,     //
      ShaderSource::basicFragmentShader //
  };
  ShaderSource::vertexShader.clearDefines();

  // ----

  ShaderProgram lightSourceProgram{
      ShaderSource::vertexShader,       //
      ShaderSource::basicFragmentShader //
  };

  // ----

  ShaderSource::fragmentShader.insertDefines({"COMPUTE_CHECKER"});
  ShaderProgram floorProgram{
      ShaderSource::vertexShader,  //
      ShaderSource::fragmentShader //
  };
  ShaderSource::fragmentShader.clearDefines();

  // ----

  ShaderProgram depthProgram{ShaderSource::vertexShader};

  // ----

  ShaderProgram postProcessingProgram{ShaderSource::postProcessingVert,
                                      ShaderSource::postProcessingFrag};

  /////////////////////////////////////////////////////////////////////////////

  /* MESH */

  Mesh cube{generateCube(4.0f)};

  glm::mat4 cubemodelMatrix{1.0f};
  // TRS rule of thumb
  // Position = M_Translate * M_Rotate * M_Scale * V_Position
  cubemodelMatrix =
      glm::translate(cubemodelMatrix, glm::vec3(0.0f, 4.0f, -10.0f));
  cubemodelMatrix = glm::rotate(cubemodelMatrix, glm::pi<float>() / 6,
                                glm::vec3(1.0f, 0.0f, 0.0f));

  // ----

  Mesh sphere{generateSphere(30, 30, 8.0f)};

  glm::mat4 sphereModelMatrix{glm::translate(glm::identity<glm::mat4>(),
                                             glm::vec3(0.0f, 8.0f, -25.0f))};

  // ----

  Mesh floor{generateQuad()};

  glm::mat4 floorModelMatrix{glm::identity<glm::mat4>()};
  // TODO: I wonder why is 100 units not enough to cover the whole frustum? The
  // far variable is 100 units, so... Update: The front of the floor is half of
  // the full length of the frustum.
  floorModelMatrix =
      glm::scale(floorModelMatrix, glm::vec3(200.0f, 1.0f, 200.0f));

  // ----

  Mesh lightSource{
      generateSphere(20.0f, 20.0f, 1.0f, glm::vec3{1.0f, 1.0f, 0.0f})};

  // TODO: Render point light & spotlight.
  // TODO: Make light source movable.
  glm::mat4 lightSourceModelMatrix{glm::identity<glm::mat4>()};
  lightSourceModelMatrix =
      glm::translate(lightSourceModelMatrix, glm::vec3{-10.0f, 10.0f, -10.0f});

  // ----

  Mesh cylinder{generateMesh(generateCylinderVertex)};

  glm::mat4 cylinderModelMatrix{glm::identity<glm::mat4>()};
  cylinderModelMatrix =
      glm::translate(cylinderModelMatrix, glm::vec3{10.0f, 5.0f, -10.0f});
  cylinderModelMatrix =
      glm::scale(cylinderModelMatrix, glm::vec3{1.0f, 1.0f, 4.0f});

  // ----

  Mesh wavyCylinder{generateMesh(generateWavyCylinderVertex)};
  glm::mat4 wavyCylinderModelMatrix{glm::identity<glm::mat4>()};
  wavyCylinderModelMatrix =
      glm::translate(wavyCylinderModelMatrix, glm::vec3{20.0f, 3.0f, -15.0f});
  wavyCylinderModelMatrix =
      glm::rotate(wavyCylinderModelMatrix, glm::pi<float>() / 2.0f,
                  glm::vec3{0.0f, 1.0f, 0.0f});
  wavyCylinderModelMatrix =
      glm::scale(wavyCylinderModelMatrix, glm::vec3{1.0f, 1.0f, 8.0f});

  // ----

  Mesh torus{generateMesh(generateTorusVertex)};
  glm::mat4 torusModelMatrix{glm::identity<glm::mat4>()};
  torusModelMatrix =
      glm::translate(torusModelMatrix, glm::vec3{20.0f, 8.0f, -25.0f});

  // ----

  Mesh postProcessingQuad{generateQuad(1.0f)};

  /////////////////////////////////////////////////////////////////////////////

  struct DepthMap {
    GLuint framebuffer;
    GLuint texture;
    const unsigned int TEXTURE_WIDTH{2048};
    const unsigned int TEXTURE_HEIGHT{2048};
    const std::array<float, 4> borderColor{1.0f, 1.0f, 1.0f, 1.0f};

    void setTextureSize(const float width, const float height) const noexcept {
      glBindTexture(GL_TEXTURE_2D, texture);
      // Store depth component in the texture, and tell graphics driver to give
      // us higher precision shadows
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0,
                   GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  };

  DepthMap depthMap;

  // Texture
  // https://wikis.khronos.org/opengl/Texture
  glGenTextures(1, &depthMap.texture);
  glBindTexture(GL_TEXTURE_2D, depthMap.texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                   depthMap.borderColor.data());
  glBindTexture(GL_TEXTURE_2D, 0);

  depthMap.setTextureSize(depthMap.TEXTURE_WIDTH, depthMap.TEXTURE_HEIGHT);

  // Framebuffer
  glGenFramebuffers(1, &depthMap.framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, depthMap.framebuffer);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthMap.texture, 0);

  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Depth Framebuffer IS complete." << std::endl;
  } else {
    std::cerr << "Depth Framebuffer is NOT complete!" << std::endl;
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
      GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
    std::cerr << "Depth Framebuffer incomplete attachment\n";
  } else {
    std::cout << "Depth Framebuffer complete attachment\n";
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  /////////////////////////////////////////////////////////////////////////////

  struct PostProcessBuffer {
    GLuint framebufferId;
    GLuint textureId;
    GLuint renderbufferId;

    void setTextureSize(const float width, const float height) const noexcept {
      glBindTexture(GL_TEXTURE_2D, textureId);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, NULL);
      glBindTexture(GL_TEXTURE_2D, 0);
    }

    void setRenderbufferSize(const float width,
                             const float height) const noexcept {
      glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width,
                            height);
      glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
  };

  PostProcessBuffer postProcessBuffer;

  // Texture
  glGenTextures(1, &postProcessBuffer.textureId);
  glBindTexture(GL_TEXTURE_2D, postProcessBuffer.textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  postProcessBuffer.setTextureSize(window_width, window_height);

  // Renderbuffer
  glGenRenderbuffers(1, &postProcessBuffer.renderbufferId);

  postProcessBuffer.setRenderbufferSize(window_width, window_height);

  // Framebuffer
  glGenFramebuffers(1, &postProcessBuffer.framebufferId);
  glBindFramebuffer(GL_FRAMEBUFFER, postProcessBuffer.framebufferId);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         postProcessBuffer.textureId, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, postProcessBuffer.renderbufferId);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Post-Process Framebuffer IS COMPLETE.\n";
  } else {
    std::cerr << "Post-Process Framebuffer is NOT complete!\n";
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

  const float sensitivity = 0.05f;

  Uint64 lastTime{SDL_GetTicks()};
  float deltaTime{0.0f};

  const float velocity{10.0f};

  glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

  Camera camera{glm::vec3{0.0f, 5.0f, 0.0f}};

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        window_width = event.window.data1;
        window_height = event.window.data2;

        glViewport(0, 0, window_width, window_height);

        postProcessBuffer.setTextureSize(window_width, window_height);
        postProcessBuffer.setRenderbufferSize(window_width, window_height);
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

        camera.update(glm::radians(pitch), -glm::radians(yaw), worldUp);
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
      camera.forward(speed);
    }
    if (keystate[SDL_SCANCODE_S]) {
      camera.backward(speed);
    }
    if (keystate[SDL_SCANCODE_A]) {
      camera.left(speed);
    }
    if (keystate[SDL_SCANCODE_D]) {
      camera.right(speed);
    }
    if (keystate[SDL_SCANCODE_SPACE]) {
      camera.up(speed);
    }
    if (keystate[SDL_SCANCODE_LCTRL]) {
      camera.down(speed);
    }

    glm::mat4 viewMatrix{camera.viewMatrix(worldUp)};

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

    glBindFramebuffer(GL_FRAMEBUFFER, depthMap.framebuffer);

    // TODO: Make shadows be affected by each light source. Currently only
    // directional light affects shadows.
    glViewport(0, 0, depthMap.TEXTURE_WIDTH, depthMap.TEXTURE_HEIGHT);

    glEnable(GL_DEPTH_TEST);
    // Fix shadow acne
    glCullFace(GL_FRONT);

    glClear(GL_DEPTH_BUFFER_BIT);

    depthProgram.use();

    depthProgram.setUniform("u_projection", lightMatrix.projection);
    depthProgram.setUniform("u_view", lightMatrix.view);

    depthProgram.setUniform("u_model", cubemodelMatrix);
    cube.bind();
    glDrawElements(GL_TRIANGLES, cube.indicesCount(), cube.indexType(), 0);

    depthProgram.setUniform("u_model", sphereModelMatrix);
    sphere.bind();
    glDrawElements(GL_TRIANGLES, sphere.indicesCount(), sphere.indexType(), 0);

    depthProgram.setUniform("u_model", cylinderModelMatrix);
    cylinder.bind();
    glDrawElements(GL_TRIANGLES, cylinder.indicesCount(), cylinder.indexType(),
                   0);

    depthProgram.setUniform("u_model", wavyCylinderModelMatrix);
    wavyCylinder.bind();
    glDrawElements(GL_TRIANGLES, wavyCylinder.indicesCount(),
                   wavyCylinder.indexType(), 0);

    depthProgram.setUniform("u_model", torusModelMatrix);
    torus.bind();
    glDrawElements(GL_TRIANGLES, torus.indicesCount(), torus.indexType(), 0);

    // Revert culling to normal one
    glCullFace(GL_BACK);

    /* LIGHT PASS */

    glBindFramebuffer(GL_FRAMEBUFFER, postProcessBuffer.framebufferId);

    glViewport(0, 0, window_width, window_height);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap.texture);

    shaderProgram.use();

    shaderProgram.setUniform("u_projection", projectionMatrix);
    shaderProgram.setUniform("u_view", viewMatrix);
    shaderProgram.setUniform("u_eyePosition", camera.eye());
    shaderProgram.setUniform("u_lightDirection", lightDirection);
    shaderProgram.setUniform("u_lightProjection", lightMatrix.projection);
    shaderProgram.setUniform("u_lightView", lightMatrix.view);
    // Texture unit 0 is reserved for color/diffuse
    shaderProgram.setUniform("u_shadowMap", 1);

    shaderProgram.setUniform("u_model", cubemodelMatrix);

    cube.bind();
    glDrawElements(GL_TRIANGLES, cube.indicesCount(), cube.indexType(), 0);

    shaderProgram.setUniform("u_model", sphereModelMatrix);

    sphere.bind();
    glDrawElements(GL_TRIANGLES, sphere.indicesCount(), sphere.indexType(), 0);

    shaderProgram.setUniform("u_model", cylinderModelMatrix);
    cylinder.bind();
    glDrawElements(GL_TRIANGLES, cylinder.indicesCount(), cylinder.indexType(),
                   0);

    shaderProgram.setUniform("u_model", wavyCylinderModelMatrix);
    wavyCylinder.bind();
    glDrawElements(GL_TRIANGLES, wavyCylinder.indicesCount(),
                   wavyCylinder.indexType(), 0);

    shaderProgram.setUniform("u_model", torusModelMatrix);
    torus.bind();
    glDrawElements(GL_TRIANGLES, torus.indicesCount(), torus.indexType(), 0);

    debugShaderProgram.use();

    debugShaderProgram.setUniform("u_projection", projectionMatrix);
    debugShaderProgram.setUniform("u_view", viewMatrix);
    debugShaderProgram.setUniform("u_model", sphereModelMatrix);
    // TODO: Figure out if we need to pass these uniforms to this shader
    // program: u_lightProjection, u_lightView, u_shadowMap

    sphere.bind();
    glDrawElements(GL_TRIANGLES, sphere.indicesCount(), sphere.indexType(), 0);

    floorProgram.use();

    floorProgram.setUniform("u_projection", projectionMatrix);
    floorProgram.setUniform("u_view", viewMatrix);
    floorProgram.setUniform("u_model", floorModelMatrix);
    floorProgram.setUniform("u_eyePosition", camera.eye());
    floorProgram.setUniform("u_lightDirection", lightDirection);
    floorProgram.setUniform("u_lightProjection", lightMatrix.projection);
    floorProgram.setUniform("u_lightView", lightMatrix.view);
    // Texture unit 0 is reserved for color/diffuse
    floorProgram.setUniform("u_shadowMap", 1);

    floor.bind();
    glDrawElements(GL_TRIANGLES, floor.indicesCount(), floor.indexType(), 0);

    lightSourceProgram.use();

    lightSourceProgram.setUniform("u_projection", projectionMatrix);
    lightSourceProgram.setUniform("u_view", viewMatrix);
    lightSourceProgram.setUniform("u_model", lightSourceModelMatrix);
    // TODO: Figure out if we need to pass these uniforms to this shader
    // program: u_lightProjection, u_lightView, u_shadowMap

    lightSource.bind();
    glDrawElements(GL_TRIANGLES, lightSource.indicesCount(),
                   lightSource.indexType(), 0);

    // Unbind texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* POST-PROCESSING PASS */

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearBufferfv(GL_COLOR, 0,
                    glm::value_ptr(glm::vec4{0.1f, 0.1f, 0.15f, 1.0f}));
    glDisable(GL_DEPTH_TEST);

    postProcessingProgram.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, postProcessBuffer.textureId);
    postProcessingProgram.setUniform("u_screenTexture", 0);

    postProcessingQuad.bind();
    glDrawElements(GL_TRIANGLES, postProcessingQuad.indicesCount(),
                   postProcessingQuad.indexType(), 0);

    /* ISSUE RENDER DIRECTIVE */
    SDL_GL_SwapWindow(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
