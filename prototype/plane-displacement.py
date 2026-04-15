from dataclasses import dataclass
import numpy as np
import plotly.graph_objects as go

# NOTE: +z is up, +x is right, +y is front


@dataclass
class Vertex:
    position: np.ndarray
    normal: np.ndarray
    tangent: np.ndarray
    bitangent: np.ndarray
    uv: np.ndarray


def plotTangentBitangentNormal(fig, vertices, stride=6):
    px = [v.position[0] for v in vertices]
    py = [v.position[1] for v in vertices]
    pz = [v.position[2] for v in vertices]

    def plt(attr, color, name):
        u_dir = [getattr(v, attr)[0] for v in vertices]
        v_dir = [getattr(v, attr)[1] for v in vertices]
        w_dir = [getattr(v, attr)[2] for v in vertices]

        fig.add_trace(go.Cone(
            x=px, y=py, z=pz, u=u_dir, v=v_dir, w=w_dir,
            colorscale=[[0, color], [1, color]],
            showscale=False,
            sizemode="absolute",
            sizeref=1.0,  # Adjust size of arrows
            name=name,
            anchor="tail"
        ))

    plt('tangent', 'red', 'Tangent (T)')
    plt('bitangent', 'green', 'Bitangent (B)')
    plt('normal', 'blue', 'Normal (N)')


def main():
    # the size of the plane
    width = 6
    height = 6
    vertices = []

    U_COUNT = 24
    V_COUNT = 24

    # create vertices from UV mapping grid at fixed height
    for u in range(U_COUNT):
        # to percentage, up to 100%
        u_perc = u / (U_COUNT - 1)
        for v in range(V_COUNT):
            # to percentage, up to 100%
            v_perc = v / (V_COUNT - 1)

            position = np.array([
                u_perc * width,
                v_perc * height,
                # we will pull z coordinate with a function
                # in a direction of a normal
                0
            ])
            # Make sure is normalized
            # To perform displacement, you need to know tangent and bitangent
            # (local right and forward vectors)
            normal = np.array([0, 0, 1])
            tangent = np.array([1, 0, 0])
            bitangent = np.array([0, 1, 0])

            vertices.append(Vertex(position, normal, tangent,
                            bitangent, np.array([u_perc, v_perc])))

    def displacementFunc(u, v):
        return np.sin(u * 2.0 * np.pi) + np.sin(v * 2.0 * np.pi)

    for vert in vertices:
        u, v = vert.uv[0], vert.uv[1]

        # NOTE: This is an approximation method, hence it needs
        # adjustments of the delta value
        delta = 0.001
        # Ensure delta fits the scale of the plane
        u_delta = delta / width
        v_delta = delta / height

        h = displacementFunc(u, v)
        h_u = displacementFunc(u + u_delta, v)
        h_v = displacementFunc(u, v + v_delta)

        # we need initial position and normal to compute new
        # SAMPLED tangent & bitangent position
        pos_tangent = vert.position + vert.tangent * \
            delta + h_u * vert.normal
        pos_bitangent = vert.position + vert.bitangent * \
            delta + h_v * vert.normal

        # compute displacement
        pos_displaced = vert.position + h * vert.normal

        # compute tangent and bitangent direction vectors
        tangent = (pos_tangent - pos_displaced) / delta
        bitangent = (pos_bitangent - pos_displaced) / delta

        normal = np.cross(tangent, bitangent)

        vert.position = pos_displaced

        vert.normal = normal / np.linalg.norm(normal)
        vert.tangent = tangent / np.linalg.norm(tangent)
        vert.bitangent = bitangent / np.linalg.norm(bitangent)

    # Just some plotly stuff that's unrelated to
    # mathematical/rendering concepts
    x = np.array([v.position[0] for v in vertices])
    y = np.array([v.position[1] for v in vertices])
    z = np.array([v.position[2] for v in vertices])

    fig = go.Figure()

    # a plane
    fig.add_trace(go.Surface(
        x=x.reshape(U_COUNT, V_COUNT),
        y=y.reshape(U_COUNT, V_COUNT),
        z=z.reshape(U_COUNT, V_COUNT),
        colorscale='Viridis',
        opacity=0.8,
        showscale=False
    ))

    plotTangentBitangentNormal(fig, vertices)

    fig.update_layout(
        scene=dict(
            xaxis_title='X (Right)',
            yaxis_title='Y (Front)',
            zaxis_title='Z (Up)',
            aspectmode='data'
        ),
        title="3d vertex displacement"
    )

    fig.show()


if __name__ == "__main__":
    main()
