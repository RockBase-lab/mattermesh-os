#pragma once
// Device identity shown to the mesh (manufacturer name), the CLI and the HTTP User-Agent of maps
// and weather.
#ifdef MM_BOARD_PLUS
#define MM_DEVICE_MODEL "T-Deck Plus"
#define MM_DEVICE_TOKEN "TDeckPlus"
#define MM_DEVICE_MANUFACTURER "LilyGO"
#elif defined(MM_BOARD_NM_EPD)
#define MM_DEVICE_MODEL "NM-EPD-420-BW"
#define MM_DEVICE_TOKEN "NmEpd420Bw"
#define MM_DEVICE_MANUFACTURER "RockBase"
#else
#define MM_DEVICE_MODEL "T-Deck Max"
#define MM_DEVICE_TOKEN "TDeckMax"
#define MM_DEVICE_MANUFACTURER "LilyGO"
#endif
