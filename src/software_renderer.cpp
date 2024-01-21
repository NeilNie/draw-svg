#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CS248
{

    // Implements SoftwareRenderer //

    // fill a sample location with color
    void SoftwareRendererImp::fill_sample(int sx, int sy, const Color &color)
    {
        // Task 2: implement this function

        // check bounds
        if (sx < 0 || sx >= width * sample_rate)
            return;
        if (sy < 0 || sy >= height * sample_rate)
            return;

        Color pixel_color;
        float inv255 = 1.0 / 255.0;
        pixel_color.r = supersample_buffer[4 * (sx + sy * width * sample_rate)] * inv255;
        pixel_color.g = supersample_buffer[4 * (sx + sy * width * sample_rate) + 1] * inv255;
        pixel_color.b = supersample_buffer[4 * (sx + sy * width * sample_rate) + 2] * inv255;
        pixel_color.a = supersample_buffer[4 * (sx + sy * width * sample_rate) + 3] * inv255;

        pixel_color = ref->alpha_blending_helper(pixel_color, color);

        supersample_buffer[4 * (sx + sy * width * sample_rate)] = (uint8_t)(pixel_color.r * 255);
        supersample_buffer[4 * (sx + sy * width * sample_rate) + 1] = (uint8_t)(pixel_color.g * 255);
        supersample_buffer[4 * (sx + sy * width * sample_rate) + 2] = (uint8_t)(pixel_color.b * 255);
        supersample_buffer[4 * (sx + sy * width * sample_rate) + 3] = (uint8_t)(pixel_color.a * 255);
    }

    // fill samples in the entire pixel specified by pixel coordinates
    void SoftwareRendererImp::fill_pixel(int x, int y, const Color &color)
    {

        // Task 2: Re-implement this function
        for (int column = x * sample_rate; column < x * sample_rate + sample_rate; column += 1) 
        {
            for (int row = y * sample_rate; row < y * sample_rate + sample_rate; row += 1)
            {
                fill_sample(column, row, color);
            }
        }

        // check bounds
        if (x < 0 || x >= width)
            return;
        if (y < 0 || y >= height)
            return;

        Color pixel_color;
        float inv255 = 1.0 / 255.0;
        pixel_color.r = pixel_buffer[4 * (x + y * width)] * inv255;
        pixel_color.g = pixel_buffer[4 * (x + y * width) + 1] * inv255;
        pixel_color.b = pixel_buffer[4 * (x + y * width) + 2] * inv255;
        pixel_color.a = pixel_buffer[4 * (x + y * width) + 3] * inv255;

        pixel_color = ref->alpha_blending_helper(pixel_color, color);

        pixel_buffer[4 * (x + y * width)] = (uint8_t)(pixel_color.r * 255);
        pixel_buffer[4 * (x + y * width) + 1] = (uint8_t)(pixel_color.g * 255);
        pixel_buffer[4 * (x + y * width) + 2] = (uint8_t)(pixel_color.b * 255);
        pixel_buffer[4 * (x + y * width) + 3] = (uint8_t)(pixel_color.a * 255);
    }

    void SoftwareRendererImp::draw_svg(SVG &svg)
    {

        // set top level transformation
        transformation = canvas_to_screen;

        // canvas outline
        Vector2D a = transform(Vector2D(0, 0));
        a.x--;
        a.y--;
        Vector2D b = transform(Vector2D(svg.width, 0));
        b.x++;
        b.y--;
        Vector2D c = transform(Vector2D(0, svg.height));
        c.x--;
        c.y++;
        Vector2D d = transform(Vector2D(svg.width, svg.height));
        d.x++;
        d.y++;

        svg_bbox_top_left = Vector2D(a.x + 1, a.y + 1);
        svg_bbox_bottom_right = Vector2D(d.x - 1, d.y - 1);

        // draw all elements
        for (size_t i = 0; i < svg.elements.size(); ++i)
        {
            draw_element(svg.elements[i]);
        }

        // draw canvas outline
        rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
        rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
        rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
        rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

        // resolve and send to pixel buffer
        resolve();
    }

    void SoftwareRendererImp::set_sample_rate(size_t sample_rate)
    {

        // Task 2:
        // You may want to modify this for supersampling support
        printf("set sample once, resized sample buffer, %zu \n", sample_rate);
        this->sample_rate = sample_rate;
        this->super_sample_buffer_mem.resize(sample_rate * sample_rate * 4 * width * height);
        this->supersample_buffer = &(super_sample_buffer_mem[0]);
        memset(this->supersample_buffer, 255, sample_rate * sample_rate * 4 * width * height);
    }

    void SoftwareRendererImp::set_pixel_buffer(unsigned char *pixel_buffer,
                                               size_t width, size_t height)
    {

        // Task 2: setting the pixel buffer and super sampling pixel buffer
        printf("set pixel buffer once, resized sample buffer, %zu \n", sample_rate);
        this->pixel_buffer = pixel_buffer;
        this->super_sample_buffer_mem.resize(sample_rate * sample_rate * 4 * width * height);
        this->supersample_buffer = &(super_sample_buffer_mem[0]);
        memset(this->supersample_buffer, 255, sample_rate * sample_rate * 4 * width * height);
        this->width = width;
        this->height = height;
    }

    Vector2D transform_vector_2d(Vector2D vector, Matrix3x3 transform)
    {
        auto output = transform * Vector3D(vector.x, vector.y, 1);
        return Vector2D(output.x, output.y);
    }

    void SoftwareRendererImp::draw_element(SVGElement *element)
    {

        // Task 3 (part 1):
        // Modify this to implement the transformation stack

        if (element->type == POINT)
        {
            auto point = static_cast<Point &>(*element);
            point.position = transform_vector_2d(point.position, element->transform);
            draw_point(point);
        } 
        else if (element->type == LINE) 
        {
            auto line = static_cast<Line &>(*element);
            line.to = transform_vector_2d(line.to, element->transform);
            line.from = transform_vector_2d(line.from, element->transform);
            draw_line(line);
        }
        else if (element->type == POLYLINE)
        {
            auto polyline = static_cast<Polyline &>(*element);
            for (int i = 0; i < polyline.points.size(); i++)
                polyline.points[i] = transform_vector_2d(polyline.points[i], element->transform);
            draw_polyline(polyline);
        }
        else if (element->type == RECT)
        {
            auto rect = static_cast<Rect &>(*element);
            rect.position = transform_vector_2d(rect.position, element->transform);
            rect.dimension = transform_vector_2d(rect.dimension, element->transform);
            draw_rect(rect);
        }
        else if (element->type == POLYGON)
        {
            auto polygon = static_cast<Polygon &>(*element);
            for (int i = 0; i < polygon.points.size(); i++)
                polygon.points[i] = transform_vector_2d(polygon.points[i], element->transform);
            draw_polygon(polygon);
        }
        else if (element->type == ELLIPSE)
        {
            auto ellipse = static_cast<Ellipse &>(*element);
            ellipse.center = transform_vector_2d(ellipse.center, element->transform);
            ellipse.radius = transform_vector_2d(ellipse.radius, element->transform);
            draw_ellipse(ellipse);
        }
        else if (element->type == IMAGE)
        {
            auto image = static_cast<Image &>(*element);
            image.position = transform_vector_2d(image.position, element->transform);
            image.dimension = transform_vector_2d(image.dimension, element->transform);
            draw_image(image);
        }
        else if (element->type == GROUP)
        {
            auto group = static_cast<Group &>(*element);
            for (int i = 0; i < group.elements.size(); i++)
                group.elements[i]->transform = element->transform * group.elements[i]->transform;
            draw_group(group);
        }

    }

    // Primitive Drawing //

    void SoftwareRendererImp::draw_point(Point &point)
    {

        Vector2D p = transform(point.position);
        rasterize_point(p.x, p.y, point.style.fillColor);
    }

    void SoftwareRendererImp::draw_line(Line &line)
    {

        Vector2D p0 = transform(line.from);
        Vector2D p1 = transform(line.to);
        rasterize_line(p0.x, p0.y, p1.x, p1.y, line.style.strokeColor);
    }

    void SoftwareRendererImp::draw_polyline(Polyline &polyline)
    {

        Color c = polyline.style.strokeColor;

        if (c.a != 0)
        {
            int nPoints = polyline.points.size();
            for (int i = 0; i < nPoints - 1; i++)
            {
                Vector2D p0 = transform(polyline.points[(i + 0) % nPoints]);
                Vector2D p1 = transform(polyline.points[(i + 1) % nPoints]);
                rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
            }
        }
    }

    void SoftwareRendererImp::draw_rect(Rect &rect)
    {

        Color c;

        // draw as two triangles
        float x = rect.position.x;
        float y = rect.position.y;
        float w = rect.dimension.x;
        float h = rect.dimension.y;

        Vector2D p0 = transform(Vector2D(x, y));
        Vector2D p1 = transform(Vector2D(x + w, y));
        Vector2D p2 = transform(Vector2D(x, y + h));
        Vector2D p3 = transform(Vector2D(x + w, y + h));

        // draw fill
        c = rect.style.fillColor;
        if (c.a != 0)
        {
            rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
            rasterize_triangle(p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c);
        }

        // draw outline
        c = rect.style.strokeColor;
        if (c.a != 0)
        {
            rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
            rasterize_line(p1.x, p1.y, p3.x, p3.y, c);
            rasterize_line(p3.x, p3.y, p2.x, p2.y, c);
            rasterize_line(p2.x, p2.y, p0.x, p0.y, c);
        }
    }

    void SoftwareRendererImp::draw_polygon(Polygon &polygon)
    {

        Color c;

        // draw fill
        c = polygon.style.fillColor;
        if (c.a != 0)
        {

            // triangulate
            vector<Vector2D> triangles;
            triangulate(polygon, triangles);

            // draw as triangles
            for (size_t i = 0; i < triangles.size(); i += 3)
            {
                Vector2D p0 = transform(triangles[i + 0]);
                Vector2D p1 = transform(triangles[i + 1]);
                Vector2D p2 = transform(triangles[i + 2]);
                rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
            }
        }

        // draw outline
        c = polygon.style.strokeColor;
        if (c.a != 0)
        {
            int nPoints = polygon.points.size();
            for (int i = 0; i < nPoints; i++)
            {
                Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
                Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
                rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
            }
        }
    }

    void SoftwareRendererImp::draw_ellipse(Ellipse &ellipse)
    {

        // Advanced Task
        // Implement ellipse rasterization
    }

    void SoftwareRendererImp::draw_image(Image &image)
    {

        // Advanced Task
        // Render image element with rotation

        Vector2D p0 = transform(image.position);
        Vector2D p1 = transform(image.position + image.dimension);

        rasterize_image(p0.x, p0.y, p1.x, p1.y, image.tex);
    }

    void SoftwareRendererImp::draw_group(Group &group)
    {

        for (size_t i = 0; i < group.elements.size(); ++i)
        {
            draw_element(group.elements[i]);
        }
    }

    // Rasterization //

    // The input arguments in the rasterization functions
    // below are all defined in screen space coordinates

    void SoftwareRendererImp::rasterize_point(float x, float y, Color color)
    {

        // fill in the nearest pixel
        int sx = (int)floor(x);
        int sy = (int)floor(y);

        // check bounds
        if (sx < 0 || sx >= width)
            return;
        if (sy < 0 || sy >= height)
            return;

        // fill sample - NOT doing alpha blending!
        // Call fill_pixel here to run alpha blending
        fill_sample(sx, sy, color);

        pixel_buffer[4 * (sx + sy * width)] = (uint8_t)(color.r * 255);
        pixel_buffer[4 * (sx + sy * width) + 1] = (uint8_t)(color.g * 255);
        pixel_buffer[4 * (sx + sy * width) + 2] = (uint8_t)(color.b * 255);
        pixel_buffer[4 * (sx + sy * width) + 3] = (uint8_t)(color.a * 255);
    }

    void SoftwareRendererImp::rasterize_line(float x0, float y0,
                                             float x1, float y1,
                                             Color color)
    {

        // Task 0:
        // Implement Bresenham's algorithm (delete the line below and implement your own)
        ref->rasterize_line_helper(x0, y0, x1, y1, width, height, color, this);

        return;

        int x, y, dx, dy, px, py, x_end, y_end;
        dx = x1 - x0;
        dy = y1 - y0;
        px = 2 * abs(dy) - abs(dx);
        py = 2 * abs(dx) - abs(dy);
        if (abs(dy) <= abs(dx))
        {
            if (dx >= 0)
            {
                x = x0;
                y = y0;
                x_end = x1;
            }
            else
            {
                x = x1;
                y = y1;
                x_end = x0;
            }
            fill_sample(x, y, color);
            for (int i = 0; x < x_end; i++)
            {
                x = x + 1;
                if (px < 0)
                {
                    px = px + 2 * abs(dy);
                }
                else
                {
                    if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                        y = y + 1;
                    else
                        y = y - 1;
                    px = px + 2 * (abs(dy) - abs(dx));
                }
                fill_sample(x, y, color);
            }
        }
        else
        {
            if (dy >= 0)
            {
                x = x0;
                y = y0;
                y_end = y1;
            }
            else
            {
                x = x1;
                y = y1;
                y_end = y0;
            }
            fill_sample(x, y, color);
            for (int i = 0; y < y_end; i++)
            {
                y = y + 1;
                if (py <= 0)
                {
                    py = py + 2 * abs(dx);
                }
                else
                {
                    if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                        x = x + 1;
                    else
                        x = x - 1;
                    py = py + 2 * (abs(dx) - abs(dy));
                }
                fill_sample(x, y, color);
            }
        }
        // Advanced Task
        // Drawing Smooth Lines with Line Width
    }

    bool is_counter_clockwise(float x0, float y0, float x1, float y1, float x2, float y2)
    {
        Vector3D v1 = Vector3D((double)x1, (double)y1, 0) - Vector3D((double)x0, (double)y0, 0);
        Vector3D v2 = Vector3D((double)x2, (double)y2, 0) - Vector3D((double)x1, (double)y1, 0);

        Vector3D cross;
        cross.x = v1.y * v2.z - v1.z * v2.y;
        cross.y = v1.x * v2.z - v1.z * v2.x;
        cross.z = v1.x * v2.y - v1.y * v2.x;

        return cross.z > 0;
    }

    bool collinear(float x, float y, float x0, float y0, float x1, float y1)
    {
        float y_int = (y0 * (x1 - x) + y1 * (x - x0)) / (x1 - x0);
        return fabs(y - y_int) < 0.5;
    }

    void SoftwareRendererImp::rasterize_triangle(float x0, float y0,
                                                 float x1, float y1,
                                                 float x2, float y2,
                                                 Color color)
    {
        // Task 1:
        // Implement triangle rasterization

        // check if it's counterclockwise, if so, swap points
        double ccw = is_counter_clockwise(x0, y0, x1, y1, x2, y2);
        if (!ccw)
        {
            float x_tmp = x0, y_tmp = y0;
            x0 = x2, y0 = y2;
            x2 = x_tmp, y2 = y_tmp;
        }

        // find the bounding boxes
        float min_x, min_y, max_x, max_y;
        min_x = min({x0, x1, x2});
        min_y = min({y0, y1, y2});
        max_x = max({x0, x1, x2});
        max_y = max({y0, y1, y2});

        Vector2D N1 = Vector2D((double)(y1 - y0), -(double)(x1 - x0));
        Vector2D N2 = Vector2D((double)(y2 - y1), -(double)(x2 - x1));
        Vector2D N3 = Vector2D((double)(y0 - y2), -(double)(x0 - x2));

        // the increment (or step size) depends on our sample rate!
        float increment = 1.f / (float)sample_rate;

        // draw for each pixel within the bounding box
        for (float y = floor(min_y); y < ceil(max_y) + increment; y+=increment)
        {
            for (float x = floor(min_x); x < ceil(max_x) + increment; x+=increment)
            {
                Vector2D V1 = Vector2D(x - x0, (y - y0));
                Vector2D V2 = Vector2D(x - x1, (y - y1));
                Vector2D V3 = Vector2D(x - x2, (y - y2));

                if ((N1.x * V1.x + N1.y * V1.y <= 0 &&
                     N2.x * V2.x + N2.y * V2.y <= 0 &&
                     N3.x * V3.x + N3.y * V3.y <= 0))
                {
                    // don't forget to scale by sample rate when populating the buffer matrix
                    fill_sample((int)(x * sample_rate), (int)(y * sample_rate), color);
                }
            }
        }
        // Advanced Task
        // Implementing Triangle Edge Rules
    }

    void SoftwareRendererImp::rasterize_image(float x0, float y0,
                                              float x1, float y1,
                                              Texture &tex)
    {
        // Task 4:
        // Implement image rasterization
        // printf("%f, %f, %f, %f\n", x0, y0, x1, y1);
        int level = 0;

        for (int y = y0 + 1; y < y1 + 1; y++)
        {
            for (int x = x0 + 1; x < x1 + 1; x++){
                float u = x / (x1 - x0) - x0 / (x1 - x0), v = y / (y1 - y0) - y0 / (y1 - y0);
                Color c = sampler->sample_bilinear(tex, u, v, level); //Color(1, 1, 0, 1); //
                fill_sample(x, y, c);
            }
        }
    }

    void apply_2x2_box_convolution_filter_at_coordinate(int x, int y, int sample_rate, int width, unsigned char *super_pixel_buffer, int *values)
    {
        for (int row = y; row < y + sample_rate; row += 1)
        {
            for (int column = x; column < x + sample_rate; column += 1)
            {
                values[0] += super_pixel_buffer[4 * (column + row * width)];
                values[1] += super_pixel_buffer[4 * (column + row * width) + 1];
                values[2] += super_pixel_buffer[4 * (column + row * width) + 2];
                values[3] += super_pixel_buffer[4 * (column + row * width) + 3];
            }
        }
        values[0] /= (sample_rate * sample_rate);
        values[1] /= (sample_rate * sample_rate);
        values[2] /= (sample_rate * sample_rate);
        values[3] /= (sample_rate * sample_rate);
    }

    // resolve samples to pixel buffer
    void SoftwareRendererImp::resolve(void)
    {

        printf("resolving super resolution image \n");

        int total_pixels = 0;

        // Task 2:
        auto supersample_width = width * sample_rate;

        // loop through the indices of the supersample buffer, step size is sample rate
        for (int row = 0; row < height * sample_rate; row += sample_rate)
        {
            for (int column = 0; column < supersample_width; column += sample_rate)
            {

                // compute the x and y of the pixel buffer to update
                int x = column / sample_rate, y = row / sample_rate;
                int rgba[4] = {0, 0, 0, 0};

                // apply average filtering
                apply_2x2_box_convolution_filter_at_coordinate(column, row, sample_rate, supersample_width, supersample_buffer, rgba);
                pixel_buffer[4 * (x + y * width)] = rgba[0];
                pixel_buffer[4 * (x + y * width) + 1] = rgba[1];
                pixel_buffer[4 * (x + y * width) + 2] = rgba[2];
                pixel_buffer[4 * (x + y * width) + 3] = rgba[3];

                total_pixels += 1;
            }
        }

        printf("%d, %zu, %zu | %zu, %zu, \n", total_pixels, height * sample_rate, supersample_width, height, width);

        // Implement supersampling
        // You may also need to modify other functions marked with "Task 2".
        return;
    }

    Color SoftwareRendererImp::alpha_blending(Color pixel_color, Color color)
    {
        // Task 5
        // Implement alpha compositing
        return pixel_color;
    }

} // namespace CS248
