import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


def main():
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    b_min = np.array([0, 0, 0])
    b_max = np.array([2, 2, 2])

    s_pos = np.array([6, 1.5, 1.5])
    s_radius = 4.0

    # =========================================================================
    # AABB bounding box
    x = [b_min[0], b_max[0]]
    y = [b_min[1], b_max[1]]
    z = [b_min[2], b_max[2]]

    faces = [
        [[x[0], y[0], z[0]], [x[1], y[0], z[0]], [
            x[1], y[1], z[0]], [x[0], y[1], z[0]]],  # Bottom
        [[x[0], y[0], z[1]], [x[1], y[0], z[1]], [
            x[1], y[1], z[1]], [x[0], y[1], z[1]]],  # Top
        [[x[0], y[0], z[0]], [x[0], y[1], z[0]], [
            x[0], y[1], z[1]], [x[0], y[0], z[1]]],  # Left
        [[x[1], y[0], z[0]], [x[1], y[1], z[0]], [
            x[1], y[1], z[1]], [x[1], y[0], z[1]]],  # Right
        [[x[0], y[0], z[0]], [x[1], y[0], z[0]], [
            x[1], y[0], z[1]], [x[0], y[0], z[1]]],  # Front
        [[x[0], y[1], z[0]], [x[1], y[1], z[0]], [
            x[1], y[1], z[1]], [x[0], y[1], z[1]]]  # Back
    ]
    ax.add_collection3d(Poly3DCollection(
        faces,
        facecolors='cyan',
        linewidths=1,
        edgecolors='b',
        alpha=.15,
        label="AABB"
    ))

    # =========================================================================
    # Sphere

    u, v = np.mgrid[0:2*np.pi:20j, 0:np.pi:10j]
    sx = s_pos[0] + s_radius * np.cos(u) * np.sin(v)
    sy = s_pos[1] + s_radius * np.sin(u) * np.sin(v)
    sz = s_pos[2] + s_radius * np.cos(v)

    ###########################################################################
    # Note
    #
    # In case a bounding box is axis-aligned we can clamp the sphere position
    # to the bounding box edge.
    # We clamp it on x, y, z axis separately.
    #
    # The clamped value is the sphere's nearest point on the bounding box.
    ###########################################################################

    # Intentionally don't use np.clip
    closest = np.array([
        min(b_max[0], max(b_min[0], s_pos[0])),
        min(b_max[1], max(b_min[1], s_pos[1])),
        min(b_max[2], max(b_min[2], s_pos[2])),
    ])

    # 3D distance equation (avoid taking square root for compute performance)
    dist_sq = np.sum((s_pos - closest)**2)
    hit = dist_sq <= s_radius**2

    color = "red" if hit else "gold"
    ax.plot_wireframe(sx, sy, sz, color=color, alpha=0.3)
    ax.scatter(*s_pos, color=color, s=50)

    # =========================================================================
    # Line from sphere center to the closest point

    ax.plot(
        [s_pos[0], closest[0]],
        [s_pos[1], closest[1]],
        [s_pos[2], closest[2]],
        color='purple',
        linestyle='--'
    )

    # =========================================================================

    ax.set_xlabel('X', color='red', fontweight='bold')
    ax.set_ylabel('Y', color='green', fontweight='bold')
    ax.set_zlabel('Z', color='blue', fontweight='bold')
    ax.set_xlim(b_min[0] - 8, b_max[0] + 8)
    ax.set_ylim(b_min[1] - 8, b_max[1] + 8)
    ax.set_zlim(b_min[2] - 8, b_max[2] + 8)

    ax.legend()
    plt.show()


if __name__ == '__main__':
    main()
