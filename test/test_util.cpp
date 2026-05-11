#include <gtest/gtest.h>

#include "../src/utils.h"

namespace
{

TEST(UtilTest, ColorConvert)
{
    constexpr float F[4] = {0.1, 0.2, 0.4, 0.8};
    const uint32_t i = util::color_f4_to_u32(F);
    float f[4];
    util::color_u32_to_f4(i, f);
    ASSERT_NEAR(F[0], f[0], 0.01F);
    ASSERT_NEAR(F[1], f[1], 0.01F);
    ASSERT_NEAR(F[2], f[2], 0.01F);
    ASSERT_NEAR(F[3], f[3], 0.01F);

    constexpr uint32_t I = 0x10204080;
    util::color_u32_to_f4(I, f);
    const uint32_t i2 = util::color_f4_to_u32(f);
    ASSERT_EQ(I, i2);
}

TEST(UtilTest, RevertColorChannelOrder)
{
    constexpr uint32_t I = 0x10204080;
    const uint32_t i = util::revert_color_channel_order(I);
    ASSERT_EQ(0x80402010, i);
}

} // namespace
