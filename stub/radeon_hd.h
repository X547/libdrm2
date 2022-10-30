#ifndef _RADEON_HD_H_
#define _RADEON_HD_H_

enum amd_asic_type {
	CHIP_TAHITI = 0,
	CHIP_PITCAIRN,	/* 1 */
	CHIP_VERDE,	/* 2 */
	CHIP_OLAND,	/* 3 */
	CHIP_HAINAN,	/* 4 */
	CHIP_BONAIRE,	/* 5 */
	CHIP_KAVERI,	/* 6 */
	CHIP_KABINI,	/* 7 */
	CHIP_HAWAII,	/* 8 */
	CHIP_MULLINS,	/* 9 */
	CHIP_TOPAZ,	/* 10 */
	CHIP_TONGA,	/* 11 */
	CHIP_FIJI,	/* 12 */
	CHIP_CARRIZO,	/* 13 */
	CHIP_STONEY,	/* 14 */
	CHIP_POLARIS10,	/* 15 */
	CHIP_POLARIS11,	/* 16 */
	CHIP_POLARIS12,	/* 17 */
	CHIP_VEGAM,	/* 18 */
	CHIP_VEGA10,	/* 19 */
	CHIP_VEGA12,	/* 20 */
	CHIP_VEGA20,	/* 21 */
	CHIP_RAVEN,	/* 22 */
	CHIP_ARCTURUS,	/* 23 */
	CHIP_RENOIR,	/* 24 */
	CHIP_NAVI10,	/* 25 */
	CHIP_NAVI14,	/* 26 */
	CHIP_NAVI12,	/* 27 */
	CHIP_LAST,
};

enum amd_chip_flags {
	AMD_ASIC_MASK = 0x0000ffffUL,
	AMD_FLAGS_MASK  = 0xffff0000UL,
	AMD_IS_MOBILITY = 0x00010000UL,
	AMD_IS_APU      = 0x00020000UL,
	AMD_IS_PX       = 0x00040000UL,
	AMD_EXP_HW_SUPPORT = 0x00080000UL,
};

#define AMDGPU_FAMILY_UNKNOWN			0
#define AMDGPU_FAMILY_SI			110 /* Hainan, Oland, Verde, Pitcairn, Tahiti */
#define AMDGPU_FAMILY_CI			120 /* Bonaire, Hawaii */
#define AMDGPU_FAMILY_KV			125 /* Kaveri, Kabini, Mullins */
#define AMDGPU_FAMILY_VI			130 /* Iceland, Tonga */
#define AMDGPU_FAMILY_CZ			135 /* Carrizo, Stoney */
#define AMDGPU_FAMILY_AI			141 /* Vega10 */
#define AMDGPU_FAMILY_RV			142 /* Raven */
#define AMDGPU_FAMILY_NV			143 /* Navi10 */

#endif	// _RADEON_HD_H_
