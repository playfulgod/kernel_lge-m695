/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "mt9p017.h"

struct mt9p017_i2c_reg_conf const pll_tbl[] = {
	//{0x0103, 0x0001}, //111205 8Bit Change	
	//{0xFFFF, 0x000A}, //111205 DELAY=10
	{0x301A, 0x0018}, /* reset_register */
	{0x3064, 0xB800}, /* smia_test_2lane_mipi */  //111205 Low-temp
	{0x31AE, 0x0202}, /* dual_lane_MIPI_interface */
	{0x0300, 0x0005}, /* vt_pix_clk_div */
	{0x0302, 0x0001}, /* vt_sys_clk_div */
	{0x0304, 0x0002}, /* pre_pll_clk_div */
	{0x0306, 0x002D}, /* pll_multipler */
	{0x0308, 0x000A}, /* op_pix_clk_div */
	{0x030A, 0x0001},  /* op_sys_clk_div */

	{0xFFFF, 0x0001}  //Delay=1ms
};

struct mt9p017_i2c_reg_conf const init_tbl[] = {
	/* Sensor Rev4 Setting 110411 */
	{0x316A, 0x8400},
	{0x316C, 0x8400},
	{0x316E, 0x8400},
	{0x3EFA, 0x1B1F},
	{0x3ED2, 0xD965},
	{0x3ED8, 0x7F1B},
	{0x3EDA, 0xAF11},
	{0x3EE2, 0x0060},
	{0x3EF2, 0xD965},
	{0x3EF8, 0x797F},
	{0x3EFC, 0xA8EF},
	{0x3EFE, 0x1F0F},
	{0x31E0, 0x1F01},
	{0x3E00, 0x0429},
	{0x3E02, 0xFFFF},
	{0x3E04, 0xFFFF},
	{0x3E06, 0xFFFF},
	{0x3E08, 0x8071},
	{0x3E0A, 0x7281},
	{0x3E0C, 0x0043},
	{0x3E0E, 0x5313},
	{0x3E10, 0x0087},
	{0x3E12, 0x1060},
	{0x3E14, 0x8540},
	{0x3E16, 0xA200},
	{0x3E18, 0x1890},
	{0x3E1A, 0x57A0},
	{0x3E1C, 0x49A6},
	{0x3E1E, 0x4988},
	{0x3E20, 0x4681},
	{0x3E22, 0x4200},
	{0x3E24, 0x828B},
	{0x3E26, 0x499C},
	{0x3E28, 0x498E},
	{0x3E2A, 0x4788},
	{0x3E2C, 0x4D80},
	{0x3E2E, 0x100C},
	{0x3E30, 0x0406},
	{0x3E32, 0x9110},
	{0x3E34, 0x0C8C},
	{0x3E36, 0x4DB9},
	{0x3E38, 0x4A42},
	{0x3E3A, 0x8341},
	{0x3E3C, 0x814B},
	{0x3E3E, 0xB24B},
	{0x3E40, 0x8056},
	{0x3E42, 0x8000},
	{0x3E44, 0x1C81},
	{0x3E46, 0x10E0},
	{0x3E48, 0x8013},
	{0x3E4A, 0x001C},
	{0x3E4C, 0x0082},
	{0x3E4E, 0x7C09},
	{0x3E50, 0x7000},
	{0x3E52, 0x8082},
	{0x3E54, 0x7281},
	{0x3E56, 0x4C40},
	{0x3E58, 0x8E4D},
	{0x3E5A, 0x8110},
	{0x3E5C, 0x0CAF},
	{0x3E5E, 0x4D80},
	{0x3E60, 0x100C},
	{0x3E62, 0x8440},
	{0x3E64, 0x4C81},
	{0x3E66, 0x7C53},
	{0x3E68, 0x7000},
	{0x3E6A, 0x0000},
	{0x3E6C, 0x0000},
	{0x3E6E, 0x0000},
	{0x3E70, 0x0000},
	{0x3E72, 0x0000},
	{0x3E74, 0x0000},
	{0x3E76, 0x0000},
	{0x3E78, 0x0000},
	{0x3E7A, 0x0000},
	{0x3E7C, 0x0000},
	{0x3E7E, 0x0000},
	{0x3E80, 0x0000},
	{0x3E82, 0x0000},
	{0x3E84, 0x0000},
	{0x3E86, 0x0000},
	{0x3E88, 0x0000},
	{0x3E8A, 0x0000},
	{0x3E8C, 0x0000},
	{0x3E8E, 0x0000},
	{0x3E90, 0x0000},
	{0x3E92, 0x0000},
	{0x3E94, 0x0000},
	{0x3E96, 0x0000},
	{0x3E98, 0x0000},
	{0x3E9A, 0x0000},
	{0x3E9C, 0x0000},
	{0x3E9E, 0x0000},
	{0x3EA0, 0x0000},
	{0x3EA2, 0x0000},
	{0x3EA4, 0x0000},
	{0x3EA6, 0x0000},
	{0x3EA8, 0x0000},
	{0x3EAA, 0x0000},
	{0x3EAC, 0x0000},
	{0x3EAE, 0x0000},
	{0x3EB0, 0x0000},
	{0x3EB2, 0x0000},
	{0x3EB4, 0x0000},
	{0x3EB6, 0x0000},
	{0x3EB8, 0x0000},
	{0x3EBA, 0x0000},
	{0x3EBC, 0x0000},
	{0x3EBE, 0x0000},
	{0x3EC0, 0x0000},
	{0x3EC2, 0x0000},
	{0x3EC4, 0x0000},
	{0x3EC6, 0x0000},
	{0x3EC8, 0x0000},
	{0x3ECA, 0x0000},
	{0x3170, 0x2150},
	{0x317A, 0x0150},
	{0x3ECC, 0x2200},
	{0x3174, 0x0000},
	{0x3176, 0X0000}, 
	//MIPI timing settings
	{0x31B0, 0x00C4}, 
	{0x31B2, 0x0064},
	{0x31B4, 0x0E77},
	{0x31B6, 0x0D24},
	{0x31B8, 0x020E},
	{0x31BA, 0x0710},
	{0x31BC, 0x2A0D},
	{0x31BE, 0xC007},		//111205 MIPI continuous
};

