//
//          +-------------------max
//          |                     |
//          |                     |
//          |        <-r->        |
//          |                     |
//          |      . * .          |
//          |     *  c  *         |
//          |      - . -          |
//          |                     |
//          min-------------------+
//                    L
//

#define bool unsigned int
bool circle_rectangle_collides(vec2 c, float radius, vec2 min, vec2 max) {
    float test; {
        test = 0.0f;
        for (int d = 0; d < 2; ++d) {
            if (c[d] < min[d]) {
                float tmp = (min[d] - c[d]);
                test += (tmp * tmp);
            } else if (max[d] < c[d]) {
                float tmp = (c[d] - max[d]);
                test += (tmp * tmp);
            }
        }
    }
    float squared_radius = (radius * radius);
    return(test < squared_radius);
}