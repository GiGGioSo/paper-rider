# Paper Rider: Rect → Polygon Refactor Plan

## Current State

**Mixed Polygon System:**
- Some objects use `PR_Rect` (UI, plane, rider, particles)
- Some objects use `PR_Polygon` (obstacles, portals, boostpads)
- UI elements (buttons, frames) use `PR_Rect`

**Collision Functions:**
- `rect_contains_point()` - point-in-rect with rotation
- `rect_are_colliding()` - SAT-based rect collision
- Both already handle rotation via angle

**Polygon System:**
- `pr_polygon.h` has vertex-based polygons (3-4 vertices)
- Used for obstacles/boostpads/portal
- Collision uses GJK library (`gjk2d`)

---

## Migration Phases

### Phase 1: Foundation (PR_Polygon Type)

**Goal:** Define unified polygon type that subsumes PR_Rect

```c
// PR_Polygon replaces PR_Rect
typedef struct PR_Polygon {
    uint32 n_vertices;
    vec2f *vertices;      // array of vertex positions
    float angle;          // rotation around local origin
    union {
        struct {
            uint32 v[16];  // edge definitions [v1, v2]
            Edge edges[16];
        };
        struct {
            uint32 v1, v2; // for quad: two edges
        };
    };
} PR_Polygon;
```

**Migration Strategy:**
- PR_Rect as 4-vertex polygon for backwards compatibility
- Keep collision functions accepting both types via union/typedef
- Gradual replacement, not all at once
- Add helper functions for rect→polygon conversion

---

### Phase 2: Collision System

**Goal:** Universal collision functions

```c
// Point containment
bool polygon_contains_point(const PR_Polygon *poly, float px, float py);

// Object collision
bool polygon_are_colliding(const PR_Polygon *p1, const PR_Polygon *p2, float *cx, float *cy);

// Legacy compatibility  
bool rect_contains_point(...) { polygon_contains_point(&rect_as_poly, ...) }
bool rect_are_colliding(...) { polygon_are_colliding(&r1_as_poly, &r2_as_poly, ...) }
```

**Implementation:**
- Use SAT (Separating Axis Theorem) for rotation-agnostic collision
- Fall back to Minkowski-sum approach
- Handle both convex and concave polygons (obstacles can be concave)
- Use existing GJK library for convex polygons

---

### Phase 3: Gameplay Objects

**Priority Order:**
1. **Obstacles/Portals/Boostpads** (already polygons) - convert to PR_Polygon
2. **Plane/Rider** - replace rect with polygon body
3. **Particles** - convert from rect to point/polygon
4. **UI elements** - migrate to polygon-based system

**Example - Plane:**
```c
typedef struct PR_Plane {
    PR_Polygon body;      // replaces PR_Rect body
    PR_Polygon render_zone;
    // ... existing fields ...
} PR_Plane;
```

---

### Phase 4: Editor Integration

**Goal:** Polygon editing in editor mode

- Editor needs to:
  - Allow adding/removing vertices
  - Manipulate control points
  - Handle vertex reordering
  - Preview polygon as it's being edited

**Editor State:**
```c
typedef struct PR_EditorPolygon {
    PR_Polygon polygon;
    uint32 selected_vertex;
    uint32 selected_edge;
    vec2f drag_start_point;  // for dragging
} PR_EditorPolygon;
```

**Editor Functions:**
- `editor_polygon_add_vertex()` - add vertex at mouse
- `editor_polygon_remove_vertex()` - delete current vertex  
- `editor_polygon_set_vertices()` - import/edit vertex array
- `editor_polygon_snap_to_grid()` - snapping helpers

---

### Phase 5: UI Elements

**Goal:** Convert all UI to polygons

**UI Objects Using Polygons:**
- `PR_Button.body` → `PR_Polygon`
- Button frames, borders
- Menu backgrounds
- Selection highlights
- Deleting frames
- Camera parallax pieces