/* Preview register settings */
struct mt9p017_i2c_reg_conf const mode_preview_tbl[] = {
	{0x3004, 0x0000}, /* x_addr_start */
	{0x3008, 0x0A25}, /* x_addr_end */
	{0x3002, 0x0000}, /* y_start_addr */
	{0x3006, 0x07A5}, /* y_addr_end */
	{0x3040, 0x04C3}, /* read_mode */
	{0x034C, 0x0514}, /* x_output_size */
	{0x034E, 0x03D4}, /* y_output_size */
	{0x300C, 0x0D4C}, /* line_length_pck */
	{0x300A, 0x0420}, /* frame_length_lines */
	{0x3012, 0x041F}, /* coarse_integration_time */
	{0x3014, 0x0A04}, /* fine_integration_time */
	{0x3010, 0x0184}  /* fine_correction */
};

/* Snapshot register settings */
struct mt9p017_i2c_reg_conf const mode_snapshot_tbl[] = {
	{0x3004, 0x0000}, /* x_addr_start */
	{0x3008, 0x0A2F}, /* x_addr_end */
	{0x3002, 0x0000}, /* y_start_addr */
	{0x3006, 0x07A7}, /* y_addr_end */
	{0x3040, 0x0041}, /* read_mode */
	{0x034C, 0x0A30}, /* x_output_size */
	{0x034E, 0x07A8}, /* y_output_size */
	{0x300C, 0x14A0}, /* line_length_pck */
	{0x300A, 0x07F8}, /* frame_length_lines */
//	{0x3012, 0x07F7}, /* coarse_integration_time */
	{0x3014, 0x12BE}, /* fine_integration_time */
	{0x3010, 0x00A0}, /* fine_correction */
};

