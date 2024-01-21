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
        auto height = tex.mipmap[level].height;

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

        // find u_00, u_01, v_00, v_01
        int u_00 = (int)(u * width), u_01 = (int)(u * width + 1), v_00 = (int)(v * height), v_01 = (int)(v * height + 1);
        u *= width;
        v *= height;

        float rgba[4] = {0.f, 0.f, 0.f, 0.f};

        for (int i = 0; i < 4; i++) {

            // two horizontal helper interpolations
            float u0 = lerp(fabs(u - u_00), tex.mipmap[level].texels[4 * (u_00 + width * v_00) + i], tex.mipmap[level].texels[4 * (u_00 + width * v_01) + i]);
            float u1 = lerp(fabs(u - u_00), tex.mipmap[level].texels[4 * (u_01 + width * v_00) + i], tex.mipmap[level].texels[4 * (u_01 + width * v_01) + i]);

            // two horizontal helper interpolations
            float t = v_01 - v;
            rgba[i] = lerp(t, u0, u1);

            // if (tex.mipmap[level].texels[4 * (v_00 + width * u_00) + i] != tex.mipmap[level].texels[4 * (v_00 + width * u_01) + i])
            // {
            //     printf("%f, %f, %d\n", u, v, v_00);
            //     printf("%d, %d\n", tex.mipmap[level].texels[4 * (v_00 + width * u_00) + i], tex.mipmap[level].texels[4 * (v_00 + width * u_01) + i]);
            //     printf("%d, %d\n", tex.mipmap[level].texels[4 * (v_01 + width * u_00) + i], tex.mipmap[level].texels[4 * (v_01 + width * u_01) + i]);
            //     printf("%f, %f\n", u0, u1);
            //     printf("%f\n", rgba[i]);
            //     printf("--\n");
            //     exit(0);
            // }

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
