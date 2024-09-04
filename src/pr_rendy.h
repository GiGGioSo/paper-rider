///
///////////////////////////////
/// RENDY THE (2D) RENDERER ///
///////////////////////////////
///

// Custom z layering

// - group by z level (needs a stable sorting)
// - group by shader
// - group by opaque/transparent


enum Layer {
    FRONT = 2,
    MIDDLE = 1,
    BACK = 0,
};
