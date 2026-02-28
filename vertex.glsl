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
