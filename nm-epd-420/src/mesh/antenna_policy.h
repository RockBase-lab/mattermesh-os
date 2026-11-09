#pragma once
namespace matter {
// External selection requires explicit confirmation. NM ships with the external
// antenna selected; it has no XL9555, so selection is persisted without an I2C switch.
#ifdef MM_BOARD_NM_EPD
constexpr bool EXTERNAL_ANTENNA_ENABLED = true;
constexpr bool DEFAULT_ANTENNA_INTERNAL = false;
#else
constexpr bool EXTERNAL_ANTENNA_ENABLED = true;
constexpr bool DEFAULT_ANTENNA_INTERNAL = true;
#endif
} // namespace matter
