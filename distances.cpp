#include "glm/geometric.hpp"
#include "glm/vec3.hpp" // IWYU pragma: keep
#include <iostream>
#include <limits>

// The purpose of this function is to be generic enough, while matching the
// mathematix from the latex file.
glm::vec3 line(const float k, const glm::vec3 &p, const glm::vec3 &dir) {
  return p + k * dir;
}

struct Segment {
  glm::vec3 p0;
  glm::vec3 p1;
};

void shortest(const Segment &segA, const Segment &segB) {
  const auto u{segA.p1 - segA.p0};
  const auto v{segB.p1 - segB.p0};
  const auto w0{segA.p0 - segB.p0};
  const auto a{glm::dot(u, u)};
  const auto b{glm::dot(u, v)};
  const auto c{glm::dot(v, v)};
  const auto d{glm::dot(u, w0)};
  const auto e{glm::dot(v, w0)};

  const auto D{b * b - a * c};
  const auto Dt{d * c - e * b};
  const auto Ds{b * d - a * e};
  const auto tParam{Dt / D};
  const auto sParam{Ds / D};

  const auto s0{glm::abs(D) > 0.00001f
                    ? glm::clamp((b * d - a * e) / D, 0.0f, 1.0f)
                    : 0.0f};
  const auto t1{glm::clamp((s0 * b - d) / a, 0.0f, 1.0f)};
  const auto s1{glm::clamp((t1 * b + e) / c, 0.0f, 1.0f)};

  const auto P{line(t1, segA.p0, u)};
  const auto Q{line(s1, segB.p0, v)};

  const float dist{glm::length(P - Q)};

  std::cout << "dist: " << dist << "\n";
  std::cout << "P: " << P.x << ", " << P.y << ", " << P.z << "\n";
  std::cout << "Q: " << Q.x << ", " << Q.y << ", " << Q.z << "\n";
}

void shortestClaude(const Segment &segA, const Segment &segB) {
  const auto u{segA.p1 - segA.p0};
  const auto v{segB.p1 - segB.p0};
  const auto w0{segA.p0 - segB.p0};
  const auto a{glm::dot(u, u)};
  const auto b{glm::dot(u, v)};
  const auto c{glm::dot(v, v)};
  const auto d{glm::dot(u, w0)};
  const auto e{glm::dot(v, w0)};

  // const float D = a * c - b * b;
  const float D = b * b - a * c;

  // Step 1: initial s — parallel check
  const float s0 =
      (glm::abs(D) > 1e-6f) ? glm::clamp((b * d - a * e) / D, 0.0f, 1.0f) : 0.0f;

  // Step 2: project s0 onto segment A to get t, clamp
  const float t1 = glm::clamp((s0 * b - d) / a, 0.0f, 1.0f);

  // Step 3: project t1 back onto segment B to get final s, clamp
  const float sf = glm::clamp((t1 * b + e) / c, 0.0f, 1.0f);

  // Final closest points
  const auto P = segA.p0 + t1 * u;
  const auto Q = segB.p0 + sf * v;

  const float dist = glm::length(P - Q);

  std::cout << "dist: " << dist << "\n";
  std::cout << "P: " << P.x << ", " << P.y << ", " << P.z << "\n";
  std::cout << "Q: " << Q.x << ", " << Q.y << ", " << Q.z << "\n";
}

void shortestBrute(const Segment &segA, const Segment &segB) {
  const auto u{segA.p1 - segA.p0};
  const auto v{segB.p1 - segB.p0};

  const int N = 100;
  float bestDist = std::numeric_limits<float>::max();
  float bestT = 0.0f;
  float bestS = 0.0f;

  for (int i = 0; i <= N; i++) {
    const float t = static_cast<float>(i) / N;
    for (int j = 0; j <= N; j++) {
      const float s = static_cast<float>(j) / N;
      const auto P = line(t, segA.p0, u);
      const auto Q = line(s, segB.p0, v);
      const float dist = glm::length(P - Q);
      if (dist < bestDist) {
        bestDist = dist;
        bestT = t;
        bestS = s;
      }
    }
  }

  const auto P = line(bestT, segA.p0, u);
  const auto Q = line(bestS, segB.p0, v);
  std::cout << "dist: " << bestDist << "\n";
  std::cout << "P: " << P.x << ", " << P.y << ", " << P.z << "\n";
  std::cout << "Q: " << Q.x << ", " << Q.y << ", " << Q.z << "\n";
}

