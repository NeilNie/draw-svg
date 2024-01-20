#include "viewport.h"

#include "CS248.h"

namespace CS248
{

    void ViewportImp::set_viewbox(float x, float y, float span)
    {

        // Task 3 (part 2):
        // Set svg to normalized device coordinate transformation.
        // Your input arguments are defined as SVG canvans coordinates.

        this->x = x;
        this->y = y;
        this->span = span;

        svg_2_norm = Matrix3x3::identity();

        svg_2_norm(0, 0) = 1 / (2 * span);
        svg_2_norm(1, 1) = 1 / (2 * span);

        svg_2_norm(0, 2) = -(x - span) / (2 * span);
        svg_2_norm(1, 2) = -(y - span) / (2 * span);

    }

    void ViewportImp::update_viewbox(float dx, float dy, float scale)
    {

        this->x -= dx;
        this->y -= dy;
        this->span *= scale;
        set_viewbox(x, y, span);
    }

} // namespace CS248
