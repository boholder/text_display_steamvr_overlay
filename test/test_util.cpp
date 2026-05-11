#include <gtest/gtest.h>

#include "../src/utils.h"

namespace
{

TEST(UtilTest, ColorConvert)
{
    constexpr float F[4] = {0.1, 0.2, 0.4, 0.8};
    const uint32_t i = util::reverse_convert_color(F);
    float f[4];
    util::convert_color(i, f);
    ASSERT_NEAR(F[0], f[0], 0.01F);
    ASSERT_NEAR(F[1], f[1], 0.01F);
    ASSERT_NEAR(F[2], f[2], 0.01F);
    ASSERT_NEAR(F[3], f[3], 0.01F);

    constexpr uint32_t I = 0x10204080;
    util::convert_color(I, f);
    const uint32_t i2 = util::reverse_convert_color(f);
    ASSERT_EQ(I, i2);
}

} // namespace
