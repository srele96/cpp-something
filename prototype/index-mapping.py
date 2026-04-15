import numpy as np


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


def get_indices(x, y, z, projection, grid_w, grid_h, grid_d, near, far):
    z_abs = abs(z)
    z_perc = np.log(z_abs / near) / np.log(far / near)
    idx_z = int(np.floor(np.clip(z_perc * grid_d, 0, grid_d - 1)))

    pt_view = np.array([x, y, z, 1.0])
    pt_ndc = projection @ pt_view
    pt_ndc = pt_ndc / pt_ndc[3]
    pt_perc = (pt_ndc + 1.0) / 2.0

    print(f"Percent: {pt_perc}")

    # Note: In the C++ implementation we're better off discarding
    # the clusters which aren't lit
    idx_x = int(np.floor(np.clip(pt_perc[0] * grid_w, 0, grid_w - 1)))
    idx_y = int(np.floor(np.clip(pt_perc[1] * grid_h, 0, grid_h - 1)))

    print(f"Before np.floor: {pt_perc[0] * grid_w}, "
          f"{pt_perc[1] * grid_h}, "
          f"{z_perc * grid_d}")
    print(f"AFter  np.floor: {idx_x}, {idx_y}, {idx_z}")


window_width = 800
window_height = 600
near = 0.1
far = 100

# Make matching projection matrix like in the C++ code, so we can take the csv
# cluster data, plug it in here, and figure out the indexing problem
projection = create_projection_matrix(
    60, window_width / window_height, near, far)

grid_w = 16
grid_h = 16
grid_d = 24


xmin = 50.511
ymin = 37.8832
zmin = -99.9977

xmax = 76.9783
ymax = 57.7337
zmax = -74.9893

get_indices(xmin, ymin, zmin, projection, grid_w, grid_h, grid_d, near, far)
get_indices(xmin, ymin, zmin + (zmax - zmin) / 2,
            projection, grid_w, grid_h, grid_d, near, far)
get_indices(xmax, ymax, zmax, projection, grid_w, grid_h, grid_d, near, far)
