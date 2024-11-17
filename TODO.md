
# TODO

[ ] New UI: try to implement this in a different project first
    [ ] create UI library :)

[ ] Port from C++ to C: this will be done during the overall rewrite, meaning that only at the end of it the build script will be actually update to use only C
    [ ] substitute glm with mathy
    [ ] typedef declarations
    [ ] MAYBE update opengl bindings
    [ ] update build script

[ ] New obstacles/portals/boosts
    [ ] update data structure (generic polygon with a limit on the number of vertices)
    [ ] update collision algorithm (gjk?)
        [ ] make sure that vertices are in counter-clockwise order
        [ ] port the gjk from the test implementation of gjk2D
    [ ] update rendering (use rendy)
        [ ] figure out how to apply textures (maybe each triangle of the object is a "glued piece of paper", that together actually make up the object)

[ ] New plane
    [ ] More complete hitbox

[ ] New rider
    [ ] More complete hitbox

[ ] Textures
    [ ] Plane (multiple types?)
    [ ] Rider (multiple types?)
    [ ] Obstacles (each type has just a different color?)
    [ ] Portals
        [ ] Anti-gravity
        [ ] Shuffler
    [ ] Boosts
        [ ] Represent boost direction
        [ ] Represent boost strength

[ ] Check for memory leaks with valgrind