**Button Example:**
```c
typedef struct PR_Button {
    bool from_center;
    PR_Polygon body;     // was PR_Rect
    vec4f col;
    char text[256];
} PR_Button;
```

**UI Collision Benefits:**
- Rounded corners on buttons
- Arbitrary shapes for hover states
- Better collision with tilted UI elements
- Custom shapes for special effects

---

### Phase 6: Rendering Pipeline

**Current Renderer:**
- `uni_vbo` - unicolor rectangles
- `tex_vbo` - textured quad
- `array_tex_vbo` - textured array
- `text_vbo` - text rendering

**Updated Renderer:**
- `poly_vbo` - polygon meshes for everything
- Batch identical polygons
- Use vertex instancing
- Texture atlas still works (UVs per vertex)

**Shaders:**
- Modify existing shaders to support vertex arrays
- Add UV attributes to polygon vertices
- Support multiple texture layers

---

### Phase 7: Data Conversion

**Level Files:**
- Current format stores rectangles as: `{x, y, w, h, angle}`
- New format stores: `[v1x, v1y, v2x, v2y, ...]`
- **Backwards compatibility:** Include legacy reader

**Migration Script:**
- Parse existing level files
- Convert rect specs to polygons
- Preserve rotation and size data
- Generate new level data

---

### Phase 8: Cleanup

**Deprecate PR_Rect:**
- Keep for 1 version (warning only)
- Add deprecation comments
- Convert all usages during that period
- Finally remove

**Deprecate PR_Button.text:**
- Keep text field for UI rendering
- Move to separate PR_UI_Text struct if needed
- Or keep text in button but render via mesh

---

## Implementation Roadmap

**Week 1-2: Phase 1-2**
- Define `PR_Polygon` + conversion helpers
- Implement/collide point + polygon collision
- Update collision functions
- Test with existing obstacles/boostpads

**Week 3-4: Phase 3-4** 
- Convert plane/rider
- Convert UI elements (buttons, frames)
- Convert particles
- Update editor state structures

**Week 5-6: Phase 5-7**
- Update renderer to handle polygons
- Implement vertex instance batching
- Convert level loader
- Convert editor tools

**Week 7-8: Phase 8 + Integration**
- Run full playtests
- Fix bugs discovered
- Performance benchmarks
- Cleanup and final polish

---

## Key Decisions

**1. Vertex Order:**
- Clockwise or counter-clockwise?
- **Decision:** Counter-clockwise (standard in OpenGL)

**2. Convex vs Concave:**
- **Decision:** Support both. Use SAT for convex, merge/convex-decompose for concave

**3. Performance Budget:**
- Max polygons per object: 32 vertices?
- **Decision:** Limit to 16 vertices for gameplay objects
- Use quad/convex for most UI and simple shapes

**4. Editor Vertex Drag:**
- Drag vertices or control points?
- **Decision:** Drag vertices directly, with Bezier handles for edges

**5. Legacy Data:**
- Support both formats in levels?
- **Decision:** Yes. Parse legacy rects AND new polygon data

---

## Testing Strategy

**Before each phase:**
1. Update unit tests with new types
2. Add regression tests for converted code
3. Visual regression tests (screenshots)

**After full refactor:**
1. Speedrun existing levels (ensure gameplay unchanged)
2. Performance profiling
3. Memory usage comparison
4. Edge case testing (rotated objects, diagonal collisions)

---

## Risk Assessment

**High Risk:**
- Collision correctness (gamedeciding physics)
- Renderer vertex batching (memory/performance)
- Editor stability (adding/removing vertices)

**Medium Risk:**
- UI layout changes from polygon rendering
- Level file conversions
- Asset import/export

**Mitigation:**
- Keep existing rect code as reference
- Run both systems in parallel during transition
- Extensive visual testing
- Unit test every collision scenario

---

## Success Criteria

- [ ] Collision behavior identical to before
- [ ] No memory leaks
- [ ] Frame rate stable/improved
- [ ] Editor can create/edit all levels
- [ ] All levels play correctly
- [ ] No new critical bugs