struct mt9p017_i2c_reg_conf const lensrolloff_tbl[] = {
	{0x3600, 0x7C6F}, /* P_GR_P0Q0 */
	{0x3602, 0x054C}, /* P_GR_P0Q1 */
	{0x3604, 0x4EB0}, /* P_GR_P0Q2 */
	{0x3606, 0x3E2D}, /* P_GR_P0Q3 */
	{0x3608, 0xBA70}, /* P_GR_P0Q4 */
	{0x360A, 0x0010}, /* P_RD_P0Q0 */
	{0x360C, 0x95CE}, /* P_RD_P0Q1 */
	{0x360E, 0x4450}, /* P_RD_P0Q2 */
	{0x3610, 0x00EF}, /* P_RD_P0Q3 */
	{0x3612, 0xCA50}, /* P_RD_P0Q4 */
	{0x3614, 0x0150}, /* P_BL_P0Q0 */
	{0x3616, 0x484D}, /* P_BL_P0Q1 */
	{0x3618, 0x1010}, /* P_BL_P0Q2 */
	{0x361A, 0x938D}, /* P_BL_P0Q3 */
	{0x361C, 0x8450}, /* P_BL_P0Q4 */
	{0x361E, 0x0250}, /* P_GB_P0Q0 */
	{0x3620, 0xB10E}, /* P_GB_P0Q1 */
	{0x3622, 0x7710}, /* P_GB_P0Q2 */
	{0x3624, 0x480E}, /* P_GB_P0Q3 */
	{0x3626, 0x8511}, /* P_GB_P0Q4 */
	{0x3640, 0xB7AB}, /* P_GR_P1Q0 */
	{0x3642, 0xAA6D}, /* P_GR_P1Q1 */
	{0x3644, 0xE26E}, /* P_GR_P1Q2 */
	{0x3646, 0xA1AD}, /* P_GR_P1Q3 */
	{0x3648, 0x622F}, /* P_GR_P1Q4 */
	{0x364A, 0xBCCB}, /* P_RD_P1Q0 */
	{0x364C, 0x126D}, /* P_RD_P1Q1 */
	{0x364E, 0x06EE}, /* P_RD_P1Q2 */
	{0x3650, 0xAF0B}, /* P_RD_P1Q3 */
	{0x3652, 0xF06E}, /* P_RD_P1Q4 */
	{0x3654, 0x4AED}, /* P_BL_P1Q0 */
	{0x3656, 0x61CD}, /* P_BL_P1Q1 */
	{0x3658, 0x190F}, /* P_BL_P1Q2 */
	{0x365A, 0xEAED}, /* P_BL_P1Q3 */
	{0x365C, 0xA1B0}, /* P_BL_P1Q4 */
	{0x365E, 0x22AD}, /* P_GB_P1Q0 */
	{0x3660, 0xF72E}, /* P_GB_P1Q1 */
	{0x3662, 0x24B0}, /* P_GB_P1Q2 */
	{0x3664, 0x0E70}, /* P_GB_P1Q3 */
	{0x3666, 0xA691}, /* P_GB_P1Q4 */
	{0x3680, 0x5570}, /* P_GR_P2Q0 */
	{0x3682, 0x19CE}, /* P_GR_P2Q1 */
	{0x3684, 0x620F}, /* P_GR_P2Q2 */
	{0x3686, 0x2BF0}, /* P_GR_P2Q3 */
	{0x3688, 0x9DD3}, /* P_GR_P2Q4 */
	{0x368A, 0x6250}, /* P_RD_P2Q0 */
	{0x368C, 0xC16E}, /* P_RD_P2Q1 */
	{0x368E, 0x456B}, /* P_RD_P2Q2 */
	{0x3690, 0x0F50}, /* P_RD_P2Q3 */
	{0x3692, 0xD5B2}, /* P_RD_P2Q4 */
	{0x3694, 0x4C50}, /* P_BL_P2Q0 */
	{0x3696, 0x4D4E}, /* P_BL_P2Q1 */
	{0x3698, 0x9A6F}, /* P_BL_P2Q2 */
	{0x369A, 0x09D0}, /* P_BL_P2Q3 */
	{0x369C, 0xAD72}, /* P_BL_P2Q4 */
	{0x369E, 0x61F0}, /* P_GB_P2Q0 */
	{0x36A0, 0x824F}, /* P_GB_P2Q1 */
	{0x36A2, 0xC3B0}, /* P_GB_P2Q2 */
	{0x36A4, 0x7D70}, /* P_GB_P2Q3 */
	{0x36A6, 0x97D2}, /* P_GB_P2Q4 */
	{0x36C0, 0x35AF}, /* P_GR_P3Q0 */
	{0x36C2, 0x2A0F}, /* P_GR_P3Q1 */
	{0x36C4, 0xAFB0}, /* P_GR_P3Q2 */
	{0x36C6, 0xBBB0}, /* P_GR_P3Q3 */
	{0x36C8, 0x6D11}, /* P_GR_P3Q4 */
	{0x36CA, 0x25EF}, /* P_RD_P3Q0 */
	{0x36CC, 0x194F}, /* P_RD_P3Q1 */
	{0x36CE, 0x8A31}, /* P_RD_P3Q2 */
	{0x36D0, 0x94B1}, /* P_RD_P3Q3 */
	{0x36D2, 0x22F2}, /* P_RD_P3Q4 */
	{0x36D4, 0x7ECC}, /* P_BL_P3Q0 */
	{0x36D6, 0x0ACE}, /* P_BL_P3Q1 */
	{0x36D8, 0xC6D0}, /* P_BL_P3Q2 */
	{0x36DA, 0x82B0}, /* P_BL_P3Q3 */
	{0x36DC, 0x78F1}, /* P_BL_P3Q4 */
	{0x36DE, 0x004F}, /* P_GB_P3Q0 */
	{0x36E0, 0x1730}, /* P_GB_P3Q1 */
	{0x36E2, 0xF091}, /* P_GB_P3Q2 */
	{0x36E4, 0xFBF1}, /* P_GB_P3Q3 */
	{0x36E6, 0x7532}, /* P_GB_P3Q4 */
	{0x3700, 0x88D1}, /* P_GR_P4Q0 */
	{0x3702, 0xF22C}, /* P_GR_P4Q1 */
	{0x3704, 0xC4F3}, /* P_GR_P4Q2 */
	{0x3706, 0xCDF2}, /* P_GR_P4Q3 */
	{0x3708, 0x2AB5}, /* P_GR_P4Q4 */
	{0x370A, 0xD630}, /* P_RD_P4Q0 */
	{0x370C, 0x7CCE}, /* P_RD_P4Q1 */
	{0x370E, 0x9BB3}, /* P_RD_P4Q2 */
	{0x3710, 0x8332}, /* P_RD_P4Q3 */
	{0x3712, 0x0BB5}, /* P_RD_P4Q4 */
	{0x3714, 0xEA10}, /* P_BL_P4Q0 */
	{0x3716, 0x8AEF}, /* P_BL_P4Q1 */
	{0x3718, 0xCCF2}, /* P_BL_P4Q2 */
	{0x371A, 0xB752}, /* P_BL_P4Q3 */
	{0x371C, 0x0455}, /* P_BL_P4Q4 */
	{0x371E, 0x8DB1}, /* P_GB_P4Q0 */
	{0x3720, 0x28F0}, /* P_GB_P4Q1 */
	{0x3722, 0xEE12}, /* P_GB_P4Q2 */
	{0x3724, 0xBE32}, /* P_GB_P4Q3 */
	{0x3726, 0x0095}, /* P_GB_P4Q4 */
	{0x3782, 0x04B8}, /* POLY_ORIGIN_C */
	{0x3784, 0x0378}, /* POLY_ORIGIN_R */
	{0x37C0, 0x6089}, /* P_GR_Q5 */
	{0x37C2, 0x1FE9}, /* P_RD_Q5 */
	{0x37C4, 0x3648}, /* P_BL_Q5 */
	{0x37C6, 0x52A6}, /* P_GB_Q5 */
	{0x3780, 0x0000}  /* Poly_sc_enable */ /* change from 0x8000 */
};

struct mt9p017_reg mt9p017_regs = {
	.pll_tbl = &pll_tbl[0],
	.plltbl_size = ARRAY_SIZE(pll_tbl),

	.init_tbl = &init_tbl[0],
	.inittbl_size = ARRAY_SIZE(init_tbl),

	.prev_tbl = &mode_preview_tbl[0],
	.prevtbl_size = ARRAY_SIZE(mode_preview_tbl),

	.snap_tbl = &mode_snapshot_tbl[0],
	.snaptbl_size = ARRAY_SIZE(mode_snapshot_tbl),

	.lensroff_tbl = &lensrolloff_tbl[0],
	.lensroff_size = ARRAY_SIZE(lensrolloff_tbl),
};
