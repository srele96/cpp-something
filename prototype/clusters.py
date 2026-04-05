import numpy as np
import plotly.graph_objects as go
from typing import Callable


def create_projection_matrix(fov_deg, aspect, near, far):
    fov_rad = np.radians(fov_deg)
    f = 1.0 / np.tan(fov_rad / 2.0)

    projection = np.zeros((4, 4))

    projection[0, 0] = f / aspect
    projection[1, 1] = f
    projection[2, 2] = (far + near) / (near - far)
    projection[2, 3] = (2 * far * near) / (near - far)
    projection[3, 2] = -1.0

    return projection


class Face:
    top_left: np.ndarray
    top_right: np.ndarray
    bottom_left: np.ndarray
    bottom_right: np.ndarray

    def __init__(self, top_left=None, top_right=None, bottom_left=None,
                 bottom_right=None):
        self.top_left = self.__default(top_left)
        self.top_right = self.__default(top_right)
        self.bottom_left = self.__default(bottom_left)
        self.bottom_right = self.__default(bottom_right)

    def __default(self, vec):
        return vec if vec is not None else np.array([0, 0, 0, 1])

    def __perspectiveDivide(self, vec):
        return vec[:3] / vec[3]

    def unproject(self, matrix):
        top_left = self.__perspectiveDivide(matrix @ self.top_left)
        top_right = self.__perspectiveDivide(matrix @ self.top_right)
        bottom_left = self.__perspectiveDivide(matrix @ self.bottom_left)
        bottom_right = self.__perspectiveDivide(matrix @ self.bottom_right)

        return Face(top_left, top_right, bottom_left, bottom_right)

    def __str__(self):
        return (f"Face(top_left={self.top_left}, "
                f"top_right={self.top_right}, "
                f"bottom_left={self.bottom_left}, "
                f"bottom_right={self.bottom_right})")


def build_cluster(
    width_segments: int,
    height_segments: int,
    depth_segments: int,
    near_plane: Face,
    far_plane: Face,
    get_depth_percent: Callable[[float], float]
):
    # =========================================================================
    # Short conceptual summary summary
    # =========================================================================
    # Width
    #
    # Near plane
    #
    # - Compute direction from top left to top right point
    # - Compute direction from bottom left to bottom right point
    # - From top left point, by top computed direction by percentage
    # - From bottom left point, by bottom computed direction, by percentage
    #
    # Far plane:
    #
    # - Compute direction from top left to top right point
    # - Compute direction from bottom left to bottom right point
    # - From top left point, by top computed direction by percentage
    # - From bottom left point, by bottom computed direction, by percentage
    #
    # =========================================================================
    # Height
    #
    # Near plane
    #
    # - Compute to bottom from top point and bottom point
    # - From computed top point, to computed bottom direction by percentage
    #
    # Far plane
    #
    # - Compute to bottom from top point and bottom point
    # - From computed top point, to computed bottom direction by percentage
    # =========================================================================
    # Depth
    #
    # - Compute directon from point on near plane to point on far plane
    # - From computed near plane point, by computed direction to far plane by
    # percentage
    # =========================================================================

    cluster = []

    for w in range(width_segments + 1):
        w_perc = w / width_segments

        dir_near_top_to_right = (near_plane.top_right - near_plane.top_left) \
            * w_perc
        dir_near_bottom_to_right = (near_plane.bottom_right -
                                    near_plane.bottom_left) * w_perc
        point_w_near_top = near_plane.top_left + dir_near_top_to_right
        point_w_near_bottom = near_plane.bottom_left + dir_near_bottom_to_right

        dir_far_top_to_right = (far_plane.top_right - far_plane.top_left) \
            * w_perc
        dir_far_bottom_to_right = (far_plane.bottom_right -
                                   far_plane.bottom_left) * w_perc
        point_w_far_top = far_plane.top_left + dir_far_top_to_right
        point_w_far_bottom = far_plane.bottom_left + dir_far_bottom_to_right

        for h in range(height_segments + 1):
            h_perc = (h / height_segments)

            dir_near_to_bottom = (point_w_near_bottom - point_w_near_top) \
                * h_perc
            point_h_near = point_w_near_top + dir_near_to_bottom

            dir_far_to_bottom = (point_w_far_bottom - point_w_far_top) * h_perc
            point_h_far = point_w_far_top + dir_far_to_bottom

            for d in range(depth_segments + 1):
                z_percent = get_depth_percent(d)

                dir_to_far = (point_h_far - point_h_near) * z_percent
                point_z = point_h_near + dir_to_far

                cluster.append(point_z)

    return cluster


def main():
    fov = 60
    width = 1080
    height = 720
    near = 2
    far = 30

    projection = create_projection_matrix(fov, width / height, near, far)
    projection_inverse = np.linalg.inv(projection)

    near_topLeft = np.array([-1,  1, -1, 1])
    near_topRight = np.array([1,  1, -1, 1])
    near_bottomLeft = np.array([-1, -1, -1, 1])
    near_bottomRight = np.array([1, -1, -1, 1])

    far_topLeft = np.array([-1,  1,  1, 1])
    far_topRight = np.array([1,  1,  1, 1])
    far_bottomLeft = np.array([-1, -1,  1, 1])
    far_bottomRight = np.array([1, -1,  1, 1])

    near_NDC = Face(near_topLeft, near_topRight,
                    near_bottomLeft, near_bottomRight)
    far_NDC = Face(far_topLeft, far_topRight, far_bottomLeft, far_bottomRight)

    # It is of extreme importance for this prototype to use the unproject
    # method because in rendering, C++ code will unproject the projection
    # matrix
    near_plane = near_NDC.unproject(projection_inverse)
    far_plane = far_NDC.unproject(projection_inverse)

    print(near_plane)
    print(far_plane)

    width_segments = 16
    height_segments = 16
    depth_segments = 24

    def compute_depth_percent(segment: int):
        z_from_origin = near * pow(far / near, segment / depth_segments)
        frustum_z = z_from_origin - near
        frustum_length = far - near
        z_percent = frustum_z / frustum_length
        return z_percent

    cluster = build_cluster(width_segments=width_segments,
                            height_segments=height_segments,
                            depth_segments=depth_segments,
                            near_plane=near_plane, far_plane=far_plane,
                            get_depth_percent=compute_depth_percent)

    # TODO: Build the Axis-Aligned Bounding Box list

    xs = [p[0] for p in cluster]
    ys = [p[1] for p in cluster]
    zs = [p[2] for p in cluster]

    fig = go.Figure(
        data=[go.Scatter3d(
            x=xs, y=ys, z=zs,
            mode='markers',
            marker=dict(size=2, opacity=0.6)
        )]
    )

    fig.show()


if __name__ == "__main__":
    main()
