#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CS248
{

    inline void uint8_to_float(float dst[4], unsigned char *src)
    {
        uint8_t *src_uint8 = (uint8_t *)src;
        dst[0] = src_uint8[0] / 255.f;
        dst[1] = src_uint8[1] / 255.f;
        dst[2] = src_uint8[2] / 255.f;
        dst[3] = src_uint8[3] / 255.f;
    }

    inline void float_to_uint8(unsigned char *dst, float src[4])
    {
        uint8_t *dst_uint8 = (uint8_t *)dst;
        dst_uint8[0] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[0])));
        dst_uint8[1] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[1])));
        dst_uint8[2] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[2])));
        dst_uint8[3] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[3])));
    }

    void Sampler2DImp::generate_mips(Texture &tex, int startLevel)
    {

        // NOTE:
        // This starter code allocates the mip levels and generates a level
        // map by filling each level with placeholder data in the form of a
        // color that differs from its neighbours'. You should instead fill
        // with the correct data!

        // Advanced Task
        // Implement mipmap for trilinear filtering

        // check start level
        if (startLevel >= tex.mipmap.size())
        {
            std::cerr << "Invalid start level";
        }

        // allocate sublevels
        int baseWidth = tex.mipmap[startLevel].width;
        int baseHeight = tex.mipmap[startLevel].height;
        int numSubLevels = (int)(log2f((float)max(baseWidth, baseHeight)));

        numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
        tex.mipmap.resize(startLevel + numSubLevels + 1);

        int width = baseWidth;
        int height = baseHeight;
        for (int i = 1; i <= numSubLevels; i++)
        {

            MipLevel &level = tex.mipmap[startLevel + i];

            // handle odd size texture by rounding down
            width = max(1, width / 2);
            assert(width > 0);
            height = max(1, height / 2);
            assert(height > 0);

            level.width = width;
            level.height = height;
            level.texels = vector<unsigned char>(4 * width * height);
        }

        // fill all 0 sub levels with interchanging colors
        Color colors[3] = {Color(1, 0, 0, 1), Color(0, 1, 0, 1), Color(0, 0, 1, 1)};
        for (size_t i = 1; i < tex.mipmap.size(); ++i)
        {

            Color c = colors[i % 3];
            MipLevel &mip = tex.mipmap[i];

            for (size_t i = 0; i < 4 * mip.width * mip.height; i += 4)
            {
                float_to_uint8(&mip.texels[i], &c.r);
            }
        }
        printf("confirming texture level size: %zu \n", tex.mipmap.size());
    }

    float lerp(float x, float v0, float v1)
    {
        return v0 + x * (v1 - v0);
    }

    Color Sampler2DImp::sample_nearest(Texture &tex,
                                       float u, float v,
                                       int level)
    {

        // Task 4: Implement nearest neighbour interpolation
        auto width = tex.mipmap[level].width;
        auto height = tex.mipmap[level].height - 1;

        int near_v = round(v * height), near_u = round(u * width);

        // return magenta for invalid level
        Color c;
        uint8_to_float(&c.r, &tex.mipmap[level].texels[4 * (near_v + width * near_u)]);
        return c;
    }

    Color Sampler2DImp::sample_bilinear(Texture &tex,
                                        float u, float v,
                                        int level)
    {

        // Task 4: Implement bilinear filtering

        // return magenta for invalid level
        if (level < 0 || level > tex.mipmap.size() - 1)
            return Color(1, 0, 1, 1);

        auto width = tex.mipmap[level].width;
        auto height = tex.mipmap[level].height;

        int u_x = round(u * width), v_y = round(v * height);
        float u_min = u_x - 0.5, v_min = v_y - 0.5;
        float u_max = u_x + 0.5, v_max = v_y + 0.5;

        // clamping
        u_min = (u_min < 0) ? u_max : u_min;
        u_max = (u_max > width) ? u_min : u_max;
        v_min = (v_min < 0) ? v_max : v_min;
        v_max = (v_max > height) ? v_min : v_max;

        // each half-integer sample point is stored at its coordinate -0.5 in texels
        u_min -=0.5; u_max -=0.5; v_min -=0.5; v_max -=0.5; 
        int u_00 = 4 * (v_max * (width) + u_min);
        int u_01 = 4 * (v_min * (width) + u_min);
        int u_10 = 4 * (v_max * (width) + u_max);
        int u_11 = 4 * (v_min * (width) + u_max);
        // in lerp, we need the half-integer coordinates back
        u_min +=0.5; u_max +=0.5; v_min +=0.5; v_max +=0.5; 

        unsigned char rgba[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++)
        {
            float u0 = lerp(u * width - u_min, tex.mipmap[level].texels[u_00 + i], tex.mipmap[level].texels[u_10 + i]);
            float u1 = lerp(u * width - u_min, tex.mipmap[level].texels[u_01 + i], tex.mipmap[level].texels[u_11 + i]);
            float t = v * height - v_min;
            rgba[i] = lerp(t, u1, u0);
        }

        return Color(rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f, rgba[3] / 255.0f);
    }

    Color Sampler2DImp::sample_trilinear(Texture &tex,
                                         float u, float v,
                                         float u_scale, float v_scale)
    {

        // Advanced Task
        // Implement trilinear filtering

        // return magenta for invalid level
        return Color(1, 0, 1, 1);
    }

} // namespace CS248