int main() {
  // Case 1: Normal case, both t and s within bounds (interior solution)
  // Skew lines in 3D, closest points somewhere in the middle of both segments
  {
    Segment a{{0, 0, 0}, {4, 0, 0}};
    Segment b{{2, 1, 2}, {2, -1, -2}};
    std::cout << "Case 1 - interior solution:\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case 2: t out of bounds (past A1 end), s within bounds
  // Segment A ends before reaching the closest point on infinite line
  {
    Segment a{{0, 0, 0}, {1, 0, 0}};
    Segment b{{5, 1, 0}, {5, -1, 0}};
    std::cout << "Case 2 - t out of bounds (past end of A):\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case 3: s out of bounds (past B1 end), t within bounds
  // Segment B ends before reaching the closest point on infinite line
  {
    Segment a{{0, 1, 0}, {4, 1, 0}};
    Segment b{{2, 0, 0},
              {2, 0, 1}}; // short segment, closest point would be beyond B1
    std::cout << "Case 3 - s out of bounds (past end of B):\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case 4: both t and s out of bounds — corner case (0,0)
  // Both closest points clamp to their p0 endpoints
  {
    Segment a{{0, 0, 0}, {-2, 0, 0}};
    Segment b{{3, 3, 0}, {3, 5, 0}};
    std::cout << "Case 4 - both out of bounds, corner (t=0,s=0):\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case 5: both t and s out of bounds — corner case (1,1)
  // Both closest points clamp to their p1 endpoints
  {
    Segment a{{0, 0, 0}, {1, 0, 0}};
    Segment b{{5, 0, 0}, {10, 0, 0}};
    std::cout << "Case 5 - both out of bounds, corner (t=1,s=0):\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case 6: parallel segments, side by side
  // D ~ 0, falls into parallel branch, s defaults to 0
  {
    Segment a{{0, 0, 0}, {4, 0, 0}};
    Segment b{{0, 1, 0}, {4, 1, 0}};
    std::cout << "Case 6 - parallel segments:\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case 7: parallel segments, one staggered ahead of the other
  // Parallel but non-overlapping, closest points are the two nearest endpoints
  {
    Segment a{{0, 0, 0}, {2, 0, 0}};
    Segment b{{5, 1, 0}, {8, 1, 0}};
    std::cout << "Case 7 - parallel staggered (non-overlapping):\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case 8: segments that nearly intersect (nearly coplanar skew lines)
  // Closest distance approaches 0, tests numerical stability
  {
    Segment a{{0, 0, 0}, {4, 0, 0}};
    Segment b{{2, 0.001f, 2}, {2, 0.001f, -2}};
    std::cout << "Case 8 - nearly intersecting:\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case BREAK: both closest points interior, but B0 is far from the solution
  // Infinite line solution: t≈0.5, s≈0.5
  // Starting from s0=0 (B0) should pull t1 toward the wrong end
  {
    Segment a{{0, 0, 0}, {2, 0, 0}};
    Segment b{{1, -10, 1}, {1, 10, -1}};
    std::cout << "Case BREAK:\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  // Case BREAK2: long diagonal segments, interior solution far from s=0 end
  {
    Segment a{{0, 0, 0}, {0, 10, 0}};
    Segment b{{1, 9, 0}, {-1, 1, 0}};
    std::cout << "Case BREAK2:\n";
    shortest(a, b);
    shortestClaude(a, b);
    shortestBrute(a, b);
  }

  return 0;
}
