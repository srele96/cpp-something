# Clustered Shading Implementation Roadmap

- [ ] **1. Grid Definition & Camera Frustum Slicing**
  - [ ] **1.1 Define Grid Resolution:** Choose a 3D grid size (e.g., $16 \times 16 \times 24$).
  - [ ] **1.2 Linear X/Y Partitioning:** Divide the screen into a 2D tile grid in screen space.
  - [ ] **1.3 Logarithmic Z-Slicing:** Slice the depth ($Z$) logarithmically so that clusters are smaller near the near-plane and larger further away.
  - [ ] **1.4 Cluster AABB Generation:** Calculate the 8 corners of each cluster in View Space and store them as Axis-Aligned Bounding Boxes (AABBs).

- [ ] **2. CPU-Side Light Culling (The "Clustering")**
  - [ ] **2.1 View Space Transformation:** Transform all Point/Spot light positions into View Space once per frame.
  - [ ] **2.2 Light-AABB Intersection:** For every cluster, loop through the light list and check if the light's sphere/cone intersects the cluster's AABB.
  - [ ] **2.3 Light Index List:** For each cluster, store a list of the IDs (indices) of the lights that touch it.

- [ ] **3. Data Flattening for OpenGL 3.3**
  - [ ] **3.1 The Global Light Table:** Store your actual light data (Position, Color, Radius) in a Texture Buffer Object (TBO) or a large 1D Texture.
  - [ ] **3.2 The Grid Lookup Texture:** Create a 3D Texture (or a 2D Texture Atlas) where each "pixel" represents a cluster.
    - [ ] **3.2.1 R channel:** Offset (Where does this cluster's light list start in the Index List?).
    - [ ] **3.2.2 G channel:** Count (How many lights are in this cluster?).
  - [ ] **3.3 The Index List Texture:** Flatten all the individual cluster light lists into one long 1D array/texture. The "Offset" from the Grid Texture points into this.

- [ ] **4. CPU-to-GPU Data Transfer**
  - [ ] **4.1 Buffer Updates:** Use `glTexSubImage` or `glBufferSubData` to upload the three data sets every frame:
  - [ ] **4.2** Upload The Light Data (Positions/Colors).
  - [ ] **4.3** Upload The Grid Lookup (Offset/Count).
  - [ ] **4.4** Upload The Flattened Index List.

- [ ] **5. Fragment Shader Implementation**
  - [ ] **5.1 Calculate Cluster Index:**
    - [ ] **5.1.1** Get the fragment's `gl_FragCoord.xy` to find the X/Y cluster.
    - [ ] **5.1.2** Use the fragment's View Space $Z$ and your logarithmic formula to find the $Z$ cluster.
  - [ ] **5.2 Fetch Grid Info:** Sample the Grid Lookup Texture using the $(X, Y, Z)$ index to get the Offset and Count.
  - [ ] **5.3 The Light Loop:**
    - [ ] **5.3.1** Loop from $0$ to `Count`.
    - [ ] **5.3.2** Fetch the `LightID` from the Index List Texture using `Offset + i`.
    - [ ] **5.3.3** Fetch the `LightData` from the Global Light Table using `LightID`.
    - [ ] **5.3.4** Calculate lighting (Phong/Blinn-Phong/PBR).

- [ ] **6. Optimization & Constraints**
  - [ ] **6.1 Hard Limit:** Decide on a maximum number of lights per cluster (e.g., 64 or 128) to keep your textures manageable.
  - [ ] **6.2 Texture Formats:** Use `GL_R32UI` or `GL_RGB32F` for your data textures to ensure you don't lose precision.
  - [ ] **6.3 Frustum Culling:** Perform a broad frustum cull on the CPU first to ignore lights that aren't visible to the camera at all.
