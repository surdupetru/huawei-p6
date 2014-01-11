/*
 * Copyright (c) 2011 Hisilicon Technologies Co., Ltd. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "k3v2_dcdc_gpu.h"
#if 1
struct gpu_profile_info gpu_profile_dcdc[] = {
	/*freq, gpu_clk_profile, gpu_vol_profile, dcdc_vol*/
	{57600, 0x00060C18, 0x00000223, 1000000},
	{120000, 0x0002C58B, 0x0000022A, 1000000},
	{240000, 0x00014285, 0x00000238, 1100000},
	{360000, 0x0000C183, 0x00000247, 1270000},
	{480000, 0x0000C102, 0x00000255, 1330000},
#ifdef WITH_G3D_600M_PROF
	{600000, 0x0000e0c1, 0x00000271, 1500000},
#endif
	{0, 0, 0, 0},
};
#endif

struct gpu_policy_info gpu_policy_powersave[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 90, 0},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 99, 50},
	{1, 1, 99, 60},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};

struct gpu_policy_info gpu_policy_normal[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 0},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 95, 80},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};

struct gpu_policy_info gpu_policy_performance[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 0},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 95, 80},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};

struct gpu_policy_info gpu_policy_special01[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 100, 12},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special02[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 100, 12},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special03[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 100, 12},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special04[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 80, 30},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special05[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 80, 30},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special06[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 80, 30},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special07[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 80, 30},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special08[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 80, 30},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special09[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 80, 30},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special0A[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 95, 30},
	{1, 1, 99, 40},
	{1, 1, 100, 50},
	{1, 1, 80, 30},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info gpu_policy_special0B[] = {
	/*uptimes, downtimes, up_threshold, down_threshold*/
	{1, 1, 75, 30},
	{1, 1, 80, 30},
	{1, 1, 80, 30},
	{1, 1, 80, 30},
	{1, 1, 23.75, 7.5},
#ifdef WITH_G3D_600M_PROF
	{1, 1, 100, 90},
#endif
	{0, 0, 0, 0},
};
struct gpu_policy_info *policy_table[] = {
	[NORMAL_POLICY] = gpu_policy_normal,
	[POWERSAVE_POLICY] = gpu_policy_powersave,
	[PERF_POLICY] = gpu_policy_performance,
	[SPEC01_POLICY] = gpu_policy_special01,
	[SPEC02_POLICY] = gpu_policy_special02,
	[SPEC03_POLICY] = gpu_policy_special03,
	[SPEC04_POLICY] = gpu_policy_special04,
	[SPEC05_POLICY] = gpu_policy_special05,
	[SPEC06_POLICY] = gpu_policy_special06,
	[SPEC07_POLICY] = gpu_policy_special07,
	[SPEC08_POLICY] = gpu_policy_special08,
	[SPEC09_POLICY] = gpu_policy_special09,
	[SPEC0A_POLICY] = gpu_policy_special0A,
	[SPEC0B_POLICY] = gpu_policy_special0B,
};

