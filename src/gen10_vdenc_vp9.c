/*
 * Copyright © 2018 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Xiang Haihao <haihao.xiang@intel.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "intel_batchbuffer.h"
#include "i965_defines.h"
#include "i965_structs.h"
#include "i965_drv_video.h"
#include "i965_encoder.h"
#include "i965_encoder_utils.h"
#include "intel_media.h"
#include "i965_encoder_common.h"
#include "gen10_vdenc_common.h"
#include "gen10_hcp_common.h"
#include "gen10_huc_common.h"
#include "gen10_vdenc_vp9.h"

static const uint32_t gen10_vdenc_vp9_dys[][4] = {
#include "shaders/brc/cnl/vp9_dys.g10b"
};

static struct i965_kernel vdenc_vp9_kernels_dys[1] = {
    {
        "dys",
        0,
        gen10_vdenc_vp9_dys,
        sizeof(gen10_vdenc_vp9_dys),
        NULL
    },
};

static const uint32_t gen10_vdenc_vp9_streamin[][4] = {
#include "shaders/brc/cnl/vp9_vdenc_hme_vp9_streamin.g10b"
};

static struct i965_kernel vdenc_vp9_kernels_streamin[1] = {
    {
        "streamin",
        0,
        gen10_vdenc_vp9_streamin,
        sizeof(gen10_vdenc_vp9_streamin),
        NULL
    },
};

static const uint32_t vdenc_vp9_huc_prob_dmem_data[320] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000,
    0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
    0x00000004, 0x00000004, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x0000ff00, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000001,
    0x00540049, 0x00000060, 0x00000072, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x02b80078, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

static const uint8_t vdenc_vp9_key_frame_default_probs[2048] = {
    0x64, 0x42, 0x14, 0x98, 0x0f, 0x65, 0x03, 0x88, 0x25, 0x05, 0x34, 0x0d, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc3, 0x1d, 0xb7, 0x54, 0x31, 0x88, 0x08, 0x2a, 0x47, 0x1f, 0x6b, 0xa9, 0x23, 0x63, 0x9f, 0x11,
    0x52, 0x8c, 0x08, 0x42, 0x72, 0x02, 0x2c, 0x4c, 0x01, 0x13, 0x20, 0x28, 0x84, 0xc9, 0x1d, 0x72,
    0xbb, 0x0d, 0x5b, 0x9d, 0x07, 0x4b, 0x7f, 0x03, 0x3a, 0x5f, 0x01, 0x1c, 0x2f, 0x45, 0x8e, 0xdd,
    0x2a, 0x7a, 0xc9, 0x0f, 0x5b, 0x9f, 0x06, 0x43, 0x79, 0x01, 0x2a, 0x4d, 0x01, 0x11, 0x1f, 0x66,
    0x94, 0xe4, 0x43, 0x75, 0xcc, 0x11, 0x52, 0x9a, 0x06, 0x3b, 0x72, 0x02, 0x27, 0x4b, 0x01, 0x0f,
    0x1d, 0x9c, 0x39, 0xe9, 0x77, 0x39, 0xd4, 0x3a, 0x30, 0xa3, 0x1d, 0x28, 0x7c, 0x0c, 0x1e, 0x51,
    0x03, 0x0c, 0x1f, 0xbf, 0x6b, 0xe2, 0x7c, 0x75, 0xcc, 0x19, 0x63, 0x9b, 0x1d, 0x94, 0xd2, 0x25,
    0x7e, 0xc2, 0x08, 0x5d, 0x9d, 0x02, 0x44, 0x76, 0x01, 0x27, 0x45, 0x01, 0x11, 0x21, 0x29, 0x97,
    0xd5, 0x1b, 0x7b, 0xc1, 0x03, 0x52, 0x90, 0x01, 0x3a, 0x69, 0x01, 0x20, 0x3c, 0x01, 0x0d, 0x1a,
    0x3b, 0x9f, 0xdc, 0x17, 0x7e, 0xc6, 0x04, 0x58, 0x97, 0x01, 0x42, 0x72, 0x01, 0x26, 0x47, 0x01,
    0x12, 0x22, 0x72, 0x88, 0xe8, 0x33, 0x72, 0xcf, 0x0b, 0x53, 0x9b, 0x03, 0x38, 0x69, 0x01, 0x21,
    0x41, 0x01, 0x11, 0x22, 0x95, 0x41, 0xea, 0x79, 0x39, 0xd7, 0x3d, 0x31, 0xa6, 0x1c, 0x24, 0x72,
    0x0c, 0x19, 0x4c, 0x03, 0x10, 0x2a, 0xd6, 0x31, 0xdc, 0x84, 0x3f, 0xbc, 0x2a, 0x41, 0x89, 0x55,
    0x89, 0xdd, 0x68, 0x83, 0xd8, 0x31, 0x6f, 0xc0, 0x15, 0x57, 0x9b, 0x02, 0x31, 0x57, 0x01, 0x10,
    0x1c, 0x59, 0xa3, 0xe6, 0x5a, 0x89, 0xdc, 0x1d, 0x64, 0xb7, 0x0a, 0x46, 0x87, 0x02, 0x2a, 0x51,
    0x01, 0x11, 0x21, 0x6c, 0xa7, 0xed, 0x37, 0x85, 0xde, 0x0f, 0x61, 0xb3, 0x04, 0x48, 0x87, 0x01,
    0x2d, 0x55, 0x01, 0x13, 0x26, 0x7c, 0x92, 0xf0, 0x42, 0x7c, 0xe0, 0x11, 0x58, 0xaf, 0x04, 0x3a,
    0x7a, 0x01, 0x24, 0x4b, 0x01, 0x12, 0x25, 0x8d, 0x4f, 0xf1, 0x7e, 0x46, 0xe3, 0x42, 0x3a, 0xb6,
    0x1e, 0x2c, 0x88, 0x0c, 0x22, 0x60, 0x02, 0x14, 0x2f, 0xe5, 0x63, 0xf9, 0x8f, 0x6f, 0xeb, 0x2e,
    0x6d, 0xc0, 0x52, 0x9e, 0xec, 0x5e, 0x92, 0xe0, 0x19, 0x75, 0xbf, 0x09, 0x57, 0x95, 0x03, 0x38,
    0x63, 0x01, 0x21, 0x39, 0x53, 0xa7, 0xed, 0x44, 0x91, 0xde, 0x0a, 0x67, 0xb1, 0x02, 0x48, 0x83,
    0x01, 0x29, 0x4f, 0x01, 0x14, 0x27, 0x63, 0xa7, 0xef, 0x2f, 0x8d, 0xe0, 0x0a, 0x68, 0xb2, 0x02,
    0x49, 0x85, 0x01, 0x2c, 0x55, 0x01, 0x16, 0x2f, 0x7f, 0x91, 0xf3, 0x47, 0x81, 0xe4, 0x11, 0x5d,
    0xb1, 0x03, 0x3d, 0x7c, 0x01, 0x29, 0x54, 0x01, 0x15, 0x34, 0x9d, 0x4e, 0xf4, 0x8c, 0x48, 0xe7,
    0x45, 0x3a, 0xb8, 0x1f, 0x2c, 0x89, 0x0e, 0x26, 0x69, 0x08, 0x17, 0x3d, 0x7d, 0x22, 0xbb, 0x34,
    0x29, 0x85, 0x06, 0x1f, 0x38, 0x25, 0x6d, 0x99, 0x33, 0x66, 0x93, 0x17, 0x57, 0x80, 0x08, 0x43,
    0x65, 0x01, 0x29, 0x3f, 0x01, 0x13, 0x1d, 0x1f, 0x9a, 0xb9, 0x11, 0x7f, 0xaf, 0x06, 0x60, 0x91,
    0x02, 0x49, 0x72, 0x01, 0x33, 0x52, 0x01, 0x1c, 0x2d, 0x17, 0xa3, 0xc8, 0x0a, 0x83, 0xb9, 0x02,
    0x5d, 0x94, 0x01, 0x43, 0x6f, 0x01, 0x29, 0x45, 0x01, 0x0e, 0x18, 0x1d, 0xb0, 0xd9, 0x0c, 0x91,
    0xc9, 0x03, 0x65, 0x9c, 0x01, 0x45, 0x6f, 0x01, 0x27, 0x3f, 0x01, 0x0e, 0x17, 0x39, 0xc0, 0xe9,
    0x19, 0x9a, 0xd7, 0x06, 0x6d, 0xa7, 0x03, 0x4e, 0x76, 0x01, 0x30, 0x45, 0x01, 0x15, 0x1d, 0xca,
    0x69, 0xf5, 0x6c, 0x6a, 0xd8, 0x12, 0x5a, 0x90, 0x21, 0xac, 0xdb, 0x40, 0x95, 0xce, 0x0e, 0x75,
    0xb1, 0x05, 0x5a, 0x8d, 0x02, 0x3d, 0x5f, 0x01, 0x25, 0x39, 0x21, 0xb3, 0xdc, 0x0b, 0x8c, 0xc6,
    0x01, 0x59, 0x94, 0x01, 0x3c, 0x68, 0x01, 0x21, 0x39, 0x01, 0x0c, 0x15, 0x1e, 0xb5, 0xdd, 0x08,
    0x8d, 0xc6, 0x01, 0x57, 0x91, 0x01, 0x3a, 0x64, 0x01, 0x1f, 0x37, 0x01, 0x0c, 0x14, 0x20, 0xba,
    0xe0, 0x07, 0x8e, 0xc6, 0x01, 0x56, 0x8f, 0x01, 0x3a, 0x64, 0x01, 0x1f, 0x37, 0x01, 0x0c, 0x16,
    0x39, 0xc0, 0xe3, 0x14, 0x8f, 0xcc, 0x03, 0x60, 0x9a, 0x01, 0x44, 0x70, 0x01, 0x2a, 0x45, 0x01,
    0x13, 0x20, 0xd4, 0x23, 0xd7, 0x71, 0x2f, 0xa9, 0x1d, 0x30, 0x69, 0x4a, 0x81, 0xcb, 0x6a, 0x78,
    0xcb, 0x31, 0x6b, 0xb2, 0x13, 0x54, 0x90, 0x04, 0x32, 0x54, 0x01, 0x0f, 0x19, 0x47, 0xac, 0xd9,
    0x2c, 0x8d, 0xd1, 0x0f, 0x66, 0xad, 0x06, 0x4c, 0x85, 0x02, 0x33, 0x59, 0x01, 0x18, 0x2a, 0x40,
    0xb9, 0xe7, 0x1f, 0x94, 0xd8, 0x08, 0x67, 0xaf, 0x03, 0x4a, 0x83, 0x01, 0x2e, 0x51, 0x01, 0x12,
    0x1e, 0x41, 0xc4, 0xeb, 0x19, 0x9d, 0xdd, 0x05, 0x69, 0xae, 0x01, 0x43, 0x78, 0x01, 0x26, 0x45,
    0x01, 0x0f, 0x1e, 0x41, 0xcc, 0xee, 0x1e, 0x9c, 0xe0, 0x07, 0x6b, 0xb1, 0x02, 0x46, 0x7c, 0x01,
    0x2a, 0x49, 0x01, 0x12, 0x22, 0xe1, 0x56, 0xfb, 0x90, 0x68, 0xeb, 0x2a, 0x63, 0xb5, 0x55, 0xaf,
    0xef, 0x70, 0xa5, 0xe5, 0x1d, 0x88, 0xc8, 0x0c, 0x67, 0xa2, 0x06, 0x4d, 0x7b, 0x02, 0x35, 0x54,
    0x4b, 0xb7, 0xef, 0x1e, 0x9b, 0xdd, 0x03, 0x6a, 0xab, 0x01, 0x4a, 0x80, 0x01, 0x2c, 0x4c, 0x01,
    0x11, 0x1c, 0x49, 0xb9, 0xf0, 0x1b, 0x9f, 0xde, 0x02, 0x6b, 0xac, 0x01, 0x4b, 0x7f, 0x01, 0x2a,
    0x49, 0x01, 0x11, 0x1d, 0x3e, 0xbe, 0xee, 0x15, 0x9f, 0xde, 0x02, 0x6b, 0xac, 0x01, 0x48, 0x7a,
    0x01, 0x28, 0x47, 0x01, 0x12, 0x20, 0x3d, 0xc7, 0xf0, 0x1b, 0xa1, 0xe2, 0x04, 0x71, 0xb4, 0x01,
    0x4c, 0x81, 0x01, 0x2e, 0x50, 0x01, 0x17, 0x29, 0x07, 0x1b, 0x99, 0x05, 0x1e, 0x5f, 0x01, 0x10,
    0x1e, 0x32, 0x4b, 0x7f, 0x39, 0x4b, 0x7c, 0x1b, 0x43, 0x6c, 0x0a, 0x36, 0x56, 0x01, 0x21, 0x34,
    0x01, 0x0c, 0x12, 0x2b, 0x7d, 0x97, 0x1a, 0x6c, 0x94, 0x07, 0x53, 0x7a, 0x02, 0x3b, 0x59, 0x01,
    0x26, 0x3c, 0x01, 0x11, 0x1b, 0x17, 0x90, 0xa3, 0x0d, 0x70, 0x9a, 0x02, 0x4b, 0x75, 0x01, 0x32,
    0x51, 0x01, 0x1f, 0x33, 0x01, 0x0e, 0x17, 0x12, 0xa2, 0xb9, 0x06, 0x7b, 0xab, 0x01, 0x4e, 0x7d,
    0x01, 0x33, 0x56, 0x01, 0x1f, 0x36, 0x01, 0x0e, 0x17, 0x0f, 0xc7, 0xe3, 0x03, 0x96, 0xcc, 0x01,
    0x5b, 0x92, 0x01, 0x37, 0x5f, 0x01, 0x1e, 0x35, 0x01, 0x0b, 0x14, 0x13, 0x37, 0xf0, 0x13, 0x3b,
    0xc4, 0x03, 0x34, 0x69, 0x29, 0xa6, 0xcf, 0x68, 0x99, 0xc7, 0x1f, 0x7b, 0xb5, 0x0e, 0x65, 0x98,
    0x05, 0x48, 0x6a, 0x01, 0x24, 0x34, 0x23, 0xb0, 0xd3, 0x0c, 0x83, 0xbe, 0x02, 0x58, 0x90, 0x01,
    0x3c, 0x65, 0x01, 0x24, 0x3c, 0x01, 0x10, 0x1c, 0x1c, 0xb7, 0xd5, 0x08, 0x86, 0xbf, 0x01, 0x56,
    0x8e, 0x01, 0x38, 0x60, 0x01, 0x1e, 0x35, 0x01, 0x0c, 0x14, 0x14, 0xbe, 0xd7, 0x04, 0x87, 0xc0,
    0x01, 0x54, 0x8b, 0x01, 0x35, 0x5b, 0x01, 0x1c, 0x31, 0x01, 0x0b, 0x14, 0x0d, 0xc4, 0xd8, 0x02,
    0x89, 0xc0, 0x01, 0x56, 0x8f, 0x01, 0x39, 0x63, 0x01, 0x20, 0x38, 0x01, 0x0d, 0x18, 0xd3, 0x1d,
    0xd9, 0x60, 0x2f, 0x9c, 0x16, 0x2b, 0x57, 0x4e, 0x78, 0xc1, 0x6f, 0x74, 0xba, 0x2e, 0x66, 0xa4,
    0x0f, 0x50, 0x80, 0x02, 0x31, 0x4c, 0x01, 0x12, 0x1c, 0x47, 0xa1, 0xcb, 0x2a, 0x84, 0xc0, 0x0a,
    0x62, 0x96, 0x03, 0x45, 0x6d, 0x01, 0x2c, 0x46, 0x01, 0x12, 0x1d, 0x39, 0xba, 0xd3, 0x1e, 0x8c,
    0xc4, 0x04, 0x5d, 0x92, 0x01, 0x3e, 0x66, 0x01, 0x26, 0x41, 0x01, 0x10, 0x1b, 0x2f, 0xc7, 0xd9,
    0x0e, 0x91, 0xc4, 0x01, 0x58, 0x8e, 0x01, 0x39, 0x62, 0x01, 0x24, 0x3e, 0x01, 0x0f, 0x1a, 0x1a,
    0xdb, 0xe5, 0x05, 0x9b, 0xcf, 0x01, 0x5e, 0x97, 0x01, 0x3c, 0x68, 0x01, 0x24, 0x3e, 0x01, 0x10,
    0x1c, 0xe9, 0x1d, 0xf8, 0x92, 0x2f, 0xdc, 0x2b, 0x34, 0x8c, 0x64, 0xa3, 0xe8, 0xb3, 0xa1, 0xde,
    0x3f, 0x8e, 0xcc, 0x25, 0x71, 0xae, 0x1a, 0x59, 0x89, 0x12, 0x44, 0x61, 0x55, 0xb5, 0xe6, 0x20,
    0x92, 0xd1, 0x07, 0x64, 0xa4, 0x03, 0x47, 0x79, 0x01, 0x2d, 0x4d, 0x01, 0x12, 0x1e, 0x41, 0xbb,
    0xe6, 0x14, 0x94, 0xcf, 0x02, 0x61, 0x9f, 0x01, 0x44, 0x74, 0x01, 0x28, 0x46, 0x01, 0x0e, 0x1d,
    0x28, 0xc2, 0xe3, 0x08, 0x93, 0xcc, 0x01, 0x5e, 0x9b, 0x01, 0x41, 0x70, 0x01, 0x27, 0x42, 0x01,
    0x0e, 0x1a, 0x10, 0xd0, 0xe4, 0x03, 0x97, 0xcf, 0x01, 0x62, 0xa0, 0x01, 0x43, 0x75, 0x01, 0x29,
    0x4a, 0x01, 0x11, 0x1f, 0x11, 0x26, 0x8c, 0x07, 0x22, 0x50, 0x01, 0x11, 0x1d, 0x25, 0x4b, 0x80,
    0x29, 0x4c, 0x80, 0x1a, 0x42, 0x74, 0x0c, 0x34, 0x5e, 0x02, 0x20, 0x37, 0x01, 0x0a, 0x10, 0x32,
    0x7f, 0x9a, 0x25, 0x6d, 0x98, 0x10, 0x52, 0x79, 0x05, 0x3b, 0x55, 0x01, 0x23, 0x36, 0x01, 0x0d,
    0x14, 0x28, 0x8e, 0xa7, 0x11, 0x6e, 0x9d, 0x02, 0x47, 0x70, 0x01, 0x2c, 0x48, 0x01, 0x1b, 0x2d,
    0x01, 0x0b, 0x11, 0x1e, 0xaf, 0xbc, 0x09, 0x7c, 0xa9, 0x01, 0x4a, 0x74, 0x01, 0x30, 0x4e, 0x01,
    0x1e, 0x31, 0x01, 0x0b, 0x12, 0x0a, 0xde, 0xdf, 0x02, 0x96, 0xc2, 0x01, 0x53, 0x80, 0x01, 0x30,
    0x4f, 0x01, 0x1b, 0x2d, 0x01, 0x0b, 0x11, 0x24, 0x29, 0xeb, 0x1d, 0x24, 0xc1, 0x0a, 0x1b, 0x6f,
    0x55, 0xa5, 0xde, 0xb1, 0xa2, 0xd7, 0x6e, 0x87, 0xc3, 0x39, 0x71, 0xa8, 0x17, 0x53, 0x78, 0x0a,
    0x31, 0x3d, 0x55, 0xbe, 0xdf, 0x24, 0x8b, 0xc8, 0x05, 0x5a, 0x92, 0x01, 0x3c, 0x67, 0x01, 0x26,
    0x41, 0x01, 0x12, 0x1e, 0x48, 0xca, 0xdf, 0x17, 0x8d, 0xc7, 0x02, 0x56, 0x8c, 0x01, 0x38, 0x61,
    0x01, 0x24, 0x3d, 0x01, 0x10, 0x1b, 0x37, 0xda, 0xe1, 0x0d, 0x91, 0xc8, 0x01, 0x56, 0x8d, 0x01,
    0x39, 0x63, 0x01, 0x23, 0x3d, 0x01, 0x0d, 0x16, 0x0f, 0xeb, 0xd4, 0x01, 0x84, 0xb8, 0x01, 0x54,
    0x8b, 0x01, 0x39, 0x61, 0x01, 0x22, 0x38, 0x01, 0x0e, 0x17, 0xb5, 0x15, 0xc9, 0x3d, 0x25, 0x7b,
    0x0a, 0x26, 0x47, 0x2f, 0x6a, 0xac, 0x5f, 0x68, 0xad, 0x2a, 0x5d, 0x9f, 0x12, 0x4d, 0x83, 0x04,
    0x32, 0x51, 0x01, 0x11, 0x17, 0x3e, 0x93, 0xc7, 0x2c, 0x82, 0xbd, 0x1c, 0x66, 0x9a, 0x12, 0x4b,
    0x73, 0x02, 0x2c, 0x41, 0x01, 0x0c, 0x13, 0x37, 0x99, 0xd2, 0x18, 0x82, 0xc2, 0x03, 0x5d, 0x92,
    0x01, 0x3d, 0x61, 0x01, 0x1f, 0x32, 0x01, 0x0a, 0x10, 0x31, 0xba, 0xdf, 0x11, 0x94, 0xcc, 0x01,
    0x60, 0x8e, 0x01, 0x35, 0x53, 0x01, 0x1a, 0x2c, 0x01, 0x0b, 0x11, 0x0d, 0xd9, 0xd4, 0x02, 0x88,
    0xb4, 0x01, 0x4e, 0x7c, 0x01, 0x32, 0x53, 0x01, 0x1d, 0x31, 0x01, 0x0e, 0x17, 0xc5, 0x0d, 0xf7,
    0x52, 0x11, 0xde, 0x19, 0x11, 0xa2, 0x7e, 0xba, 0xf7, 0xea, 0xbf, 0xf3, 0xb0, 0xb1, 0xea, 0x68,
    0x9e, 0xdc, 0x42, 0x80, 0xba, 0x37, 0x5a, 0x89, 0x6f, 0xc5, 0xf2, 0x2e, 0x9e, 0xdb, 0x09, 0x68,
    0xab, 0x02, 0x41, 0x7d, 0x01, 0x2c, 0x50, 0x01, 0x11, 0x5b, 0x68, 0xd0, 0xf5, 0x27, 0xa8, 0xe0,
    0x03, 0x6d, 0xa2, 0x01, 0x4f, 0x7c, 0x01, 0x32, 0x66, 0x01, 0x2b, 0x66, 0x54, 0xdc, 0xf6, 0x1f,
    0xb1, 0xe7, 0x02, 0x73, 0xb4, 0x01, 0x4f, 0x86, 0x01, 0x37, 0x4d, 0x01, 0x3c, 0x4f, 0x2b, 0xf3,
    0xf0, 0x08, 0xb4, 0xd9, 0x01, 0x73, 0xa6, 0x01, 0x54, 0x79, 0x01, 0x33, 0x43, 0x01, 0x10, 0x06,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc0, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0x61, 0x5e, 0x5d,
    0x18, 0x63, 0x55, 0x77, 0x2c, 0x3e, 0x3b, 0x43, 0x95, 0x35, 0x35, 0x5e, 0x14, 0x30, 0x53, 0x35,
    0x18, 0x34, 0x12, 0x12, 0x96, 0x28, 0x27, 0x4e, 0x0c, 0x1a, 0x43, 0x21, 0x0b, 0x18, 0x07, 0x05,
    0xae, 0x23, 0x31, 0x44, 0x0b, 0x1b, 0x39, 0x0f, 0x09, 0x0c, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x90, 0x0b, 0x36, 0x9d, 0xc3, 0x82, 0x2e, 0x3a, 0x6c, 0x76, 0x0f, 0x7b, 0x94, 0x83, 0x65, 0x2c,
    0x5d, 0x83, 0x71, 0x0c, 0x17, 0xbc, 0xe2, 0x8e, 0x1a, 0x20, 0x7d, 0x78, 0x0b, 0x32, 0x7b, 0xa3,
    0x87, 0x40, 0x4d, 0x67, 0x71, 0x09, 0x24, 0x9b, 0x6f, 0x9d, 0x20, 0x2c, 0xa1, 0x74, 0x09, 0x37,
    0xb0, 0x4c, 0x60, 0x25, 0x3d, 0x95, 0x73, 0x09, 0x1c, 0x8d, 0xa1, 0xa7, 0x15, 0x19, 0xc1, 0x78,
    0x0c, 0x20, 0x91, 0xc3, 0x8e, 0x20, 0x26, 0x56, 0x74, 0x0c, 0x40, 0x78, 0x8c, 0x7d, 0x31, 0x73,
    0x79, 0x66, 0x13, 0x42, 0xa2, 0xb6, 0x7a, 0x23, 0x3b, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t vdenc_vp9_inter_default_probs[2048] = {
    0x64, 0x42, 0x14, 0x98, 0x0f, 0x65, 0x03, 0x88, 0x25, 0x05, 0x34, 0x0d, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc3, 0x1d, 0xb7, 0x54, 0x31, 0x88, 0x08, 0x2a, 0x47, 0x1f, 0x6b, 0xa9, 0x23, 0x63, 0x9f, 0x11,
    0x52, 0x8c, 0x08, 0x42, 0x72, 0x02, 0x2c, 0x4c, 0x01, 0x13, 0x20, 0x28, 0x84, 0xc9, 0x1d, 0x72,
    0xbb, 0x0d, 0x5b, 0x9d, 0x07, 0x4b, 0x7f, 0x03, 0x3a, 0x5f, 0x01, 0x1c, 0x2f, 0x45, 0x8e, 0xdd,
    0x2a, 0x7a, 0xc9, 0x0f, 0x5b, 0x9f, 0x06, 0x43, 0x79, 0x01, 0x2a, 0x4d, 0x01, 0x11, 0x1f, 0x66,
    0x94, 0xe4, 0x43, 0x75, 0xcc, 0x11, 0x52, 0x9a, 0x06, 0x3b, 0x72, 0x02, 0x27, 0x4b, 0x01, 0x0f,
    0x1d, 0x9c, 0x39, 0xe9, 0x77, 0x39, 0xd4, 0x3a, 0x30, 0xa3, 0x1d, 0x28, 0x7c, 0x0c, 0x1e, 0x51,
    0x03, 0x0c, 0x1f, 0xbf, 0x6b, 0xe2, 0x7c, 0x75, 0xcc, 0x19, 0x63, 0x9b, 0x1d, 0x94, 0xd2, 0x25,
    0x7e, 0xc2, 0x08, 0x5d, 0x9d, 0x02, 0x44, 0x76, 0x01, 0x27, 0x45, 0x01, 0x11, 0x21, 0x29, 0x97,
    0xd5, 0x1b, 0x7b, 0xc1, 0x03, 0x52, 0x90, 0x01, 0x3a, 0x69, 0x01, 0x20, 0x3c, 0x01, 0x0d, 0x1a,
    0x3b, 0x9f, 0xdc, 0x17, 0x7e, 0xc6, 0x04, 0x58, 0x97, 0x01, 0x42, 0x72, 0x01, 0x26, 0x47, 0x01,
    0x12, 0x22, 0x72, 0x88, 0xe8, 0x33, 0x72, 0xcf, 0x0b, 0x53, 0x9b, 0x03, 0x38, 0x69, 0x01, 0x21,
    0x41, 0x01, 0x11, 0x22, 0x95, 0x41, 0xea, 0x79, 0x39, 0xd7, 0x3d, 0x31, 0xa6, 0x1c, 0x24, 0x72,
    0x0c, 0x19, 0x4c, 0x03, 0x10, 0x2a, 0xd6, 0x31, 0xdc, 0x84, 0x3f, 0xbc, 0x2a, 0x41, 0x89, 0x55,
    0x89, 0xdd, 0x68, 0x83, 0xd8, 0x31, 0x6f, 0xc0, 0x15, 0x57, 0x9b, 0x02, 0x31, 0x57, 0x01, 0x10,
    0x1c, 0x59, 0xa3, 0xe6, 0x5a, 0x89, 0xdc, 0x1d, 0x64, 0xb7, 0x0a, 0x46, 0x87, 0x02, 0x2a, 0x51,
    0x01, 0x11, 0x21, 0x6c, 0xa7, 0xed, 0x37, 0x85, 0xde, 0x0f, 0x61, 0xb3, 0x04, 0x48, 0x87, 0x01,
    0x2d, 0x55, 0x01, 0x13, 0x26, 0x7c, 0x92, 0xf0, 0x42, 0x7c, 0xe0, 0x11, 0x58, 0xaf, 0x04, 0x3a,
    0x7a, 0x01, 0x24, 0x4b, 0x01, 0x12, 0x25, 0x8d, 0x4f, 0xf1, 0x7e, 0x46, 0xe3, 0x42, 0x3a, 0xb6,
    0x1e, 0x2c, 0x88, 0x0c, 0x22, 0x60, 0x02, 0x14, 0x2f, 0xe5, 0x63, 0xf9, 0x8f, 0x6f, 0xeb, 0x2e,
    0x6d, 0xc0, 0x52, 0x9e, 0xec, 0x5e, 0x92, 0xe0, 0x19, 0x75, 0xbf, 0x09, 0x57, 0x95, 0x03, 0x38,
    0x63, 0x01, 0x21, 0x39, 0x53, 0xa7, 0xed, 0x44, 0x91, 0xde, 0x0a, 0x67, 0xb1, 0x02, 0x48, 0x83,
    0x01, 0x29, 0x4f, 0x01, 0x14, 0x27, 0x63, 0xa7, 0xef, 0x2f, 0x8d, 0xe0, 0x0a, 0x68, 0xb2, 0x02,
    0x49, 0x85, 0x01, 0x2c, 0x55, 0x01, 0x16, 0x2f, 0x7f, 0x91, 0xf3, 0x47, 0x81, 0xe4, 0x11, 0x5d,
    0xb1, 0x03, 0x3d, 0x7c, 0x01, 0x29, 0x54, 0x01, 0x15, 0x34, 0x9d, 0x4e, 0xf4, 0x8c, 0x48, 0xe7,
    0x45, 0x3a, 0xb8, 0x1f, 0x2c, 0x89, 0x0e, 0x26, 0x69, 0x08, 0x17, 0x3d, 0x7d, 0x22, 0xbb, 0x34,
    0x29, 0x85, 0x06, 0x1f, 0x38, 0x25, 0x6d, 0x99, 0x33, 0x66, 0x93, 0x17, 0x57, 0x80, 0x08, 0x43,
    0x65, 0x01, 0x29, 0x3f, 0x01, 0x13, 0x1d, 0x1f, 0x9a, 0xb9, 0x11, 0x7f, 0xaf, 0x06, 0x60, 0x91,
    0x02, 0x49, 0x72, 0x01, 0x33, 0x52, 0x01, 0x1c, 0x2d, 0x17, 0xa3, 0xc8, 0x0a, 0x83, 0xb9, 0x02,
    0x5d, 0x94, 0x01, 0x43, 0x6f, 0x01, 0x29, 0x45, 0x01, 0x0e, 0x18, 0x1d, 0xb0, 0xd9, 0x0c, 0x91,
    0xc9, 0x03, 0x65, 0x9c, 0x01, 0x45, 0x6f, 0x01, 0x27, 0x3f, 0x01, 0x0e, 0x17, 0x39, 0xc0, 0xe9,
    0x19, 0x9a, 0xd7, 0x06, 0x6d, 0xa7, 0x03, 0x4e, 0x76, 0x01, 0x30, 0x45, 0x01, 0x15, 0x1d, 0xca,
    0x69, 0xf5, 0x6c, 0x6a, 0xd8, 0x12, 0x5a, 0x90, 0x21, 0xac, 0xdb, 0x40, 0x95, 0xce, 0x0e, 0x75,
    0xb1, 0x05, 0x5a, 0x8d, 0x02, 0x3d, 0x5f, 0x01, 0x25, 0x39, 0x21, 0xb3, 0xdc, 0x0b, 0x8c, 0xc6,
    0x01, 0x59, 0x94, 0x01, 0x3c, 0x68, 0x01, 0x21, 0x39, 0x01, 0x0c, 0x15, 0x1e, 0xb5, 0xdd, 0x08,
    0x8d, 0xc6, 0x01, 0x57, 0x91, 0x01, 0x3a, 0x64, 0x01, 0x1f, 0x37, 0x01, 0x0c, 0x14, 0x20, 0xba,
    0xe0, 0x07, 0x8e, 0xc6, 0x01, 0x56, 0x8f, 0x01, 0x3a, 0x64, 0x01, 0x1f, 0x37, 0x01, 0x0c, 0x16,
    0x39, 0xc0, 0xe3, 0x14, 0x8f, 0xcc, 0x03, 0x60, 0x9a, 0x01, 0x44, 0x70, 0x01, 0x2a, 0x45, 0x01,
    0x13, 0x20, 0xd4, 0x23, 0xd7, 0x71, 0x2f, 0xa9, 0x1d, 0x30, 0x69, 0x4a, 0x81, 0xcb, 0x6a, 0x78,
    0xcb, 0x31, 0x6b, 0xb2, 0x13, 0x54, 0x90, 0x04, 0x32, 0x54, 0x01, 0x0f, 0x19, 0x47, 0xac, 0xd9,
    0x2c, 0x8d, 0xd1, 0x0f, 0x66, 0xad, 0x06, 0x4c, 0x85, 0x02, 0x33, 0x59, 0x01, 0x18, 0x2a, 0x40,
    0xb9, 0xe7, 0x1f, 0x94, 0xd8, 0x08, 0x67, 0xaf, 0x03, 0x4a, 0x83, 0x01, 0x2e, 0x51, 0x01, 0x12,
    0x1e, 0x41, 0xc4, 0xeb, 0x19, 0x9d, 0xdd, 0x05, 0x69, 0xae, 0x01, 0x43, 0x78, 0x01, 0x26, 0x45,
    0x01, 0x0f, 0x1e, 0x41, 0xcc, 0xee, 0x1e, 0x9c, 0xe0, 0x07, 0x6b, 0xb1, 0x02, 0x46, 0x7c, 0x01,
    0x2a, 0x49, 0x01, 0x12, 0x22, 0xe1, 0x56, 0xfb, 0x90, 0x68, 0xeb, 0x2a, 0x63, 0xb5, 0x55, 0xaf,
    0xef, 0x70, 0xa5, 0xe5, 0x1d, 0x88, 0xc8, 0x0c, 0x67, 0xa2, 0x06, 0x4d, 0x7b, 0x02, 0x35, 0x54,
    0x4b, 0xb7, 0xef, 0x1e, 0x9b, 0xdd, 0x03, 0x6a, 0xab, 0x01, 0x4a, 0x80, 0x01, 0x2c, 0x4c, 0x01,
    0x11, 0x1c, 0x49, 0xb9, 0xf0, 0x1b, 0x9f, 0xde, 0x02, 0x6b, 0xac, 0x01, 0x4b, 0x7f, 0x01, 0x2a,
    0x49, 0x01, 0x11, 0x1d, 0x3e, 0xbe, 0xee, 0x15, 0x9f, 0xde, 0x02, 0x6b, 0xac, 0x01, 0x48, 0x7a,
    0x01, 0x28, 0x47, 0x01, 0x12, 0x20, 0x3d, 0xc7, 0xf0, 0x1b, 0xa1, 0xe2, 0x04, 0x71, 0xb4, 0x01,
    0x4c, 0x81, 0x01, 0x2e, 0x50, 0x01, 0x17, 0x29, 0x07, 0x1b, 0x99, 0x05, 0x1e, 0x5f, 0x01, 0x10,
    0x1e, 0x32, 0x4b, 0x7f, 0x39, 0x4b, 0x7c, 0x1b, 0x43, 0x6c, 0x0a, 0x36, 0x56, 0x01, 0x21, 0x34,
    0x01, 0x0c, 0x12, 0x2b, 0x7d, 0x97, 0x1a, 0x6c, 0x94, 0x07, 0x53, 0x7a, 0x02, 0x3b, 0x59, 0x01,
    0x26, 0x3c, 0x01, 0x11, 0x1b, 0x17, 0x90, 0xa3, 0x0d, 0x70, 0x9a, 0x02, 0x4b, 0x75, 0x01, 0x32,
    0x51, 0x01, 0x1f, 0x33, 0x01, 0x0e, 0x17, 0x12, 0xa2, 0xb9, 0x06, 0x7b, 0xab, 0x01, 0x4e, 0x7d,
    0x01, 0x33, 0x56, 0x01, 0x1f, 0x36, 0x01, 0x0e, 0x17, 0x0f, 0xc7, 0xe3, 0x03, 0x96, 0xcc, 0x01,
    0x5b, 0x92, 0x01, 0x37, 0x5f, 0x01, 0x1e, 0x35, 0x01, 0x0b, 0x14, 0x13, 0x37, 0xf0, 0x13, 0x3b,
    0xc4, 0x03, 0x34, 0x69, 0x29, 0xa6, 0xcf, 0x68, 0x99, 0xc7, 0x1f, 0x7b, 0xb5, 0x0e, 0x65, 0x98,
    0x05, 0x48, 0x6a, 0x01, 0x24, 0x34, 0x23, 0xb0, 0xd3, 0x0c, 0x83, 0xbe, 0x02, 0x58, 0x90, 0x01,
    0x3c, 0x65, 0x01, 0x24, 0x3c, 0x01, 0x10, 0x1c, 0x1c, 0xb7, 0xd5, 0x08, 0x86, 0xbf, 0x01, 0x56,
    0x8e, 0x01, 0x38, 0x60, 0x01, 0x1e, 0x35, 0x01, 0x0c, 0x14, 0x14, 0xbe, 0xd7, 0x04, 0x87, 0xc0,
    0x01, 0x54, 0x8b, 0x01, 0x35, 0x5b, 0x01, 0x1c, 0x31, 0x01, 0x0b, 0x14, 0x0d, 0xc4, 0xd8, 0x02,
    0x89, 0xc0, 0x01, 0x56, 0x8f, 0x01, 0x39, 0x63, 0x01, 0x20, 0x38, 0x01, 0x0d, 0x18, 0xd3, 0x1d,
    0xd9, 0x60, 0x2f, 0x9c, 0x16, 0x2b, 0x57, 0x4e, 0x78, 0xc1, 0x6f, 0x74, 0xba, 0x2e, 0x66, 0xa4,
    0x0f, 0x50, 0x80, 0x02, 0x31, 0x4c, 0x01, 0x12, 0x1c, 0x47, 0xa1, 0xcb, 0x2a, 0x84, 0xc0, 0x0a,
    0x62, 0x96, 0x03, 0x45, 0x6d, 0x01, 0x2c, 0x46, 0x01, 0x12, 0x1d, 0x39, 0xba, 0xd3, 0x1e, 0x8c,
    0xc4, 0x04, 0x5d, 0x92, 0x01, 0x3e, 0x66, 0x01, 0x26, 0x41, 0x01, 0x10, 0x1b, 0x2f, 0xc7, 0xd9,
    0x0e, 0x91, 0xc4, 0x01, 0x58, 0x8e, 0x01, 0x39, 0x62, 0x01, 0x24, 0x3e, 0x01, 0x0f, 0x1a, 0x1a,
    0xdb, 0xe5, 0x05, 0x9b, 0xcf, 0x01, 0x5e, 0x97, 0x01, 0x3c, 0x68, 0x01, 0x24, 0x3e, 0x01, 0x10,
    0x1c, 0xe9, 0x1d, 0xf8, 0x92, 0x2f, 0xdc, 0x2b, 0x34, 0x8c, 0x64, 0xa3, 0xe8, 0xb3, 0xa1, 0xde,
    0x3f, 0x8e, 0xcc, 0x25, 0x71, 0xae, 0x1a, 0x59, 0x89, 0x12, 0x44, 0x61, 0x55, 0xb5, 0xe6, 0x20,
    0x92, 0xd1, 0x07, 0x64, 0xa4, 0x03, 0x47, 0x79, 0x01, 0x2d, 0x4d, 0x01, 0x12, 0x1e, 0x41, 0xbb,
    0xe6, 0x14, 0x94, 0xcf, 0x02, 0x61, 0x9f, 0x01, 0x44, 0x74, 0x01, 0x28, 0x46, 0x01, 0x0e, 0x1d,
    0x28, 0xc2, 0xe3, 0x08, 0x93, 0xcc, 0x01, 0x5e, 0x9b, 0x01, 0x41, 0x70, 0x01, 0x27, 0x42, 0x01,
    0x0e, 0x1a, 0x10, 0xd0, 0xe4, 0x03, 0x97, 0xcf, 0x01, 0x62, 0xa0, 0x01, 0x43, 0x75, 0x01, 0x29,
    0x4a, 0x01, 0x11, 0x1f, 0x11, 0x26, 0x8c, 0x07, 0x22, 0x50, 0x01, 0x11, 0x1d, 0x25, 0x4b, 0x80,
    0x29, 0x4c, 0x80, 0x1a, 0x42, 0x74, 0x0c, 0x34, 0x5e, 0x02, 0x20, 0x37, 0x01, 0x0a, 0x10, 0x32,
    0x7f, 0x9a, 0x25, 0x6d, 0x98, 0x10, 0x52, 0x79, 0x05, 0x3b, 0x55, 0x01, 0x23, 0x36, 0x01, 0x0d,
    0x14, 0x28, 0x8e, 0xa7, 0x11, 0x6e, 0x9d, 0x02, 0x47, 0x70, 0x01, 0x2c, 0x48, 0x01, 0x1b, 0x2d,
    0x01, 0x0b, 0x11, 0x1e, 0xaf, 0xbc, 0x09, 0x7c, 0xa9, 0x01, 0x4a, 0x74, 0x01, 0x30, 0x4e, 0x01,
    0x1e, 0x31, 0x01, 0x0b, 0x12, 0x0a, 0xde, 0xdf, 0x02, 0x96, 0xc2, 0x01, 0x53, 0x80, 0x01, 0x30,
    0x4f, 0x01, 0x1b, 0x2d, 0x01, 0x0b, 0x11, 0x24, 0x29, 0xeb, 0x1d, 0x24, 0xc1, 0x0a, 0x1b, 0x6f,
    0x55, 0xa5, 0xde, 0xb1, 0xa2, 0xd7, 0x6e, 0x87, 0xc3, 0x39, 0x71, 0xa8, 0x17, 0x53, 0x78, 0x0a,
    0x31, 0x3d, 0x55, 0xbe, 0xdf, 0x24, 0x8b, 0xc8, 0x05, 0x5a, 0x92, 0x01, 0x3c, 0x67, 0x01, 0x26,
    0x41, 0x01, 0x12, 0x1e, 0x48, 0xca, 0xdf, 0x17, 0x8d, 0xc7, 0x02, 0x56, 0x8c, 0x01, 0x38, 0x61,
    0x01, 0x24, 0x3d, 0x01, 0x10, 0x1b, 0x37, 0xda, 0xe1, 0x0d, 0x91, 0xc8, 0x01, 0x56, 0x8d, 0x01,
    0x39, 0x63, 0x01, 0x23, 0x3d, 0x01, 0x0d, 0x16, 0x0f, 0xeb, 0xd4, 0x01, 0x84, 0xb8, 0x01, 0x54,
    0x8b, 0x01, 0x39, 0x61, 0x01, 0x22, 0x38, 0x01, 0x0e, 0x17, 0xb5, 0x15, 0xc9, 0x3d, 0x25, 0x7b,
    0x0a, 0x26, 0x47, 0x2f, 0x6a, 0xac, 0x5f, 0x68, 0xad, 0x2a, 0x5d, 0x9f, 0x12, 0x4d, 0x83, 0x04,
    0x32, 0x51, 0x01, 0x11, 0x17, 0x3e, 0x93, 0xc7, 0x2c, 0x82, 0xbd, 0x1c, 0x66, 0x9a, 0x12, 0x4b,
    0x73, 0x02, 0x2c, 0x41, 0x01, 0x0c, 0x13, 0x37, 0x99, 0xd2, 0x18, 0x82, 0xc2, 0x03, 0x5d, 0x92,
    0x01, 0x3d, 0x61, 0x01, 0x1f, 0x32, 0x01, 0x0a, 0x10, 0x31, 0xba, 0xdf, 0x11, 0x94, 0xcc, 0x01,
    0x60, 0x8e, 0x01, 0x35, 0x53, 0x01, 0x1a, 0x2c, 0x01, 0x0b, 0x11, 0x0d, 0xd9, 0xd4, 0x02, 0x88,
    0xb4, 0x01, 0x4e, 0x7c, 0x01, 0x32, 0x53, 0x01, 0x1d, 0x31, 0x01, 0x0e, 0x17, 0xc5, 0x0d, 0xf7,
    0x52, 0x11, 0xde, 0x19, 0x11, 0xa2, 0x7e, 0xba, 0xf7, 0xea, 0xbf, 0xf3, 0xb0, 0xb1, 0xea, 0x68,
    0x9e, 0xdc, 0x42, 0x80, 0xba, 0x37, 0x5a, 0x89, 0x6f, 0xc5, 0xf2, 0x2e, 0x9e, 0xdb, 0x09, 0x68,
    0xab, 0x02, 0x41, 0x7d, 0x01, 0x2c, 0x50, 0x01, 0x11, 0x5b, 0x68, 0xd0, 0xf5, 0x27, 0xa8, 0xe0,
    0x03, 0x6d, 0xa2, 0x01, 0x4f, 0x7c, 0x01, 0x32, 0x66, 0x01, 0x2b, 0x66, 0x54, 0xdc, 0xf6, 0x1f,
    0xb1, 0xe7, 0x02, 0x73, 0xb4, 0x01, 0x4f, 0x86, 0x01, 0x37, 0x4d, 0x01, 0x3c, 0x4f, 0x2b, 0xf3,
    0xf0, 0x08, 0xb4, 0xd9, 0x01, 0x73, 0xa6, 0x01, 0x54, 0x79, 0x01, 0x33, 0x43, 0x01, 0x10, 0x06,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc0, 0x80, 0x40, 0x02, 0xad, 0x22, 0x07, 0x91, 0x55, 0x07, 0xa6, 0x3f, 0x07, 0x5e, 0x42, 0x08,
    0x40, 0x2e, 0x11, 0x51, 0x1f, 0x19, 0x1d, 0x1e, 0xeb, 0xa2, 0x24, 0xff, 0x22, 0x03, 0x95, 0x90,
    0x09, 0x66, 0xbb, 0xe1, 0xef, 0xb7, 0x77, 0x60, 0x29, 0x21, 0x10, 0x4d, 0x4a, 0x8e, 0x8e, 0xac,
    0xaa, 0xee, 0xf7, 0x32, 0x7e, 0x7b, 0xdd, 0xe2, 0x41, 0x20, 0x12, 0x90, 0xa2, 0xc2, 0x29, 0x33,
    0x62, 0x84, 0x44, 0x12, 0xa5, 0xd9, 0xc4, 0x2d, 0x28, 0x4e, 0xad, 0x50, 0x13, 0xb0, 0xf0, 0xc1,
    0x40, 0x23, 0x2e, 0xdd, 0x87, 0x26, 0xc2, 0xf8, 0x79, 0x60, 0x55, 0x1d, 0xc7, 0x7a, 0x8d, 0x93,
    0x3f, 0x9f, 0x94, 0x85, 0x76, 0x79, 0x68, 0x72, 0xae, 0x49, 0x57, 0x5c, 0x29, 0x53, 0x52, 0x63,
    0x32, 0x35, 0x27, 0x27, 0xb1, 0x3a, 0x3b, 0x44, 0x1a, 0x3f, 0x34, 0x4f, 0x19, 0x11, 0x0e, 0x0c,
    0xde, 0x22, 0x1e, 0x48, 0x10, 0x2c, 0x3a, 0x20, 0x0c, 0x0a, 0x07, 0x06, 0x20, 0x40, 0x60, 0x80,
    0xe0, 0x90, 0xc0, 0xa8, 0xc0, 0xb0, 0xc0, 0xc6, 0xc6, 0xf5, 0xd8, 0x88, 0x8c, 0x94, 0xa0, 0xb0,
    0xc0, 0xe0, 0xea, 0xea, 0xf0, 0x80, 0xd8, 0x80, 0xb0, 0xa0, 0xb0, 0xb0, 0xc0, 0xc6, 0xc6, 0xd0,
    0xd0, 0x88, 0x8c, 0x94, 0xa0, 0xb0, 0xc0, 0xe0, 0xea, 0xea, 0xf0, 0x80, 0x80, 0x40, 0x60, 0x70,
    0x40, 0x40, 0x60, 0x40, 0x80, 0x80, 0x40, 0x60, 0x70, 0x40, 0x40, 0x60, 0x40, 0xa0, 0x80, 0xa0,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x07, 0x4c, 0xb0, 0xd0, 0x7e, 0x1c, 0x36, 0x67, 0x30, 0x0c, 0x9a, 0x9b, 0x8b, 0x5a, 0x22,
    0x75, 0x77, 0x43, 0x06, 0x19, 0xcc, 0xf3, 0x9e, 0x0d, 0x15, 0x60, 0x61, 0x05, 0x2c, 0x83, 0xb0,
    0x8b, 0x30, 0x44, 0x61, 0x53, 0x05, 0x2a, 0x9c, 0x6f, 0x98, 0x1a, 0x31, 0x98, 0x50, 0x05, 0x3a,
    0xb2, 0x4a, 0x53, 0x21, 0x3e, 0x91, 0x56, 0x05, 0x20, 0x9a, 0xc0, 0xa8, 0x0e, 0x16, 0xa3, 0x55,
    0x05, 0x20, 0x9c, 0xd8, 0x94, 0x13, 0x1d, 0x49, 0x4d, 0x07, 0x40, 0x74, 0x84, 0x7a, 0x25, 0x7e,
    0x78, 0x65, 0x15, 0x6b, 0xb5, 0xc0, 0x67, 0x13, 0x43, 0x7d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint16_t
vdenc_vp9_quant_ac[256] = {
    4,       8,    9,   10,   11,   12,   13,   14,
    15,     16,   17,   18,   19,   20,   21,   22,
    23,     24,   25,   26,   27,   28,   29,   30,
    31,     32,   33,   34,   35,   36,   37,   38,
    39,     40,   41,   42,   43,   44,   45,   46,
    47,     48,   49,   50,   51,   52,   53,   54,
    55,     56,   57,   58,   59,   60,   61,   62,
    63,     64,   65,   66,   67,   68,   69,   70,
    71,     72,   73,   74,   75,   76,   77,   78,
    79,     80,   81,   82,   83,   84,   85,   86,
    87,     88,   89,   90,   91,   92,   93,   94,
    95,     96,   97,   98,   99,  100,  101,  102,
    104,   106,  108,  110,  112,  114,  116,  118,
    120,   122,  124,  126,  128,  130,  132,  134,
    136,   138,  140,  142,  144,  146,  148,  150,
    152,   155,  158,  161,  164,  167,  170,  173,
    176,   179,  182,  185,  188,  191,  194,  197,
    200,   203,  207,  211,  215,  219,  223,  227,
    231,   235,  239,  243,  247,  251,  255,  260,
    265,   270,  275,  280,  285,  290,  295,  300,
    305,   311,  317,  323,  329,  335,  341,  347,
    353,   359,  366,  373,  380,  387,  394,  401,
    408,   416,  424,  432,  440,  448,  456,  465,
    474,   483,  492,  501,  510,  520,  530,  540,
    550,   560,  571,  582,  593,  604,  615,  627,
    639,   651,  663,  676,  689,  702,  715,  729,
    743,   757,  771,  786,  801,  816,  832,  848,
    864,   881,  898,  915,  933,  951,  969,  988,
    1007, 1026, 1046, 1066, 1087, 1108, 1129, 1151,
    1173, 1196, 1219, 1243, 1267, 1292, 1317, 1343,
    1369, 1396, 1423, 1451, 1479, 1508, 1537, 1567,
    1597, 1628, 1660, 1692, 1725, 1759, 1793, 1828,
};

#define OUT_BUFFER_2DW(batch, bo, is_target, delta)  do {               \
        if (bo) {                                                       \
            OUT_BCS_RELOC64(batch,                                      \
                            bo,                                         \
                            I915_GEM_DOMAIN_RENDER,                     \
                            is_target ? I915_GEM_DOMAIN_RENDER : 0,     \
                            delta);                                     \
        } else {                                                        \
            OUT_BCS_BATCH(batch, 0);                                    \
            OUT_BCS_BATCH(batch, 0);                                    \
        }                                                               \
    } while (0)

#define OUT_BUFFER_3DW(batch, bo, is_target, delta, attr)  do { \
        OUT_BUFFER_2DW(batch, bo, is_target, delta);            \
        OUT_BCS_BATCH(batch, i965->intel.mocs_state);           \
    } while (0)

#define ALLOC_VDENC_BUFFER_RESOURCE(buffer, bfsize, des) do {   \
        buffer.type = I965_GPE_RESOURCE_BUFFER;                 \
        buffer.width = bfsize;                                  \
        buffer.height = 1;                                      \
        buffer.pitch = buffer.width;                            \
        buffer.size = buffer.pitch;                             \
        buffer.tiling = I915_TILING_NONE;                       \
        i965_allocate_gpe_resource(i965->intel.bufmgr,          \
                                   &buffer,                     \
                                   bfsize,                      \
                                   (des));                      \
    } while (0)

#define MAX_URB_SIZE                    4096 /* In register */

extern const unsigned int gen9_vp9_avs_coeffs[256];

static void
gen10_vdenc_vp9_init_media_object_walker_parameter(struct intel_encoder_context *encoder_context,
                                                   struct gpe_encoder_kernel_walker_parameter *kernel_walker_param,
                                                   struct gpe_media_object_walker_parameter *walker_param)
{
    memset(walker_param, 0, sizeof(*walker_param));

    walker_param->use_scoreboard = kernel_walker_param->use_scoreboard;

    walker_param->block_resolution.x = kernel_walker_param->resolution_x;
    walker_param->block_resolution.y = kernel_walker_param->resolution_y;

    walker_param->global_resolution.x = kernel_walker_param->resolution_x;
    walker_param->global_resolution.y = kernel_walker_param->resolution_y;

    walker_param->global_outer_loop_stride.x = kernel_walker_param->resolution_x;
    walker_param->global_outer_loop_stride.y = 0;

    walker_param->global_inner_loop_unit.x = 0;
    walker_param->global_inner_loop_unit.y = kernel_walker_param->resolution_y;

    walker_param->local_loop_exec_count = 0xFFFF;  //MAX VALUE
    walker_param->global_loop_exec_count = 0xFFFF;  //MAX VALUE

    if (kernel_walker_param->no_dependency) {
        walker_param->scoreboard_mask = 0;
        walker_param->use_scoreboard = 0;
        // Raster scan walking pattern
        walker_param->local_outer_loop_stride.x = 0;
        walker_param->local_outer_loop_stride.y = 1;
        walker_param->local_inner_loop_unit.x = 1;
        walker_param->local_inner_loop_unit.y = 0;
        walker_param->local_end.x = kernel_walker_param->resolution_x - 1;
        walker_param->local_end.y = 0;
    } else {
        walker_param->local_end.x = 0;
        walker_param->local_end.y = 0;

        if (kernel_walker_param->walker_degree == VP9_45Z_DEGREE) {
            // 45z degree
            walker_param->scoreboard_mask = 0x0F;

            walker_param->global_loop_exec_count = 0x3FF;
            walker_param->local_loop_exec_count = 0x3FF;

            walker_param->global_resolution.x = (unsigned int)(kernel_walker_param->resolution_x / 2.f) + 1;
            walker_param->global_resolution.y = 2 * kernel_walker_param->resolution_y;

            walker_param->global_start.x = 0;
            walker_param->global_start.y = 0;

            walker_param->global_outer_loop_stride.x = walker_param->global_resolution.x;
            walker_param->global_outer_loop_stride.y = 0;

            walker_param->global_inner_loop_unit.x = 0;
            walker_param->global_inner_loop_unit.y = walker_param->global_resolution.y;

            walker_param->block_resolution.x = walker_param->global_resolution.x;
            walker_param->block_resolution.y = walker_param->global_resolution.y;

            walker_param->local_start.x = 0;
            walker_param->local_start.y = 0;

            walker_param->local_outer_loop_stride.x = 1;
            walker_param->local_outer_loop_stride.y = 0;

            walker_param->local_inner_loop_unit.x = -1;
            walker_param->local_inner_loop_unit.y = 4;

            walker_param->middle_loop_extra_steps = 3;
            walker_param->mid_loop_unit_x = 0;
            walker_param->mid_loop_unit_y = 1;
        } else {
            // 26 degree
            walker_param->scoreboard_mask = 0x0F;
            walker_param->local_outer_loop_stride.x = 1;
            walker_param->local_outer_loop_stride.y = 0;
            walker_param->local_inner_loop_unit.x = -2;
            walker_param->local_inner_loop_unit.y = 1;
        }
    }
}

static void
gen10_vdenc_vp9_run_media_object_walker(VADriverContextP ctx,
                                        struct intel_encoder_context *encoder_context,
                                        struct i965_gpe_context *gpe_context,
                                        struct gpe_media_object_walker_parameter *param)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;

    intel_batchbuffer_start_atomic(batch, 0x1000);

    intel_batchbuffer_emit_mi_flush(batch);
    gpe->pipeline_setup(ctx, gpe_context, batch);
    gpe->media_object_walker(ctx, gpe_context, batch, param);
    gpe->media_state_flush(ctx, gpe_context, batch);
    gpe->pipeline_end(ctx, gpe_context, batch);

    intel_batchbuffer_end_atomic(batch);

    intel_batchbuffer_flush(batch);
}

static void
gen10_vdenc_vp9_gpe_context_vfe_scoreboard_init(struct i965_gpe_context *gpe_context, struct gen10_vdenc_vp9_kernel_scoreboard_parameters *scoreboard_params)
{
    gpe_context->vfe_desc5.scoreboard0.mask = scoreboard_params->mask;
    gpe_context->vfe_desc5.scoreboard0.type = scoreboard_params->type;
    gpe_context->vfe_desc5.scoreboard0.enable = scoreboard_params->enable;

    if (scoreboard_params->walk_pattern_flag) {
        gpe_context->vfe_desc5.scoreboard0.mask = 0x0F;
        gpe_context->vfe_desc5.scoreboard0.type = 1;

        gpe_context->vfe_desc6.scoreboard1.delta_x0 = 0x0;
        gpe_context->vfe_desc6.scoreboard1.delta_y0 = 0xF;

        gpe_context->vfe_desc6.scoreboard1.delta_x1 = 0x0;
        gpe_context->vfe_desc6.scoreboard1.delta_y1 = 0xE;

        gpe_context->vfe_desc6.scoreboard1.delta_x2 = 0xF;
        gpe_context->vfe_desc6.scoreboard1.delta_y2 = 0x3;

        gpe_context->vfe_desc6.scoreboard1.delta_x3 = 0xF;
        gpe_context->vfe_desc6.scoreboard1.delta_y3 = 0x1;
    } else {
        // Scoreboard 0
        gpe_context->vfe_desc6.scoreboard1.delta_x0 = 0xF;
        gpe_context->vfe_desc6.scoreboard1.delta_y0 = 0x0;

        // Scoreboard 1
        gpe_context->vfe_desc6.scoreboard1.delta_x1 = 0x0;
        gpe_context->vfe_desc6.scoreboard1.delta_y1 = 0xF;

        // Scoreboard 2
        gpe_context->vfe_desc6.scoreboard1.delta_x2 = 0x1;
        gpe_context->vfe_desc6.scoreboard1.delta_y2 = 0xF;

        // Scoreboard 3
        gpe_context->vfe_desc6.scoreboard1.delta_x3 = 0xF;
        gpe_context->vfe_desc6.scoreboard1.delta_y3 = 0xF;

        // Scoreboard 4
        gpe_context->vfe_desc7.scoreboard2.delta_x4 = 0xF;
        gpe_context->vfe_desc7.scoreboard2.delta_y4 = 0x1;

        // Scoreboard 5
        gpe_context->vfe_desc7.scoreboard2.delta_x5 = 0x0;
        gpe_context->vfe_desc7.scoreboard2.delta_y5 = 0xE;

        // Scoreboard 6
        gpe_context->vfe_desc7.scoreboard2.delta_x6 = 0x1;
        gpe_context->vfe_desc7.scoreboard2.delta_y6 = 0xE;

        // Scoreboard 7
        gpe_context->vfe_desc7.scoreboard2.delta_x6 = 0xF;
        gpe_context->vfe_desc7.scoreboard2.delta_y6 = 0xE;
    }
}

static void
gen10_vdenc_vp9_gpe_context_init_once(VADriverContextP ctx,
                                      struct i965_gpe_context *gpe_context,
                                      struct gen10_vdenc_vp9_kernel_parameters *kernel_params)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);

    gpe_context->curbe.length = kernel_params->curbe_size; // in bytes

    gpe_context->sampler.entry_size = 0;
    gpe_context->sampler.max_entries = 0;

    if (kernel_params->sampler_size) {
        gpe_context->sampler.entry_size = ALIGN(kernel_params->sampler_size, 64);
        gpe_context->sampler.max_entries = 1;
    }

    gpe_context->idrt.entry_size = ALIGN(sizeof(struct gen8_interface_descriptor_data), 64);
    gpe_context->idrt.max_entries = NUM_KERNELS_PER_GPE_CONTEXT;

    gpe_context->surface_state_binding_table.max_entries = MAX_VP9_VDENC_SURFACES;
    gpe_context->surface_state_binding_table.binding_table_offset = 0;
    gpe_context->surface_state_binding_table.surface_state_offset = gpe_context->surface_state_binding_table.binding_table_offset + ALIGN(MAX_VP9_VDENC_SURFACES * sizeof(unsigned int), 64);
    gpe_context->surface_state_binding_table.length = ALIGN(MAX_VP9_VDENC_SURFACES * sizeof(unsigned int), 64) + ALIGN(MAX_VP9_VDENC_SURFACES * SURFACE_STATE_PADDED_SIZE_GEN9, 64);

    if (i965->intel.eu_total > 0)
        gpe_context->vfe_state.max_num_threads = 6 * i965->intel.eu_total;
    else
        gpe_context->vfe_state.max_num_threads = 112;

    gpe_context->vfe_state.curbe_allocation_size = ALIGN(gpe_context->curbe.length, 32) >> 5; // in registers
    gpe_context->vfe_state.urb_entry_size = MAX(1, (ALIGN(kernel_params->inline_data_size, 32) +
                                                    ALIGN(kernel_params->external_data_size, 32)) >> 5); // in registers
    gpe_context->vfe_state.num_urb_entries = (MAX_URB_SIZE -
                                              gpe_context->vfe_state.curbe_allocation_size -
                                              ((gpe_context->idrt.entry_size >> 5) *
                                               gpe_context->idrt.max_entries)) / gpe_context->vfe_state.urb_entry_size;
    gpe_context->vfe_state.num_urb_entries = CLAMP(gpe_context->vfe_state.num_urb_entries, 1, 64);
    gpe_context->vfe_state.gpgpu_mode = 0;
}

static void
gen10_vdenc_vp9_dys_set_sampler_state(struct i965_gpe_context *gpe_context)
{
    struct gen9_sampler_8x8_avs *sampler_cmd;

    if (!gpe_context)
        return;

    dri_bo_map(gpe_context->sampler.bo, 1);

    if (!gpe_context->sampler.bo->virtual)
        return;

    sampler_cmd = (struct gen9_sampler_8x8_avs *)(gpe_context->sampler.bo->virtual + gpe_context->sampler.offset);
    memset(sampler_cmd, 0, sizeof(struct gen9_sampler_8x8_avs));

    sampler_cmd->dw0.r3c_coefficient = 15;
    sampler_cmd->dw0.r3x_coefficient = 6;
    sampler_cmd->dw0.strong_edge_threshold = 8;
    sampler_cmd->dw0.weak_edge_threshold = 1;
    sampler_cmd->dw0.gain_factor = 32;

    sampler_cmd->dw2.r5c_coefficient = 3;
    sampler_cmd->dw2.r5cx_coefficient = 8;
    sampler_cmd->dw2.r5x_coefficient = 9;
    sampler_cmd->dw2.strong_edge_weight = 6;
    sampler_cmd->dw2.regular_weight = 3;
    sampler_cmd->dw2.non_edge_weight = 2;
    sampler_cmd->dw2.global_noise_estimation = 255;

    sampler_cmd->dw3.enable_8tap_adaptive_filter = 0;
    sampler_cmd->dw3.cos_alpha = 79;
    sampler_cmd->dw3.sin_alpha = 101;

    sampler_cmd->dw5.diamond_du = 0;
    sampler_cmd->dw5.hs_margin = 3;
    sampler_cmd->dw5.diamond_alpha = 100;

    sampler_cmd->dw7.inv_margin_vyl = 3300;

    sampler_cmd->dw8.inv_margin_vyu = 1600;

    sampler_cmd->dw10.y_slope2 = 24;
    sampler_cmd->dw10.s0l = 1792;

    sampler_cmd->dw12.y_slope1 = 24;

    sampler_cmd->dw14.s0u = 256;

    sampler_cmd->dw15.s2u = 1792;
    sampler_cmd->dw15.s1u = 0;

    memcpy(sampler_cmd->coefficients, &gen9_vp9_avs_coeffs[0], 17 * sizeof(struct gen8_sampler_8x8_avs_coefficients));

    sampler_cmd->dw152.default_sharpness_level = 255;
    sampler_cmd->dw152.max_derivative_4_pixels = 7;
    sampler_cmd->dw152.max_derivative_8_pixels = 20;
    sampler_cmd->dw152.transition_area_with_4_pixels = 4;
    sampler_cmd->dw152.transition_area_with_8_pixels = 5;

    sampler_cmd->dw153.bypass_x_adaptive_filtering = 1;
    sampler_cmd->dw153.bypass_y_adaptive_filtering = 1;
    sampler_cmd->dw153.adaptive_filter_for_all_channel = 0;

    memcpy(sampler_cmd->extra_coefficients, &gen9_vp9_avs_coeffs[17 * 8], 15 * sizeof(struct gen8_sampler_8x8_avs_coefficients));

    dri_bo_unmap(gpe_context->sampler.bo);
}

static void
gen10_vdenc_vp9_dys_context_init(VADriverContextP ctx, struct gen10_vdenc_vp9_context *vdenc_context)
{
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    struct i965_gpe_context *gpe_context = NULL;
    struct gen10_vdenc_vp9_kernel_parameters kernel_params;
    struct gen10_vdenc_vp9_kernel_scoreboard_parameters scoreboard_params;

    kernel_params.curbe_size = sizeof(struct gen10_vdenc_vp9_dys_curbe_data);
    kernel_params.inline_data_size = 0;
    kernel_params.external_data_size = 0;
    kernel_params.sampler_size = sizeof(struct gen9_sampler_8x8_avs);

    memset(&scoreboard_params, 0, sizeof(scoreboard_params));
    scoreboard_params.mask = 0xFF;
    scoreboard_params.enable = vdenc_context->use_hw_scoreboard;
    scoreboard_params.type = vdenc_context->use_hw_non_stalling_scoreborad;
    scoreboard_params.walk_pattern_flag = 0;

    gpe_context = &vdenc_context->dys_context.gpe_context;
    gen10_vdenc_vp9_gpe_context_init_once(ctx, gpe_context, &kernel_params);
    gen10_vdenc_vp9_gpe_context_vfe_scoreboard_init(gpe_context, &scoreboard_params);
    gpe->load_kernels(ctx,
                      gpe_context,
                      &vdenc_vp9_kernels_dys[0],
                      1);
}

static void
gen10_vdenc_vp9_streamin_context_init(VADriverContextP ctx, struct gen10_vdenc_vp9_context *vdenc_context)
{
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    struct i965_gpe_context *gpe_context = NULL;
    struct gen10_vdenc_vp9_kernel_parameters kernel_params;
    struct gen10_vdenc_vp9_kernel_scoreboard_parameters scoreboard_params;

    kernel_params.curbe_size = sizeof(struct gen10_vdenc_vp9_streamin_curbe_data);
    kernel_params.inline_data_size = 0;
    kernel_params.external_data_size = 0;
    kernel_params.sampler_size = 0;

    memset(&scoreboard_params, 0, sizeof(scoreboard_params));
    scoreboard_params.mask = 0xFF;
    scoreboard_params.enable = vdenc_context->use_hw_scoreboard;
    scoreboard_params.type = vdenc_context->use_hw_non_stalling_scoreborad;
    scoreboard_params.walk_pattern_flag = 0;

    gpe_context = &vdenc_context->streamin_context.gpe_context;
    gen10_vdenc_vp9_gpe_context_init_once(ctx, gpe_context, &kernel_params);
    gen10_vdenc_vp9_gpe_context_vfe_scoreboard_init(gpe_context, &scoreboard_params);
    gpe->load_kernels(ctx,
                      gpe_context,
                      &vdenc_vp9_kernels_streamin[0],
                      1);
}

static Bool
gen10_vdenc_vp9_gpe_context_init(VADriverContextP ctx,
                                 struct intel_encoder_context *encoder_context,
                                 struct gen10_vdenc_vp9_context *vdenc_context)
{
    gen10_vdenc_vp9_dys_context_init(ctx, vdenc_context);
    gen10_vdenc_vp9_streamin_context_init(ctx, vdenc_context);

    return True;
}

static void
gen10_vdenc_vp9_kernel_context_destroy(struct gen10_vdenc_vp9_context *vdenc_context)
{
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;

    gpe->context_destroy(&vdenc_context->dys_context.gpe_context);
    gpe->context_destroy(&vdenc_context->streamin_context.gpe_context);
}

static VAStatus
gen10_vdenc_vp9_gpe_kernel_init(VADriverContextP ctx,
                                struct encode_state *encode_state,
                                struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct gen10_vdenc_vp9_dys_context *dys_context = &vdenc_context->dys_context;
    struct gen10_vdenc_vp9_streamin_context *streamin_context = &vdenc_context->streamin_context;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;

    /* DYS */
    gpe->context_init(ctx, &dys_context->gpe_context);
    gen10_vdenc_vp9_dys_set_sampler_state(&dys_context->gpe_context);

    gpe->context_init(ctx, &streamin_context->gpe_context);

    return VA_STATUS_SUCCESS;
}

static void
gen10_vdenc_vp9_dys_set_curbe_data(VADriverContextP ctx,
                                   struct encode_state *encode_state,
                                   struct i965_gpe_context *gpe_context,
                                   struct intel_encoder_context *encoder_context,
                                   struct gen10_vdenc_vp9_dys_curbe_parameters *curbe_param)
{
    struct gen10_vdenc_vp9_dys_curbe_data *curbe_cmd;

    curbe_cmd = i965_gpe_context_map_curbe(gpe_context);

    if (!curbe_cmd)
        return;

    memset(curbe_cmd, 0, sizeof(*curbe_cmd));

    curbe_cmd->dw0.input_frame_width = curbe_param->input_width;
    curbe_cmd->dw0.input_frame_height = curbe_param->input_height;

    curbe_cmd->dw1.output_frame_width = curbe_param->output_width;
    curbe_cmd->dw1.output_frame_height = curbe_param->output_height;

    curbe_cmd->dw2.delta_u = 1.0f / curbe_param->output_width;
    curbe_cmd->dw3.delta_v = 1.0f / curbe_param->output_height;

    curbe_cmd->dw16.input_frame_nv12_bti = VP9_BTI_DYS_INPUT_NV12;
    curbe_cmd->dw17.output_frame_y_bti = VP9_BTI_DYS_OUTPUT_Y;
    curbe_cmd->dw18.avs_sample_bti = 0;

    i965_gpe_context_unmap_curbe(gpe_context);
}

static VAStatus
gen10_vdenc_vp9_dys_surface(VADriverContextP ctx,
                            struct encode_state *encode_state,
                            struct i965_gpe_context *gpe_context,
                            struct intel_encoder_context *encoder_context,
                            struct gen10_vdenc_vp9_dys_surface_parameters *surface_param)
{

    if (surface_param->input_frame)
        i965_add_adv_gpe_surface(ctx,
                                 gpe_context,
                                 surface_param->input_frame,
                                 VP9_BTI_DYS_INPUT_NV12);

    if (surface_param->output_frame) {
        i965_add_2d_gpe_surface(ctx,
                                gpe_context,
                                surface_param->output_frame,
                                0,
                                1,
                                I965_SURFACEFORMAT_R8_UNORM,
                                VP9_BTI_DYS_OUTPUT_Y);

        i965_add_2d_gpe_surface(ctx,
                                gpe_context,
                                surface_param->output_frame,
                                1,
                                1,
                                I965_SURFACEFORMAT_R16_UINT,
                                VP9_BTI_DYS_OUTPUT_UV);
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_dys_kernel(VADriverContextP ctx,
                           struct encode_state *encode_state,
                           struct intel_encoder_context *encoder_context,
                           struct gen10_vdenc_vp9_dys_kernel_parameters *dys_kernel_param)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct i965_gpe_context *gpe_context;
    struct gen10_vdenc_vp9_dys_curbe_parameters curbe_param;
    struct gen10_vdenc_vp9_dys_surface_parameters surface_param;
    struct gpe_media_object_walker_parameter media_object_walker_param;
    struct gpe_encoder_kernel_walker_parameter kernel_walker_param;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    unsigned int resolution_x, resolution_y;

    gpe_context = &vdenc_context->dys_context.gpe_context;
    gpe->reset_binding_table(ctx, gpe_context);

    memset(&curbe_param, 0, sizeof(curbe_param));
    curbe_param.input_width = dys_kernel_param->input_width;
    curbe_param.input_height = dys_kernel_param->input_height;
    curbe_param.output_width = dys_kernel_param->output_width;
    curbe_param.output_height = dys_kernel_param->output_height;
    gen10_vdenc_vp9_dys_set_curbe_data(ctx,
                                       encode_state,
                                       gpe_context,
                                       encoder_context,
                                       &curbe_param);

    // Add surface states
    memset(&surface_param, 0, sizeof(surface_param));
    surface_param.input_frame = dys_kernel_param->input_surface;
    surface_param.output_frame = dys_kernel_param->output_surface;
    surface_param.vert_line_stride = 0;
    surface_param.vert_line_stride_offset = 0;
    gen10_vdenc_vp9_dys_surface(ctx,
                                encode_state,
                                gpe_context,
                                encoder_context,
                                &surface_param);

    gpe->setup_interface_data(ctx, gpe_context);

    resolution_x = ALIGN(dys_kernel_param->output_width, 16) / 16;
    resolution_y = ALIGN(dys_kernel_param->output_height, 16) / 16;
    memset(&kernel_walker_param, 0, sizeof(kernel_walker_param));
    kernel_walker_param.resolution_x = resolution_x;
    kernel_walker_param.resolution_y = resolution_y;
    kernel_walker_param.no_dependency = 1;
    gen10_vdenc_vp9_init_media_object_walker_parameter(encoder_context, &kernel_walker_param, &media_object_walker_param);

    gen10_vdenc_vp9_run_media_object_walker(ctx,
                                            encoder_context,
                                            gpe_context,
                                            &media_object_walker_param);

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_dys_src_frame(VADriverContextP ctx,
                              struct encode_state *encode_state,
                              struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct gen10_vdenc_vp9_dys_kernel_parameters dys_kernel_param;
    struct object_surface *obj_surface, *input_surface, *output_surface;
    struct vdenc_vp9_surface *vdenc_vp9_surface;
    VAEncPictureParameterBufferVP9  *pic_param = vdenc_context->pic_param;

    if (!vdenc_context->dys_in_use)
        return VA_STATUS_SUCCESS;

    if ((pic_param->frame_width_src != pic_param->frame_width_dst) ||
        (pic_param->frame_height_src != pic_param->frame_height_dst)) {
        input_surface = encode_state->input_yuv_object;
        obj_surface = encode_state->reconstructed_object;
        vdenc_vp9_surface = (struct vdenc_vp9_surface *)(obj_surface->private_data);
        output_surface = vdenc_vp9_surface->dys_surface_obj;

        memset(&dys_kernel_param, 0, sizeof(dys_kernel_param));
        dys_kernel_param.input_width = pic_param->frame_width_src;
        dys_kernel_param.input_height = pic_param->frame_height_src;
        dys_kernel_param.input_surface = input_surface;
        dys_kernel_param.output_width = pic_param->frame_width_dst;
        dys_kernel_param.output_height = pic_param->frame_height_dst;
        dys_kernel_param.output_surface = output_surface;

        gen10_vdenc_vp9_dys_kernel(ctx,
                                   encode_state,
                                   encoder_context,
                                   &dys_kernel_param);
    }

    return VA_STATUS_SUCCESS;
}

#if 0

static void
gen10_vdenc_vp9_streamin_set_curbe_data(VADriverContextP ctx,
                                        struct encode_state *encode_state,
                                        struct i965_gpe_context *gpe_context,
                                        struct intel_encoder_context *encoder_context,
                                        struct gen10_vdenc_vp9_streamin_curbe_parameters *curbe_param)
{
    struct gen10_vdenc_vp9_streamin_curbe_data *curbe_cmd;

    curbe_cmd = i965_gpe_context_map_curbe(gpe_context);

    if (!curbe_cmd)
        return;

    memset(curbe_cmd, 0, sizeof(*curbe_cmd));

    i965_gpe_context_unmap_curbe(gpe_context);
}

static VAStatus
gen10_vdenc_vp9_streamin_surface(VADriverContextP ctx,
                                 struct encode_state *encode_state,
                                 struct i965_gpe_context *gpe_context,
                                 struct intel_encoder_context *encoder_context,
                                 struct gen10_vdenc_vp9_streamin_surface_parameters *surface_param)
{

    if (surface_param->input_frame)
        i965_add_adv_gpe_surface(ctx,
                                 gpe_context,
                                 surface_param->input_frame,
                                 VP9_BTI_DYS_INPUT_NV12);

    if (surface_param->output_frame) {
        i965_add_2d_gpe_surface(ctx,
                                gpe_context,
                                surface_param->output_frame,
                                0,
                                1,
                                I965_SURFACEFORMAT_R8_UNORM,
                                VP9_BTI_DYS_OUTPUT_Y);

        i965_add_2d_gpe_surface(ctx,
                                gpe_context,
                                surface_param->output_frame,
                                1,
                                1,
                                I965_SURFACEFORMAT_R16_UINT,
                                VP9_BTI_DYS_OUTPUT_UV);
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_streamin_kernel(VADriverContextP ctx,
                                struct encode_state *encode_state,
                                struct intel_encoder_context *encoder_context,
                                struct gen10_vdenc_vp9_streamin_kernel_parameters *streamin_kernel_param)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct i965_gpe_context *gpe_context;
    struct gen10_vdenc_vp9_streamin_curbe_parameters curbe_param;
    struct gen10_vdenc_vp9_streamin_surface_parameters surface_param;
    struct gpe_media_object_walker_parameter media_object_walker_param;
    struct gpe_encoder_kernel_walker_parameter kernel_walker_param;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    unsigned int resolution_x, resolution_y;

    gpe_context = &vdenc_context->streamin_context.gpe_context;
    gpe->reset_binding_table(ctx, gpe_context);

    memset(&curbe_param, 0, sizeof(curbe_param));
    curbe_param.input_width = streamin_kernel_param->input_width;
    curbe_param.input_height = streamin_kernel_param->input_height;
    curbe_param.output_width = streamin_kernel_param->output_width;
    curbe_param.output_height = streamin_kernel_param->output_height;
    gen10_vdenc_vp9_streamin_set_curbe_data(ctx,
                                            encode_state,
                                            gpe_context,
                                            encoder_context,
                                            &curbe_param);

    // Add surface states
    memset(&surface_param, 0, sizeof(surface_param));
    surface_param.input_frame = streamin_kernel_param->input_surface;
    surface_param.output_frame = streamin_kernel_param->output_surface;
    surface_param.vert_line_stride = 0;
    surface_param.vert_line_stride_offset = 0;
    gen10_vdenc_vp9_streamin_surface(ctx,
                                     encode_state,
                                     gpe_context,
                                     encoder_context,
                                     &surface_param);

    gpe->setup_interface_data(ctx, gpe_context);

    resolution_x = ALIGN(streamin_kernel_param->output_width, 16) / 16;
    resolution_y = ALIGN(streamin_kernel_param->output_height, 16) / 16;
    memset(&kernel_walker_param, 0, sizeof(kernel_walker_param));
    kernel_walker_param.resolution_x = resolution_x;
    kernel_walker_param.resolution_y = resolution_y;
    kernel_walker_param.no_dependency = 1;
    gen10_vdenc_vp9_init_media_object_walker_parameter(encoder_context, &kernel_walker_param, &media_object_walker_param);

    gen10_vdenc_vp9_run_media_object_walker(ctx,
                                            encoder_context,
                                            gpe_context,
                                            &media_object_walker_param);

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_run_streamin(VADriverContextP ctx,
                             struct encode_state *encode_state,
                             struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct gen10_vdenc_vp9_streamin_kernel_parameters streamin_kernel_param;
    struct object_surface *obj_surface, *input_surface, *output_surface;
    struct vdenc_vp9_surface *vdenc_vp9_surface;
    VAEncPictureParameterBufferVP9  *pic_param = vdenc_context->pic_param;

    input_surface = encode_state->input_yuv_object;
    obj_surface = encode_state->reconstructed_object;
    vdenc_vp9_surface = (struct vdenc_vp9_surface *)(obj_surface->private_data);
    output_surface = vdenc_vp9_surface->dys_surface_obj;

    memset(&streamin_kernel_param, 0, sizeof(streamin_kernel_param));
    streamin_kernel_param.input_width = pic_param->frame_width_src;
    streamin_kernel_param.input_height = pic_param->frame_height_src;
    streamin_kernel_param.input_surface = input_surface;
    streamin_kernel_param.output_width = pic_param->frame_width_dst;
    streamin_kernel_param.output_height = pic_param->frame_height_dst;
    streamin_kernel_param.output_surface = output_surface;

    gen10_vdenc_vp9_streamin_kernel(ctx,
                                    encode_state,
                                    encoder_context,
                                    &streamin_kernel_param);
    return VA_STATUS_SUCCESS;
}

#endif

static uint32_t
intel_convert_sign_mag(int val, int sign_bit_pos)
{
    uint32_t ret_val = 0;

    if (val < 0) {
        val = -val;
        ret_val = ((1 << (sign_bit_pos - 1)) | (val & ((1 << (sign_bit_pos - 1)) - 1)));
    } else {
        ret_val = val & ((1 << (sign_bit_pos - 1)) - 1);
    }

    return ret_val;
}

static void
gen10_vdenc_vp9_update_brc_parameters(VADriverContextP ctx,
                                      struct encode_state *encode_state,
                                      struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;

    vdenc_context->brc_enabled = 0;
    vdenc_context->target_bit_rate = 0;
    vdenc_context->max_bit_rate = 0;
    vdenc_context->min_bit_rate = 0;
    vdenc_context->init_vbv_buffer_fullness_in_bit = 0;
    vdenc_context->vbv_buffer_size_in_bit = 0;
}

static VAStatus
gen10_vdenc_vp9_update_parameters(VADriverContextP ctx,
                                  VAProfile profile,
                                  struct encode_state *encode_state,
                                  struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct object_surface *obj_surface;
    VAEncPictureParameterBufferVP9 *pic_param = NULL;
    VAEncMiscParameterTypeVP9PerSegmantParam *seg_param = NULL;
    VAEncSequenceParameterBufferVP9 *seq_param = NULL;
    VDEncVP9Surface *vdenc_vp9_surface;

    vdenc_context->dys_multiple_pass_enbaled = 1;
    vdenc_context->tx_mode = TX_MODE_SELECT;
    vdenc_context->current_pass = 0;
    vdenc_context->num_passes = 1;

    if (vdenc_context->has_hme)
        vdenc_context->need_hme = (encoder_context->quality_level == 1);
    else
        vdenc_context->need_hme = 0;

    gen10_vdenc_vp9_update_brc_parameters(ctx, encode_state, encoder_context);

    if (vdenc_context->use_huc)
        vdenc_context->num_passes = 2;

    if (vdenc_context->brc_enabled) {
        vdenc_context->num_passes = VDENC_VP9_BRC_MAX_NUM_OF_PASSES;

        if (!vdenc_context->multiple_pass_brc_enabled)
            vdenc_context->num_passes--;
    }

    if (!encode_state->pic_param_ext ||
        !encode_state->pic_param_ext->buffer) {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    pic_param = (VAEncPictureParameterBufferVP9 *)encode_state->pic_param_ext->buffer;

    if (!pic_param->pic_flags.bits.frame_type)
        vdenc_context->is_key_frame = 1;
    else
        vdenc_context->is_key_frame = 0;

    if (pic_param->pic_flags.bits.intra_only)
        vdenc_context->frame_intra_only = 1;
    else
        vdenc_context->frame_intra_only = 0;

    vdenc_context->is_8bit = (VAProfileVP9Profile0 == profile);
    vdenc_context->ref_frame_flag = VP9_REF_NONE;
    vdenc_context->num_ref_frames = 0;
    vdenc_context->pic_param = NULL;
    vdenc_context->segment_param = NULL;
    vdenc_context->seq_param = NULL;
    vdenc_context->last_ref_obj = NULL;
    vdenc_context->golden_ref_obj = NULL;
    vdenc_context->alt_ref_obj = NULL;
    i965_free_gpe_resource(&vdenc_context->mb_segment_map_buffer_res);

    if (vdenc_context->is_key_frame || vdenc_context->frame_intra_only) {
        /* this will be regarded as I-frame type */
        vdenc_context->vp9_frame_type = VP9_FRAME_I;
    } else {
        vdenc_context->vp9_frame_type = VP9_FRAME_P;
        vdenc_context->ref_frame_flag = (pic_param->ref_flags.bits.ref_frame_ctrl_l0 | pic_param->ref_flags.bits.ref_frame_ctrl_l1);

        /* Last */
        obj_surface = SURFACE(pic_param->reference_frames[pic_param->ref_flags.bits.ref_last_idx]);

        if (!obj_surface ||
            !obj_surface->bo ||
            !obj_surface->private_data) {
            vdenc_context->ref_frame_flag &= ~(VP9_REF_LAST);
            obj_surface = NULL;
        }

        vdenc_context->last_ref_obj = obj_surface;

        /* Golden */
        obj_surface = SURFACE(pic_param->reference_frames[pic_param->ref_flags.bits.ref_gf_idx]);

        if (!obj_surface ||
            !obj_surface->bo ||
            !obj_surface->private_data) {
            vdenc_context->ref_frame_flag &= ~(VP9_REF_GOLDEN);
            obj_surface = NULL;
        }

        vdenc_context->golden_ref_obj = obj_surface;

        /* Alt */
        obj_surface = SURFACE(pic_param->reference_frames[pic_param->ref_flags.bits.ref_arf_idx]);

        if (!obj_surface ||
            !obj_surface->bo ||
            !obj_surface->private_data) {
            vdenc_context->ref_frame_flag &= ~(VP9_REF_ALT);
            obj_surface = NULL;
        }

        vdenc_context->alt_ref_obj = obj_surface;

        /* remove the duplicated flag and ref frame list */
        if (vdenc_context->ref_frame_flag & VP9_REF_LAST) {
            if (pic_param->reference_frames[pic_param->ref_flags.bits.ref_last_idx] ==
                pic_param->reference_frames[pic_param->ref_flags.bits.ref_gf_idx]) {
                vdenc_context->ref_frame_flag &= ~(VP9_REF_GOLDEN);
                vdenc_context->golden_ref_obj = NULL;
            }

            if (pic_param->reference_frames[pic_param->ref_flags.bits.ref_last_idx] ==
                pic_param->reference_frames[pic_param->ref_flags.bits.ref_arf_idx]) {
                vdenc_context->ref_frame_flag &= ~(VP9_REF_ALT);
                vdenc_context->alt_ref_obj = NULL;
            }

            vdenc_context->num_ref_frames++;
        }

        if (vdenc_context->ref_frame_flag & VP9_REF_GOLDEN) {
            if (pic_param->reference_frames[pic_param->ref_flags.bits.ref_gf_idx] ==
                pic_param->reference_frames[pic_param->ref_flags.bits.ref_arf_idx]) {
                vdenc_context->ref_frame_flag &= ~(VP9_REF_ALT);
                vdenc_context->alt_ref_obj = NULL;
            }

            vdenc_context->num_ref_frames++;
        }

        if (vdenc_context->ref_frame_flag & VP9_REF_ALT) {
            vdenc_context->num_ref_frames++;
        }

        if (vdenc_context->ref_frame_flag == VP9_REF_NONE)
            return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    vdenc_context->hme_enabled = (vdenc_context->need_hme && vdenc_context->vp9_frame_type == VP9_FRAME_P);
    vdenc_context->hme_16x_enabled = (vdenc_context->hme_enabled && vdenc_context->has_hme_16x);

    if (pic_param->pic_flags.bits.segmentation_enabled) {
        if (!encode_state->q_matrix ||
            !encode_state->q_matrix->buffer) {
            return VA_STATUS_ERROR_INVALID_PARAMETER;
        }

        if (!encode_state->encmb_map ||
            !encode_state->encmb_map->bo)
            return VA_STATUS_ERROR_INVALID_PARAMETER;

        seg_param = (VAEncMiscParameterTypeVP9PerSegmantParam *)encode_state->q_matrix->buffer;
        vdenc_context->segment_param = seg_param;
        i965_dri_object_to_2d_gpe_resource(&vdenc_context->mb_segment_map_buffer_res,
                                           encode_state->encmb_map->bo,
                                           vdenc_context->frame_width_in_mi_units,
                                           vdenc_context->frame_height_in_mi_units,
                                           vdenc_context->frame_width_in_mi_units);
    }

    if (encode_state->seq_param_ext &&
        encode_state->seq_param_ext->buffer)
        seq_param = (VAEncSequenceParameterBufferVP9 *)encode_state->seq_param_ext->buffer;

    if (!seq_param) {
        seq_param = &vdenc_context->bogus_seq_param;
    }

    vdenc_context->pic_param = pic_param;
    vdenc_context->seq_param = seq_param;

    vdenc_context->frame_width = pic_param->frame_width_dst; // TODO, check dynamic scaling
    vdenc_context->frame_height = pic_param->frame_height_dst; // TODO, check dynamic scaling

    if (vdenc_context->max_frame_width < vdenc_context->frame_width)
        vdenc_context->max_frame_width = vdenc_context->frame_width;

    if (vdenc_context->max_frame_height < vdenc_context->frame_height)
        vdenc_context->max_frame_height = vdenc_context->frame_height;

    vdenc_context->frame_width_in_mi_units = ALIGN(vdenc_context->frame_width, 8) / 8;
    vdenc_context->frame_height_in_mi_units = ALIGN(vdenc_context->frame_height, 8) / 8;
    vdenc_context->frame_width_in_sbs = ALIGN(vdenc_context->frame_width, 64) / 64;
    vdenc_context->frame_height_in_sbs = ALIGN(vdenc_context->frame_height, 64) / 64;

    vdenc_context->down_scaled_width_4x = ALIGN(vdenc_context->frame_width / 4, 16);
    vdenc_context->down_scaled_height_4x = ALIGN(vdenc_context->frame_height / 4, 16);
    vdenc_context->down_scaled_width_16x = ALIGN(vdenc_context->frame_width / 16, 16);
    vdenc_context->down_scaled_height_16x = ALIGN(vdenc_context->frame_height / 16, 16);

    vdenc_context->frame_width_in_mbs = ALIGN(vdenc_context->frame_width, 16) / 16;
    vdenc_context->frame_height_in_mbs = ALIGN(vdenc_context->frame_height, 16) / 16;
    vdenc_context->down_scaled_width_in_mb4x = vdenc_context->down_scaled_width_4x / 16;
    vdenc_context->down_scaled_height_in_mb4x = vdenc_context->down_scaled_height_4x / 16;
    vdenc_context->down_scaled_width_in_mb16x = vdenc_context->down_scaled_width_16x / 16;
    vdenc_context->down_scaled_height_in_mb16x = vdenc_context->down_scaled_height_16x / 16;

    vdenc_context->dys_in_use = 0;

    if (pic_param->frame_width_src != pic_param->frame_width_dst ||
        pic_param->frame_height_src != pic_param->frame_height_dst)
        vdenc_context->dys_in_use = 1;

    vdenc_context->dys_ref_frame_flag = VP9_REF_NONE;

    if (vdenc_context->vp9_frame_type == VP9_FRAME_P) {
        vdenc_context->dys_ref_frame_flag = vdenc_context->ref_frame_flag;

        if ((vdenc_context->ref_frame_flag & VP9_REF_LAST) &&
            vdenc_context->last_ref_obj) {
            obj_surface = vdenc_context->last_ref_obj;
            vdenc_vp9_surface = (VDEncVP9Surface *)(obj_surface->private_data);

            if (vdenc_context->frame_width == vdenc_vp9_surface->frame_width &&
                vdenc_context->frame_height == vdenc_vp9_surface->frame_height)
                vdenc_context->dys_ref_frame_flag &= ~(VP9_REF_LAST);
        }

        if ((vdenc_context->ref_frame_flag & VP9_REF_GOLDEN) &&
            vdenc_context->golden_ref_obj) {
            obj_surface = vdenc_context->golden_ref_obj;
            vdenc_vp9_surface = (VDEncVP9Surface *)(obj_surface->private_data);

            if (vdenc_context->frame_width == vdenc_vp9_surface->frame_width &&
                vdenc_context->frame_height == vdenc_vp9_surface->frame_height)
                vdenc_context->dys_ref_frame_flag &= ~(VP9_REF_GOLDEN);
        }

        if ((vdenc_context->ref_frame_flag & VP9_REF_ALT) &&
            vdenc_context->alt_ref_obj) {
            obj_surface = vdenc_context->alt_ref_obj;
            vdenc_vp9_surface = (VDEncVP9Surface *)(obj_surface->private_data);

            if (vdenc_context->frame_width == vdenc_vp9_surface->frame_width &&
                vdenc_context->frame_height == vdenc_vp9_surface->frame_height)
                vdenc_context->dys_ref_frame_flag &= ~(VP9_REF_ALT);
        }

        if (vdenc_context->dys_ref_frame_flag)
            vdenc_context->dys_in_use = 1;
    }

    if (vdenc_context->dys_ref_frame_flag != VP9_REF_NONE &&
        vdenc_context->dys_multiple_pass_enbaled &&
        !vdenc_context->use_huc)
        vdenc_context->num_passes = 2;

    return VA_STATUS_SUCCESS;
}

static void
gen10_vdenc_vp9_update_streamin_state(VADriverContextP ctx,
                                      struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct gen10_vdenc_vp9_streamin_state *streamin_state;
    int row, col, i;
    int bwidth = ALIGN(vdenc_context->frame_width, VP9_SUPER_BLOCK_WIDTH) / 32;
    int bheight = ALIGN(vdenc_context->frame_height, VP9_SUPER_BLOCK_HEIGHT) / 32;
    unsigned char *segid_buffer, segid;

    if (!vdenc_context->segment_param && !vdenc_context->hme_enabled)
        return;

    i965_zero_gpe_resource(&vdenc_context->vdenc_streamin_buffer_res);

    if (!vdenc_context->segment_param)
        return;

    streamin_state = (struct gen10_vdenc_vp9_streamin_state *)i965_map_gpe_resource(&vdenc_context->vdenc_streamin_buffer_res);

    if (!streamin_state)
        return;

    segid_buffer = (unsigned char *)i965_map_gpe_resource(&vdenc_context->mb_segment_map_buffer_res);

    if (!segid_buffer) {
        i965_unmap_gpe_resource(&vdenc_context->vdenc_streamin_buffer_res);
        return;
    }

    for (col = 0;  col < bwidth; col++) {
        for (row = 0; row < bheight; row++) {
            i = row * bwidth + col;
            segid = segid_buffer[i * 16];

            streamin_state[i].dw7.segid_enable = 1;
            streamin_state[i].dw7.segid_32x32_0_16x16_03_vp9_only = (segid | segid << 4 | segid << 8 | segid << 12);
            streamin_state[i].dw0.max_tu_size = 3;
            streamin_state[i].dw0.max_cu_size = 3;

            if ((i % 4) == 3 && !vdenc_context->is_key_frame) {
                if (!(streamin_state[i - 3].dw7.segid_32x32_0_16x16_03_vp9_only == streamin_state[i - 2].dw7.segid_32x32_0_16x16_03_vp9_only &&
                      streamin_state[i - 2].dw7.segid_32x32_0_16x16_03_vp9_only == streamin_state[i - 1].dw7.segid_32x32_0_16x16_03_vp9_only &&
                      streamin_state[i - 1].dw7.segid_32x32_0_16x16_03_vp9_only == streamin_state[i].dw7.segid_32x32_0_16x16_03_vp9_only)) {
                    streamin_state[i - 3].dw0.max_cu_size = streamin_state[i - 2].dw0.max_cu_size = streamin_state[i - 1].dw0.max_cu_size = streamin_state[i].dw0.max_cu_size = 2;
                }
            }

            streamin_state[i].dw0.num_ime_predictors = 8;

            switch (encoder_context->quality_level) {
            case 7:
                streamin_state[i].dw0.num_ime_predictors = 4;
                streamin_state[i].dw6.num_merge_candidate_cu_8x8 = 0;
                streamin_state[i].dw6.num_merge_candidate_cu_16x16 = 2;
                streamin_state[i].dw6.num_merge_candidate_cu_32x32 = 2;
                streamin_state[i].dw6.num_merge_candidate_cu_64x64 = 2;

                break;

            case 1:
            case 4:
            default:
                streamin_state[i].dw6.num_merge_candidate_cu_8x8 = 1;
                streamin_state[i].dw6.num_merge_candidate_cu_16x16 = 2;
                streamin_state[i].dw6.num_merge_candidate_cu_32x32 = 3;
                streamin_state[i].dw6.num_merge_candidate_cu_64x64 = 4;

                break;
            }
        }
    }

    i965_unmap_gpe_resource(&vdenc_context->mb_segment_map_buffer_res);
    i965_unmap_gpe_resource(&vdenc_context->vdenc_streamin_buffer_res);
}

static VAStatus
gen10_vdenc_vp9_allocate_resources_once(VADriverContextP ctx,
                                        struct encode_state *encode_state,
                                        struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    int res_size;

    if (vdenc_context->allocate_once_done)
        return VA_STATUS_SUCCESS;

    i965_free_gpe_resource(&vdenc_context->brc_history_buffer_res);
    i965_free_gpe_resource(&vdenc_context->brc_constant_data_buffer_res);
    i965_free_gpe_resource(&vdenc_context->brc_bitstream_size_buffer_res);
    i965_free_gpe_resource(&vdenc_context->brc_huc_data_buffer_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_brc_stat_buffer_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_brc_pak_stat_buffer_res);
    i965_free_gpe_resource(&vdenc_context->segmentid_buffer_res);

    res_size = VDENC_VP9_BRC_HISTORY_BUFFER_SIZE;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->brc_history_buffer_res,
                                res_size,
                                "Brc history buffer");

    res_size = VDENC_VP9_BRC_CONSTANT_DATA_SIZE;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->brc_constant_data_buffer_res,
                                res_size,
                                "Brc constant buffer");

    res_size = ALIGN(VDENC_VP9_BRC_BITSTREAM_SIZE_BUFFER_SIZE, 4096);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->brc_bitstream_size_buffer_res,
                                res_size,
                                "Brc bitstream buffer");

    res_size = ALIGN(VDENC_VP9_BRC_HUC_DATA_BUFFER_SIZE, 4096);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->brc_huc_data_buffer_res,
                                res_size,
                                "Brc huc data");

    res_size = ALIGN(VDENC_VP9_BRC_STATS_BUF_SIZE, 4096);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_brc_stat_buffer_res,
                                res_size,
                                "VDEnc brc statistics buffer");

    res_size = ALIGN(VDENC_VP9_BRC_PAK_STATS_BUF_SIZE, 4096);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_brc_pak_stat_buffer_res,
                                res_size,
                                "VDEnc brc pak statistics buffer");
    i965_zero_gpe_resource(&vdenc_context->vdenc_brc_pak_stat_buffer_res);

    res_size = vdenc_context->frame_width_in_sbs * vdenc_context->frame_height_in_sbs * 64;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->segmentid_buffer_res,
                                res_size,
                                "VP9 segment id");

    i965_zero_gpe_resource(&vdenc_context->segmentid_buffer_res);

    vdenc_context->allocate_once_done = 1;

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_allocate_resources(VADriverContextP ctx,
                                   struct encode_state *encode_state,
                                   struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    int i, res_size;
    uint32_t frame_width_in_sbs, frame_height_in_sbs, frame_size_in_sbs;
    unsigned int width, height, pitch, depth_factor = 1;
    char *pbuffer;

    if (!vdenc_context->is_8bit)
        depth_factor = 2;

    gen10_vdenc_vp9_allocate_resources_once(ctx, encode_state, encoder_context);

    /* If the width/height of allocated buffer is greater than the expected,
     * it is unnecessary to allocate it again
     */
    if (vdenc_context->res_width >= vdenc_context->frame_width &&
        vdenc_context->res_height >= vdenc_context->frame_height) {

        return VA_STATUS_SUCCESS;
    }

    frame_width_in_sbs = vdenc_context->frame_width_in_sbs;
    frame_height_in_sbs = vdenc_context->frame_height_in_sbs;
    frame_size_in_sbs = frame_width_in_sbs * frame_height_in_sbs;

    vdenc_context->coding_unit_offset = ALIGN(frame_size_in_sbs * 16, 4096); /* 3 dws for HCP_PAK_OBJECT and 1 dw for padding zero */
    vdenc_context->mb_code_buffer_size = ALIGN(8 * frame_size_in_sbs * (5 + 64 * 8), 4096);
    res_size = vdenc_context->mb_code_buffer_size + 8 * 64;
    i965_free_gpe_resource(&vdenc_context->mb_code_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->mb_code_buffer_res,
                                res_size,
                                "VP9 mb_code surface");

    res_size = frame_width_in_sbs * 64;
    i965_free_gpe_resource(&vdenc_context->hvd_line_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->hvd_line_buffer_res,
                                res_size,
                                "VP9 hvd line line");

    i965_free_gpe_resource(&vdenc_context->hvd_tile_line_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->hvd_tile_line_buffer_res,
                                res_size,
                                "VP9 hvd tile_line line");

    i965_free_gpe_resource(&vdenc_context->deblocking_filter_line_buffer_res);
    res_size = frame_width_in_sbs * (18 * depth_factor) * 64;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->deblocking_filter_line_buffer_res,
                                res_size,
                                "VP9 deblocking filter line");

    i965_free_gpe_resource(&vdenc_context->deblocking_filter_tile_line_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->deblocking_filter_tile_line_buffer_res,
                                res_size,
                                "VP9 deblocking tile line");

    i965_free_gpe_resource(&vdenc_context->deblocking_filter_tile_col_buffer_res);
    res_size = frame_height_in_sbs * (17 * depth_factor) * 64;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->deblocking_filter_tile_col_buffer_res,
                                res_size,
                                "VP9 deblocking tile col");

    res_size = frame_width_in_sbs * 5 * 64;
    i965_free_gpe_resource(&vdenc_context->metadata_line_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->metadata_line_buffer_res,
                                res_size,
                                "VP9 metadata line");

    i965_free_gpe_resource(&vdenc_context->metadata_tile_line_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->metadata_tile_line_buffer_res,
                                res_size,
                                "VP9 metadata tile line");

    res_size = frame_height_in_sbs * 5 * 64;
    i965_free_gpe_resource(&vdenc_context->metadata_tile_col_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->metadata_tile_col_buffer_res,
                                res_size,
                                "VP9 metadata tile col");

    res_size = frame_size_in_sbs * 9 * 64;

    for (i = 0; i < 2; i++) {
        i965_free_gpe_resource(&vdenc_context->mv_temporal_buffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->mv_temporal_buffer_res[i],
                                    res_size,
                                    "VP9 temporal mv");
    }

    res_size = 32 * 64;

    for (i = 0; i < 4; i++) {
        i965_free_gpe_resource(&vdenc_context->prob_buffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->prob_buffer_res[i],
                                    res_size,
                                    "VP9 prob buffer");
    }

    i965_free_gpe_resource(&vdenc_context->prob_delta_buffer_res);
    res_size = 29 * 64;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->prob_delta_buffer_res,
                                res_size,
                                "VP9 prob delta");

    i965_free_gpe_resource(&vdenc_context->compressed_header_buffer_res);
    res_size = 32 * 64;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->compressed_header_buffer_res,
                                res_size,
                                "VP9 compressed input buffer");

    i965_free_gpe_resource(&vdenc_context->prob_counter_buffer_res);
    res_size = 193 * 64;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->prob_counter_buffer_res,
                                res_size,
                                "VP9 prob counter");

    i965_free_gpe_resource(&vdenc_context->tile_record_streamout_buffer_res);
    res_size = frame_size_in_sbs * 64;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->tile_record_streamout_buffer_res,
                                res_size,
                                "VP9 tile record stream_out");

    i965_free_gpe_resource(&vdenc_context->cu_stat_streamout_buffer_res);
    res_size = frame_size_in_sbs * 64 * 8;
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->cu_stat_streamout_buffer_res,
                                res_size,
                                "VP9 CU stat stream_out");

    res_size = MAX(sizeof(struct vdenc_vp9_huc_prob_dmem), sizeof(struct vp9_huc_prob_dmem));
    res_size = ALIGN(res_size, 64);

    for (i = 0; i < 2; i++) {
        i965_free_gpe_resource(&vdenc_context->huc_prob_dmem_buffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_prob_dmem_buffer_res[i],
                                    res_size,
                                    "huc prob dmem");
    }

    res_size = ALIGN(sizeof(vdenc_vp9_key_frame_default_probs) + sizeof(vdenc_vp9_inter_default_probs), 64);
    i965_free_gpe_resource(&vdenc_context->huc_default_prob_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_default_prob_buffer_res,
                                res_size,
                                "huc default prob dmem");

    pbuffer = (char *)i965_map_gpe_resource(&vdenc_context->huc_default_prob_buffer_res);

    if (!pbuffer)
        return VA_STATUS_ERROR_ALLOCATION_FAILED;

    memcpy(pbuffer, vdenc_vp9_key_frame_default_probs, sizeof(vdenc_vp9_key_frame_default_probs));
    pbuffer += sizeof(vdenc_vp9_key_frame_default_probs);
    memcpy(pbuffer, vdenc_vp9_inter_default_probs, sizeof(vdenc_vp9_inter_default_probs));

    i965_unmap_gpe_resource(&vdenc_context->huc_default_prob_buffer_res);

    res_size = 32 * 64;
    i965_free_gpe_resource(&vdenc_context->huc_prob_output_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_prob_output_buffer_res,
                                res_size,
                                "HuC prob output buffer");

    res_size = 80;
    i965_free_gpe_resource(&vdenc_context->huc_pak_insert_uncompressed_header_input_2nd_batchbuffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_pak_insert_uncompressed_header_input_2nd_batchbuffer_res,
                                res_size,
                                "HuC pak insert uncompressed header read buffer");

    res_size = 80;
    i965_free_gpe_resource(&vdenc_context->huc_pak_insert_uncompressed_header_output_2nd_batchbuffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_pak_insert_uncompressed_header_output_2nd_batchbuffer_res,
                                res_size,
                                "HuC pak insert uncompressed header write buffer");

    res_size = 16; /* 4 dws */
    i965_free_gpe_resource(&vdenc_context->huc_status_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_status_buffer_res,
                                res_size,
                                "HuC status");

    width = vdenc_context->down_scaled_width_in_mb4x * 32;
    height = vdenc_context->down_scaled_height_in_mb4x * 40;
    pitch = ALIGN(width, 128);
    i965_free_gpe_resource(&vdenc_context->s4x_memv_data_buffer_res);
    i965_gpe_allocate_2d_resource(i965->intel.bufmgr,
                                  &vdenc_context->s4x_memv_data_buffer_res,
                                  width, height,
                                  ALIGN(width, 64),
                                  "VP9 4x MEMV data");

    width = vdenc_context->down_scaled_width_in_mb4x * 8;
    height = vdenc_context->down_scaled_height_in_mb4x * 40;
    pitch = ALIGN(width, 128);
    i965_free_gpe_resource(&vdenc_context->s4x_memv_distortion_buffer_res);
    i965_gpe_allocate_2d_resource(i965->intel.bufmgr,
                                  &vdenc_context->s4x_memv_distortion_buffer_res,
                                  width, height,
                                  ALIGN(width, 64),
                                  "VP9 4x MEMV distorion");

    width = ALIGN(vdenc_context->down_scaled_width_in_mb16x * 32, 64);
    height = vdenc_context->down_scaled_height_in_mb16x * 40;
    pitch = ALIGN(width, 128);
    i965_free_gpe_resource(&vdenc_context->s16x_memv_data_buffer_res);
    i965_gpe_allocate_2d_resource(i965->intel.bufmgr,
                                  &vdenc_context->s16x_memv_data_buffer_res,
                                  width, height,
                                  pitch,
                                  "VP9 16x MEMV data");

    width = vdenc_context->frame_width_in_mbs * 16;
    height = vdenc_context->frame_height_in_mbs * 8;
    pitch = ALIGN(width, 64);
    i965_free_gpe_resource(&vdenc_context->output_16x16_inter_modes_buffer_res);
    i965_gpe_allocate_2d_resource(i965->intel.bufmgr,
                                  &vdenc_context->output_16x16_inter_modes_buffer_res,
                                  width, height,
                                  ALIGN(width, 64),
                                  "VP9 output inter_mode");

    res_size = vdenc_context->frame_width_in_mbs * vdenc_context->frame_height_in_mbs * 16 * 4;

    for (i = 0; i < 2; i++) {
        i965_free_gpe_resource(&vdenc_context->mode_decision_buffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->mode_decision_buffer_res[i],
                                    res_size,
                                    "VP9 mode decision");
    }

    // buffers for VDEnc
    res_size = ALIGN(VDENC_HCP_VP9_PIC_STATE_SIZE + VDENC_HCP_VP9_SEGMENT_STATE_SIZE * 8 + VDENC_VDENC_CMD0_STATE_SIZE + VDENC_VDENC_CMD1_STATE_SIZE + 8, 4096);

    for (i = 0; i < 4; i++) {
        i965_free_gpe_resource(&vdenc_context->vdenc_pic_state_input_2nd_batchbuffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_pic_state_input_2nd_batchbuffer_res[i],
                                    res_size,
                                    "VDEnc pic state input 2nd level batchbuffer");

        i965_free_gpe_resource(&vdenc_context->vdenc_pic_state_output_2nd_batchbuffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_pic_state_output_2nd_batchbuffer_res[i],
                                    res_size,
                                    "VDEnc pic state output 2nd level batchbuffer");
    }

    i965_free_gpe_resource(&vdenc_context->vdenc_dys_pic_state_2nd_batchbuffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_dys_pic_state_2nd_batchbuffer_res,
                                res_size,
                                "VDEnc dys pic state 2nd level batchbuffer");

    i965_free_gpe_resource(&vdenc_context->vdenc_brc_init_reset_dmem_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_brc_init_reset_dmem_buffer_res,
                                ALIGN(sizeof(struct vdenc_vp9_huc_brc_init_dmem), 64),
                                "brc init&reset dmem buffer");

    for (i = 0; i < VDENC_VP9_BRC_MAX_NUM_OF_PASSES; i++) {
        i965_free_gpe_resource(&vdenc_context->vdenc_brc_update_dmem_buffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_brc_update_dmem_buffer_res[i],
                                    ALIGN(sizeof(struct vdenc_vp9_huc_brc_update_dmem), 64),
                                    "brc update dmem buffer");
    }

    i965_free_gpe_resource(&vdenc_context->vdenc_segment_map_stream_out_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_segment_map_stream_out_buffer_res,
                                frame_size_in_sbs * 64,
                                "VDEnc segment map stream out");

    i965_free_gpe_resource(&vdenc_context->vdenc_sse_src_pixel_row_store_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_sse_src_pixel_row_store_buffer_res,
                                (frame_width_in_sbs + 2) * 32 * 64,
                                "VDEnc sse src pixel row store");
    i965_zero_gpe_resource(&vdenc_context->vdenc_sse_src_pixel_row_store_buffer_res);

    i965_free_gpe_resource(&vdenc_context->vdenc_data_extension_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_data_extension_buffer_res,
                                VDENC_VP9_HUC_DATA_EXTENSION_SIZE,
                                "VDEnc data extension buffer");
    i965_zero_gpe_resource(&vdenc_context->vdenc_data_extension_buffer_res);

    i965_free_gpe_resource(&vdenc_context->vdenc_streamin_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_streamin_buffer_res,
                                (ALIGN(vdenc_context->frame_width, VP9_SUPER_BLOCK_WIDTH) / 32) * (ALIGN(vdenc_context->frame_height, VP9_SUPER_BLOCK_HEIGHT) / 32) * 64,
                                "VDEnc stream in");
    i965_zero_gpe_resource(&vdenc_context->vdenc_streamin_buffer_res);

    i965_free_gpe_resource(&vdenc_context->huc_status2_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_status2_buffer_res,
                                64,
                                "HuC Status 2");

    i965_free_gpe_resource(&vdenc_context->vdenc_row_store_scratch_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->vdenc_row_store_scratch_res,
                                vdenc_context->frame_width_in_mbs * 64,
                                "VDENC row store scratch buffer");

    for (i = 0; i < VDENC_VP9_BRC_MAX_NUM_OF_PASSES; i++) {
        i965_free_gpe_resource(&vdenc_context->huc_initializer_dmem_buffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_initializer_dmem_buffer_res[i],
                                    ALIGN(sizeof(struct huc_initializer_dmem), 64),
                                    "HuC Initializer DMEM buffer");
        i965_zero_gpe_resource(&vdenc_context->huc_initializer_dmem_buffer_res[i]);

        i965_free_gpe_resource(&vdenc_context->huc_initializer_data_buffer_res[i]);
        ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_initializer_data_buffer_res[i],
                                    ALIGN(sizeof(struct huc_initializer_data), 0x1000),
                                    "HuC Initializer Data buffer");
        i965_zero_gpe_resource(&vdenc_context->huc_initializer_data_buffer_res[i]);
    }

    i965_free_gpe_resource(&vdenc_context->huc_initializer_dys_dmem_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_initializer_dys_dmem_buffer_res,
                                ALIGN(sizeof(struct huc_initializer_dmem), 64),
                                "HuC Initializer DYS DMEM buffer");
    i965_zero_gpe_resource(&vdenc_context->huc_initializer_dys_dmem_buffer_res);

    i965_free_gpe_resource(&vdenc_context->huc_initializer_dys_data_buffer_res);
    ALLOC_VDENC_BUFFER_RESOURCE(vdenc_context->huc_initializer_dys_data_buffer_res,
                                ALIGN(sizeof(struct huc_initializer_data), 0x1000),
                                "HuC Initializer DYS data buffer");
    i965_zero_gpe_resource(&vdenc_context->huc_initializer_dys_data_buffer_res);

    if (!vdenc_context->frame_header_data) {
        /* allocate 512 bytes for generating the uncompressed header */
        vdenc_context->frame_header_data = calloc(1, 512);
    }

    vdenc_context->res_width = vdenc_context->frame_width;
    vdenc_context->res_height = vdenc_context->frame_height;

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_uncompressed_header(VADriverContextP ctx,
                                    struct encode_state *encode_state,
                                    struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    VAEncPictureParameterBufferVP9  *pic_param;
    int driver_header_flag = 1; /* '1' means the driver should generate the uncompressed header */

    if (!vdenc_context->pic_param)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    pic_param = vdenc_context->pic_param;

    if (encode_state->packed_header_data_ext &&
        encode_state->packed_header_data_ext[0] &&
        pic_param->bit_offset_first_partition_size) {
        VAEncPackedHeaderParameterBuffer *param = NULL;

        param = (VAEncPackedHeaderParameterBuffer *)encode_state->packed_header_params_ext[0]->buffer;

        if (param->type == VAEncPackedHeaderRawData) {
            char *header_data;
            unsigned int length_in_bits;

            header_data = (char *)encode_state->packed_header_data_ext[0]->buffer;
            length_in_bits = param->bit_length;
            driver_header_flag = 0;

            vdenc_context->frame_header.bit_offset_first_partition_size = pic_param->bit_offset_first_partition_size;
            vdenc_context->frame_header_length = ALIGN(length_in_bits, 8) >> 3;
            vdenc_context->alias_insert_data = header_data;

            vdenc_context->frame_header.bit_offset_ref_lf_delta = pic_param->bit_offset_ref_lf_delta;
            vdenc_context->frame_header.bit_offset_mode_lf_delta = pic_param->bit_offset_mode_lf_delta;
            vdenc_context->frame_header.bit_offset_lf_level = pic_param->bit_offset_lf_level;
            vdenc_context->frame_header.bit_offset_qindex = pic_param->bit_offset_qindex;
            vdenc_context->frame_header.bit_offset_segmentation = pic_param->bit_offset_segmentation;
            vdenc_context->frame_header.bit_size_segmentation = pic_param->bit_size_segmentation;
        }
    }

    if (driver_header_flag) {
        memset(&vdenc_context->frame_header, 0, sizeof(vdenc_context->frame_header));
        intel_write_uncompressed_header(encode_state,
                                        VAProfileVP9Profile0,
                                        vdenc_context->frame_header_data,
                                        &vdenc_context->frame_header_length,
                                        &vdenc_context->frame_header);
        vdenc_context->alias_insert_data = vdenc_context->frame_header_data;
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_check_dys_surfaces(VADriverContextP ctx,
                                   struct object_surface *obj_surface,
                                   uint32_t frame_width,
                                   uint32_t frame_height)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct vdenc_vp9_surface *vdenc_vp9_surface;
    int dys_width_4x, dys_height_4x;
    int dys_width_8x, dys_height_8x;
    int dys_width_16x, dys_height_16x;

    /* As this is handled after the surface checking, it is unnecessary
     * to check the surface bo and vp9_priv_surface again
     */

    vdenc_vp9_surface = (struct vdenc_vp9_surface *)(obj_surface->private_data);

    if (!vdenc_vp9_surface)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    /* if the frame_width/height of dys_surface is the same as
     * the expected, it is unnecessary to allocate it again
     */

    if (vdenc_vp9_surface->dys_frame_width == frame_width &&
        vdenc_vp9_surface->dys_frame_height == frame_height)
        return VA_STATUS_SUCCESS;

    if (vdenc_vp9_surface->dys_surface_obj) {
        i965_DestroySurfaces(vdenc_vp9_surface->ctx, &vdenc_vp9_surface->dys_surface_id, 1);
        vdenc_vp9_surface->dys_surface_id = VA_INVALID_SURFACE;
        vdenc_vp9_surface->dys_surface_obj = NULL;
    }

    if (vdenc_vp9_surface->dys_4x_surface_obj) {
        i965_DestroySurfaces(vdenc_vp9_surface->ctx, &vdenc_vp9_surface->dys_4x_surface_id, 1);
        vdenc_vp9_surface->dys_4x_surface_id = VA_INVALID_SURFACE;
        vdenc_vp9_surface->dys_4x_surface_obj = NULL;
    }

    if (vdenc_vp9_surface->dys_8x_surface_obj) {
        i965_DestroySurfaces(vdenc_vp9_surface->ctx, &vdenc_vp9_surface->dys_8x_surface_id, 1);
        vdenc_vp9_surface->dys_8x_surface_id = VA_INVALID_SURFACE;
        vdenc_vp9_surface->dys_8x_surface_obj = NULL;
    }

    i965_CreateSurfaces(ctx,
                        frame_width,
                        frame_height,
                        VA_RT_FORMAT_YUV420,
                        1,
                        &vdenc_vp9_surface->dys_surface_id);
    vdenc_vp9_surface->dys_surface_obj = SURFACE(vdenc_vp9_surface->dys_surface_id);

    if (!vdenc_vp9_surface->dys_surface_obj) {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    i965_check_alloc_surface_bo(ctx,
                                vdenc_vp9_surface->dys_surface_obj,
                                1,
                                VA_FOURCC('N', 'V', '1', '2'),
                                SUBSAMPLE_YUV420);

    dys_width_4x = ALIGN(frame_width / 4, 16);
    dys_height_4x = ALIGN(frame_height / 4, 16);

    i965_CreateSurfaces(ctx,
                        dys_width_4x,
                        dys_height_4x,
                        VA_RT_FORMAT_YUV420,
                        1,
                        &vdenc_vp9_surface->dys_4x_surface_id);

    vdenc_vp9_surface->dys_4x_surface_obj = SURFACE(vdenc_vp9_surface->dys_4x_surface_id);

    if (!vdenc_vp9_surface->dys_4x_surface_obj) {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    i965_check_alloc_surface_bo(ctx,
                                vdenc_vp9_surface->dys_4x_surface_obj,
                                1,
                                VA_FOURCC('N', 'V', '1', '2'),
                                SUBSAMPLE_YUV420);

    dys_width_8x = ALIGN(frame_width / 8, 16);
    dys_height_8x = ALIGN(frame_height / 8, 16);
    i965_CreateSurfaces(ctx,
                        dys_width_8x,
                        dys_height_8x,
                        VA_RT_FORMAT_YUV420,
                        1,
                        &vdenc_vp9_surface->dys_8x_surface_id);
    vdenc_vp9_surface->dys_8x_surface_obj = SURFACE(vdenc_vp9_surface->dys_8x_surface_id);

    if (!vdenc_vp9_surface->dys_8x_surface_obj) {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    i965_check_alloc_surface_bo(ctx,
                                vdenc_vp9_surface->dys_8x_surface_obj,
                                1,
                                VA_FOURCC('N', 'V', '1', '2'),
                                SUBSAMPLE_YUV420);

    dys_width_16x = ALIGN(frame_width / 16, 16);
    dys_height_16x = ALIGN(frame_height / 16, 16);
    i965_CreateSurfaces(ctx,
                        dys_width_16x,
                        dys_height_16x,
                        VA_RT_FORMAT_YUV420,
                        1,
                        &vdenc_vp9_surface->dys_16x_surface_id);
    vdenc_vp9_surface->dys_16x_surface_obj = SURFACE(vdenc_vp9_surface->dys_16x_surface_id);

    if (!vdenc_vp9_surface->dys_16x_surface_obj) {
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }

    i965_check_alloc_surface_bo(ctx,
                                vdenc_vp9_surface->dys_16x_surface_obj,
                                1,
                                VA_FOURCC('N', 'V', '1', '2'),
                                SUBSAMPLE_YUV420);

    vdenc_vp9_surface->dys_frame_width = frame_width;
    vdenc_vp9_surface->dys_frame_height = frame_height;

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_prepare(VADriverContextP ctx,
                        VAProfile profile,
                        struct encode_state *encode_state,
                        struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct i965_coded_buffer_segment *coded_buffer_segment;
    struct object_surface *obj_surface;
    struct object_buffer *obj_buffer;
    VAStatus va_status;
    VAEncPictureParameterBufferVP9 *pic_param = NULL;
    VDEncVP9Surface *vdenc_vp9_surface;
    dri_bo *bo;
    char *pbuffer;

    va_status = gen10_vdenc_vp9_update_parameters(ctx, profile, encode_state, encoder_context);

    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    pic_param = vdenc_context->pic_param;

    /* Check reconstructed obj */
    obj_surface = encode_state->reconstructed_object;

    if (pic_param->frame_width_dst > obj_surface->orig_width ||
        pic_param->frame_height_dst > obj_surface->orig_height)
        return VA_STATUS_ERROR_INVALID_SURFACE;

    if (!vdenc_context->dys_enabled &&
        ((pic_param->frame_width_src != pic_param->frame_width_dst) ||
         (pic_param->frame_height_src != pic_param->frame_height_dst)))
        return VA_STATUS_ERROR_UNIMPLEMENTED;  // TODO

    i965_check_alloc_surface_bo(ctx, obj_surface, 1, VA_FOURCC_NV12, SUBSAMPLE_YUV420);

    if (obj_surface->private_data == NULL) {
        vdenc_vp9_surface = calloc(sizeof(VDEncVP9Surface), 1);
        assert(vdenc_vp9_surface);

        vdenc_vp9_surface->ctx = ctx;
        vdenc_vp9_surface->frame_width = vdenc_context->frame_width;
        vdenc_vp9_surface->frame_height = vdenc_context->frame_height;

        i965_CreateSurfaces(ctx,
                            vdenc_context->down_scaled_width_4x,
                            vdenc_context->down_scaled_height_4x,
                            VA_RT_FORMAT_YUV420,
                            1,
                            &vdenc_vp9_surface->scaled_4x_surface_id);
        vdenc_vp9_surface->scaled_4x_surface_obj = SURFACE(vdenc_vp9_surface->scaled_4x_surface_id);
        assert(vdenc_vp9_surface->scaled_4x_surface_obj);
        i965_check_alloc_surface_bo(ctx,
                                    vdenc_vp9_surface->scaled_4x_surface_obj,
                                    1,
                                    VA_FOURCC_NV12,
                                    SUBSAMPLE_YUV420);

        i965_CreateSurfaces(ctx,
                            vdenc_context->down_scaled_width_4x >> 1,
                            vdenc_context->down_scaled_height_4x >> 1,
                            VA_RT_FORMAT_YUV420,
                            1,
                            &vdenc_vp9_surface->scaled_8x_surface_id);
        vdenc_vp9_surface->scaled_8x_surface_obj = SURFACE(vdenc_vp9_surface->scaled_8x_surface_id);
        assert(vdenc_vp9_surface->scaled_8x_surface_obj);
        i965_check_alloc_surface_bo(ctx,
                                    vdenc_vp9_surface->scaled_8x_surface_obj,
                                    1,
                                    VA_FOURCC_NV12,
                                    SUBSAMPLE_YUV420);

        i965_CreateSurfaces(ctx,
                            vdenc_context->down_scaled_width_4x >> 2,
                            vdenc_context->down_scaled_height_4x >> 2,
                            VA_RT_FORMAT_YUV420,
                            1,
                            &vdenc_vp9_surface->scaled_16x_surface_id);
        vdenc_vp9_surface->scaled_16x_surface_obj = SURFACE(vdenc_vp9_surface->scaled_16x_surface_id);
        assert(vdenc_vp9_surface->scaled_16x_surface_obj);
        i965_check_alloc_surface_bo(ctx,
                                    vdenc_vp9_surface->scaled_16x_surface_obj,
                                    1,
                                    VA_FOURCC_NV12,
                                    SUBSAMPLE_YUV420);

        obj_surface->private_data = (void *)vdenc_vp9_surface;
        obj_surface->free_private_data = (void *)vdenc_free_vp9_surface;
    }

    vdenc_vp9_surface = (VDEncVP9Surface *)obj_surface->private_data;
    assert(vdenc_vp9_surface->scaled_4x_surface_obj);
    assert(vdenc_vp9_surface->scaled_8x_surface_obj);
    assert(vdenc_vp9_surface->scaled_16x_surface_obj);

    /* Check dys object */
    if (vdenc_context->dys_in_use &&
        (pic_param->frame_width_src != pic_param->frame_width_dst ||
         pic_param->frame_height_src != pic_param->frame_height_dst)) {
        va_status = gen10_vdenc_vp9_check_dys_surfaces(ctx,
                                                       encode_state->reconstructed_object,
                                                       pic_param->frame_width_dst,
                                                       pic_param->frame_height_dst);

        if (va_status)
            return va_status;
    }

    if ((vdenc_context->dys_ref_frame_flag & VP9_REF_LAST) && vdenc_context->last_ref_obj) {
        va_status = gen10_vdenc_vp9_check_dys_surfaces(ctx,
                                                       vdenc_context->last_ref_obj,
                                                       vdenc_context->frame_width,
                                                       vdenc_context->frame_height);

        if (va_status)
            return va_status;
    }

    if ((vdenc_context->dys_ref_frame_flag & VP9_REF_GOLDEN) && vdenc_context->golden_ref_obj) {
        va_status = gen10_vdenc_vp9_check_dys_surfaces(ctx,
                                                       vdenc_context->golden_ref_obj,
                                                       vdenc_context->frame_width,
                                                       vdenc_context->frame_height);

        if (va_status)
            return va_status;
    }

    if ((vdenc_context->dys_ref_frame_flag & VP9_REF_ALT) && vdenc_context->alt_ref_obj) {
        va_status = gen10_vdenc_vp9_check_dys_surfaces(ctx,
                                                       vdenc_context->alt_ref_obj,
                                                       vdenc_context->frame_width,
                                                       vdenc_context->frame_height);

        if (va_status)
            return va_status;
    }

    /* Reconstructed res */
    i965_free_gpe_resource(&vdenc_context->recon_surface_res);
    i965_free_gpe_resource(&vdenc_context->scaled_4x_recon_surface_res);
    i965_free_gpe_resource(&vdenc_context->scaled_8x_recon_surface_res);
    i965_free_gpe_resource(&vdenc_context->scaled_16x_recon_surface_res);
    i965_object_surface_to_2d_gpe_resource(&vdenc_context->recon_surface_res, obj_surface);
    i965_object_surface_to_2d_gpe_resource(&vdenc_context->scaled_4x_recon_surface_res, vdenc_vp9_surface->scaled_4x_surface_obj);
    i965_object_surface_to_2d_gpe_resource(&vdenc_context->scaled_8x_recon_surface_res, vdenc_vp9_surface->scaled_8x_surface_obj);
    i965_object_surface_to_2d_gpe_resource(&vdenc_context->scaled_16x_recon_surface_res, vdenc_vp9_surface->scaled_16x_surface_obj);

    /* Reference res */
    i965_free_gpe_resource(&vdenc_context->last_ref_res);
    i965_free_gpe_resource(&vdenc_context->golden_ref_res);
    i965_free_gpe_resource(&vdenc_context->alt_ref_res);

    if (vdenc_context->last_ref_obj) {
        if (vdenc_context->dys_ref_frame_flag & VP9_REF_LAST) {
            assert(vdenc_context->last_ref_obj->private_data);
            vdenc_vp9_surface = (struct vdenc_vp9_surface *)vdenc_context->last_ref_obj->private_data;
            obj_surface = vdenc_vp9_surface->dys_surface_obj;
            assert(obj_surface);
        } else
            obj_surface = vdenc_context->last_ref_obj;

        i965_object_surface_to_2d_gpe_resource(&vdenc_context->last_ref_res, obj_surface);
    }

    if (vdenc_context->golden_ref_obj) {
        if (vdenc_context->dys_ref_frame_flag & VP9_REF_GOLDEN) {
            assert(vdenc_context->golden_ref_obj->private_data);
            vdenc_vp9_surface = (struct vdenc_vp9_surface *)vdenc_context->golden_ref_obj->private_data;
            obj_surface = vdenc_vp9_surface->dys_surface_obj;
            assert(obj_surface);
        } else
            obj_surface = vdenc_context->golden_ref_obj;

        i965_object_surface_to_2d_gpe_resource(&vdenc_context->golden_ref_res, obj_surface);
    }

    if (vdenc_context->alt_ref_obj) {
        if (vdenc_context->dys_ref_frame_flag & VP9_REF_ALT) {
            assert(vdenc_context->alt_ref_obj->private_data);
            vdenc_vp9_surface = (struct vdenc_vp9_surface *)vdenc_context->alt_ref_obj->private_data;
            obj_surface = vdenc_vp9_surface->dys_surface_obj;
            assert(obj_surface);
        } else
            obj_surface = vdenc_context->alt_ref_obj;

        i965_object_surface_to_2d_gpe_resource(&vdenc_context->alt_ref_res, obj_surface);
    }

    /* Input YUV res */
    i965_free_gpe_resource(&vdenc_context->uncompressed_input_yuv_surface_res);

    if (vdenc_context->dys_in_use &&
        ((pic_param->frame_width_src != pic_param->frame_width_dst) ||
         (pic_param->frame_height_src != pic_param->frame_height_dst))) {
        assert(encode_state->reconstructed_object && encode_state->reconstructed_object->private_data);
        vdenc_vp9_surface = (struct vdenc_vp9_surface *)encode_state->reconstructed_object->private_data;
        obj_surface = vdenc_vp9_surface->dys_surface_obj;
        assert(obj_surface);
    } else {
        obj_surface = encode_state->input_yuv_object;
    }

    i965_object_surface_to_2d_gpe_resource(&vdenc_context->uncompressed_input_yuv_surface_res, obj_surface);

    /* Encoded bitstream */
    obj_buffer = encode_state->coded_buf_object;
    bo = obj_buffer->buffer_store->bo;
    i965_free_gpe_resource(&vdenc_context->compressed_bitstream.res);
    i965_dri_object_to_buffer_gpe_resource(&vdenc_context->compressed_bitstream.res, bo);
    vdenc_context->compressed_bitstream.start_offset = I965_CODEDBUFFER_HEADER_SIZE;
    vdenc_context->compressed_bitstream.end_offset = ALIGN(obj_buffer->size_element - 0x1000, 0x1000);

    /* Status buffer */
    i965_free_gpe_resource(&vdenc_context->status_bffuer.res);
    i965_dri_object_to_buffer_gpe_resource(&vdenc_context->status_bffuer.res, bo);
    vdenc_context->status_bffuer.base_offset = offsetof(struct i965_coded_buffer_segment, codec_private_data);
    vdenc_context->status_bffuer.size = ALIGN(sizeof(struct gen10_vdenc_vp9_status), 64);
    vdenc_context->status_bffuer.bytes_per_frame_offset = offsetof(struct gen10_vdenc_vp9_status, bytes_per_frame);
    assert(vdenc_context->status_bffuer.base_offset + vdenc_context->status_bffuer.size <
           vdenc_context->compressed_bitstream.start_offset);

    dri_bo_map(bo, 1);

    coded_buffer_segment = (struct i965_coded_buffer_segment *)bo->virtual;
    coded_buffer_segment->mapped = 0;
    coded_buffer_segment->codec = encoder_context->codec;
    coded_buffer_segment->status_support = 1;

    pbuffer = bo->virtual;
    pbuffer += vdenc_context->status_bffuer.base_offset;
    memset(pbuffer, 0, vdenc_context->status_bffuer.size);

    dri_bo_unmap(bo);

    va_status = gen10_vdenc_vp9_allocate_resources(ctx, encode_state, encoder_context);

    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    gen10_vdenc_vp9_uncompressed_header(ctx, encode_state, encoder_context);
    gen10_vdenc_vp9_gpe_kernel_init(ctx, encode_state, encoder_context);
    gen10_vdenc_vp9_dys_src_frame(ctx, encode_state, encoder_context);
    gen10_vdenc_vp9_update_streamin_state(ctx, encoder_context);

    return VA_STATUS_SUCCESS;
}

static void
gen10_vdenc_vp9_huc_store_huc_status2(VADriverContextP ctx,
                                      struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct gpe_mi_store_register_mem_parameter mi_store_register_mem_params;
    struct gpe_mi_store_data_imm_parameter mi_store_data_imm_params;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;

    /* Write HUC_STATUS2 mask (1 << 6) */
    memset(&mi_store_data_imm_params, 0, sizeof(mi_store_data_imm_params));
    mi_store_data_imm_params.bo = vdenc_context->huc_status2_buffer_res.bo;
    mi_store_data_imm_params.offset = 0;
    mi_store_data_imm_params.dw0 = (1 << 6);
    gpe->mi_store_data_imm(ctx, batch, &mi_store_data_imm_params);

    /* Store HUC_STATUS2 */
    memset(&mi_store_register_mem_params, 0, sizeof(mi_store_register_mem_params));
    mi_store_register_mem_params.mmio_offset = VCS0_HUC_STATUS2;
    mi_store_register_mem_params.bo = vdenc_context->huc_status2_buffer_res.bo;
    mi_store_register_mem_params.offset = 4;
    gpe->mi_store_register_mem(ctx, batch, &mi_store_register_mem_params);
}

static void
gen10_vdenc_mfx_wait(VADriverContextP ctx,
                     struct encode_state *encode_state,
                     struct intel_encoder_context *encoder_context)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 1);

    OUT_BCS_BATCH(batch, MFX_WAIT);

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_hcp_pipe_mode_select(VADriverContextP ctx,
                                     struct encode_state *encode_state,
                                     struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 6);

    OUT_BCS_BATCH(batch, HCP_PIPE_MODE_SELECT | (6 - 2));
    OUT_BCS_BATCH(batch,
                  (!!vdenc_context->brc_enabled << 12) |        // Frame level
                  (1 << 10) |
                  (HCP_CODEC_VP9 << 5) |
                  (0 << 3) | /* disable Pic Status / Error Report */
                  (!!vdenc_context->brc_enabled << 2) |         // Pipeline
                  HCP_CODEC_SELECT_ENCODE);
    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch, 0);
    // OUT_BCS_BATCH(batch, (1 << 6));  // TODO check it later
    OUT_BCS_BATCH(batch, 0);

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_hcp_surface_state(VADriverContextP ctx,
                                  struct intel_encoder_context *encoder_context,
                                  struct i965_gpe_resource *gpe_resource,
                                  int id)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    if (!gpe_resource->bo)
        return;

    BEGIN_BCS_BATCH(batch, 3);
    OUT_BCS_BATCH(batch, HCP_SURFACE_STATE | (3 - 2));
    OUT_BCS_BATCH(batch,
                  (id << 28) |
                  (gpe_resource->pitch - 1));
    OUT_BCS_BATCH(batch,
                  (SURFACE_FORMAT_PLANAR_420_8 << 28) |
                  (gpe_resource->y_cb_offset));
    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_hcp_pipe_buf_addr_state(VADriverContextP ctx,
                                        struct encode_state *encode_state,
                                        struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    VAEncPictureParameterBufferVP9 *pic_param = vdenc_context->pic_param;
    unsigned int i;

    BEGIN_BCS_BATCH(batch, 104);

    OUT_BCS_BATCH(batch, HCP_PIPE_BUF_ADDR_STATE | (104 - 2));

    /* DW 1..3 decoded surface */
    OUT_BUFFER_3DW(batch, vdenc_context->recon_surface_res.bo, 1, 0, 0);

    /* DW 4..6 deblocking line */
    OUT_BUFFER_3DW(batch, vdenc_context->deblocking_filter_line_buffer_res.bo, 1, 0, 0);

    /* DW 7..9 deblocking tile line */
    OUT_BUFFER_3DW(batch, vdenc_context->deblocking_filter_tile_line_buffer_res.bo, 1, 0, 0);

    /* DW 10..12 deblocking tile col */
    OUT_BUFFER_3DW(batch, vdenc_context->deblocking_filter_tile_col_buffer_res.bo, 1, 0, 0);

    /* DW 13..15 metadata line */
    OUT_BUFFER_3DW(batch, vdenc_context->metadata_line_buffer_res.bo, 1, 0, 0);

    /* DW 16..18 metadata tile line */
    OUT_BUFFER_3DW(batch, vdenc_context->metadata_tile_line_buffer_res.bo, 1, 0, 0);

    /* DW 19..21 metadata tile col */
    OUT_BUFFER_3DW(batch, vdenc_context->metadata_tile_col_buffer_res.bo, 1, 0, 0);

    /* DW 22..30 SAO is not used for VP9 */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW 31..33 Current Motion vector temporal buffer */
    OUT_BUFFER_3DW(batch, vdenc_context->mv_temporal_buffer_res[vdenc_context->curr_mv_temporal_index].bo, 1, 0, 0);

    /* DW 34..36 Not used */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* Only the first three reference_frame is used for VP9 */
    /* DW 37..52 for reference_frame */
    i = 0;

    if (vdenc_context->vp9_frame_type == VP9_FRAME_P) {
        for (i = 0; i < 3; i++) {
            if (encode_state->reference_objects[i] &&
                encode_state->reference_objects[i]->bo)
                OUT_BUFFER_2DW(batch, encode_state->reference_objects[i]->bo, 0, 0);
            else
                OUT_BUFFER_2DW(batch, NULL, 0, 0);
        }
    }

    for (; i < 8; i++) {
        OUT_BUFFER_2DW(batch, NULL, 0, 0);
    }

    OUT_BCS_BATCH(batch, i965->intel.mocs_state);

    /* DW 54..56 for source input */
    OUT_BUFFER_3DW(batch, vdenc_context->uncompressed_input_yuv_surface_res.bo, 0, 0, 0);

    /* DW 57..59 StreamOut is not used */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW 60..62. Not used for encoder */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW 63..65. ILDB Not used for encoder */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW 66..81 For the collocated motion vector temporal buffer */
    if (vdenc_context->vp9_frame_type == VP9_FRAME_P) {
        int prev_index = vdenc_context->curr_mv_temporal_index ^ 0x01;
        OUT_BUFFER_2DW(batch, vdenc_context->mv_temporal_buffer_res[prev_index].bo, 0, 0);
    } else {
        OUT_BUFFER_2DW(batch, NULL, 0, 0);
    }

    for (i = 1; i < 8; i++) {
        OUT_BUFFER_2DW(batch, NULL, 0, 0);
    }

    OUT_BCS_BATCH(batch, i965->intel.mocs_state);

    /* DW 83..85 VP9 prob buffer */
    if (vdenc_context->current_pass == vdenc_context->num_passes - 1)
        OUT_BUFFER_3DW(batch, vdenc_context->huc_prob_output_buffer_res.bo, 1, 0, 0);
    else {
        int frame_ctx = pic_param->pic_flags.bits.frame_context_idx;

        OUT_BUFFER_3DW(batch, vdenc_context->prob_buffer_res[frame_ctx].bo, 1, 0, 0);
    }

    /* DW 86..88 Segment id buffer */
    OUT_BUFFER_3DW(batch, vdenc_context->segmentid_buffer_res.bo, 1, 0, 0);

    /* DW 89..91 HVD line rowstore buffer */
    OUT_BUFFER_3DW(batch, vdenc_context->hvd_line_buffer_res.bo, 1, 0, 0);

    /* DW 92..94 HVD tile line rowstore buffer */
    OUT_BUFFER_3DW(batch, vdenc_context->hvd_tile_line_buffer_res.bo, 1, 0, 0);

    /* DW 95..97 SAO streamout. Not used for VP9 */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW 98..101 Frame Stat Stream out */
    OUT_BUFFER_3DW(batch, vdenc_context->vdenc_brc_pak_stat_buffer_res.bo, 1, 0, 0);

    /* 101..103 */
    OUT_BUFFER_3DW(batch, vdenc_context->vdenc_sse_src_pixel_row_store_buffer_res.bo, 1, 0, 0);

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_hcp_ind_obj_base_addr_state(VADriverContextP ctx, struct encode_state *encode_state, struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 29);

    OUT_BCS_BATCH(batch, HCP_IND_OBJ_BASE_ADDR_STATE | (29 - 2));

    /* indirect bitstream object base */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);
    /* the upper bound of indirect bitstream object */
    OUT_BUFFER_2DW(batch, NULL, 0, 0);

    /* DW 6: Indirect CU object base address, ignored for vdenc */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW 9..11, PAK-BSE */
    OUT_BUFFER_3DW(batch,
                   vdenc_context->compressed_bitstream.res.bo,
                   1,
                   vdenc_context->compressed_bitstream.start_offset,
                   0);
    OUT_BUFFER_2DW(batch,
                   vdenc_context->compressed_bitstream.res.bo,
                   1,
                   vdenc_context->compressed_bitstream.end_offset);

    /* DW 14..16 compressed header buffer */
    OUT_BUFFER_3DW(batch,
                   vdenc_context->compressed_header_buffer_res.bo,
                   0,
                   0,
                   0);

    /* DW 17..19 prob counter streamout */
    OUT_BUFFER_3DW(batch,
                   vdenc_context->prob_counter_buffer_res.bo,
                   1,
                   0,
                   0);

    /* DW 20..22 prob delta streamin */
    OUT_BUFFER_3DW(batch,
                   vdenc_context->prob_delta_buffer_res.bo,
                   0,
                   0,
                   0);

    /* DW 23..25 Tile record streamout */
    OUT_BUFFER_3DW(batch,
                   vdenc_context->tile_record_streamout_buffer_res.bo,
                   1,
                   0,
                   0);

    /* DW 26..28, VP9 PAK CU level Statistic streamout */
    OUT_BUFFER_3DW(batch,
                   vdenc_context->cu_stat_streamout_buffer_res.bo,
                   1,
                   0,
                   0);

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_vdenc_pipe_mode_select(VADriverContextP ctx,
                                       struct encode_state *encode_state,
                                       struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 2);

    OUT_BCS_BATCH(batch, VDENC_PIPE_MODE_SELECT | (2 - 2));
    OUT_BCS_BATCH(batch,
                  (0 << 12) |                   /* TODO, Bit Depth */
                  (0 << 9) |                    /* Must be 0 */
                  (vdenc_context->vdenc_pak_threshold_check_enable << 8) |
                  (1 << 7)  |                   /* Tlb prefetch enable */
                  (vdenc_context->vdenc_pak_object_streamout_enable << 6) |
                  (1 << 5)  |                   /* Frame Statistics Stream-Out Enable */
                  (VDENC_CODEC_VP9 << 0));

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_vdenc_src_surface_state(VADriverContextP ctx,
                                        struct intel_encoder_context *encoder_context,
                                        struct i965_gpe_resource *gpe_resource)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 6);

    OUT_BCS_BATCH(batch, VDENC_SRC_SURFACE_STATE | (6 - 2));
    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch,
                  ((gpe_resource->height - 1) << 18) |
                  ((gpe_resource->width - 1) << 4));
    OUT_BCS_BATCH(batch,
                  (VDENC_SURFACE_PLANAR_420_8 << 28) |  /* 420 planar YUV surface, TODO: 10bits support  */
                  (1 << 27) |                           /* must be 1 for interleave U/V, hardware requirement */
                  ((gpe_resource->pitch - 1) << 3) |    /* pitch */
                  (0 << 2)  |                           /* must be 0 for interleave U/V */
                  (1 << 1)  |                           /* must be tiled */
                  (I965_TILEWALK_YMAJOR << 0));         /* tile walk, TILEWALK_YMAJOR */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource->y_cb_offset));         /* y offset for U(cb) */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource->y_cb_offset));         /* y offset for v(cr) */

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_vdenc_ref_surface_state(VADriverContextP ctx,
                                        struct intel_encoder_context *encoder_context,
                                        struct i965_gpe_resource *gpe_resource)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 6);

    OUT_BCS_BATCH(batch, VDENC_REF_SURFACE_STATE | (6 - 2));
    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch,
                  ((gpe_resource->height - 1) << 18) |
                  ((gpe_resource->width - 1) << 4));
    OUT_BCS_BATCH(batch,
                  (VDENC_SURFACE_PLANAR_420_8 << 28) |  /* 420 planar YUV surface TODO: 10bits support */
                  (1 << 27) |                           /* must be 1 for interleave U/V, hardware requirement */
                  ((gpe_resource->pitch - 1) << 3) |    /* pitch */
                  (0 << 2)  |                           /* must be 0 for interleave U/V */
                  (1 << 1)  |                           /* must be tiled */
                  (I965_TILEWALK_YMAJOR << 0));         /* tile walk, TILEWALK_YMAJOR */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource->y_cb_offset));         /* y offset for U(cb) */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource->y_cb_offset));         /* y offset for v(cr) */

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_vdenc_ds_ref_surface_state(VADriverContextP ctx,
                                           struct intel_encoder_context *encoder_context,
                                           struct i965_gpe_resource *gpe_resource0,
                                           struct i965_gpe_resource *gpe_resource1)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 10);

    OUT_BCS_BATCH(batch, VDENC_DS_REF_SURFACE_STATE | (10 - 2));
    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch,
                  ((gpe_resource0->height - 1) << 18) |
                  ((gpe_resource0->width - 1) << 4));
    OUT_BCS_BATCH(batch,
                  (VDENC_SURFACE_PLANAR_420_8 << 28) |  /* 420 planar YUV surface TODO: 10bits surface */
                  (1 << 27) |                           /* must be 1 for interleave U/V, hardware requirement */
                  ((gpe_resource0->pitch - 1) << 3) |    /* pitch */
                  (0 << 2)  |                           /* must be 0 for interleave U/V */
                  (1 << 1)  |                           /* must be tiled */
                  (I965_TILEWALK_YMAJOR << 0));         /* tile walk, TILEWALK_YMAJOR */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource0->y_cb_offset));         /* y offset for U(cb) */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource0->y_cb_offset));         /* y offset for v(cr) */

    OUT_BCS_BATCH(batch,
                  ((gpe_resource1->height - 1) << 18) |
                  ((gpe_resource1->width - 1) << 4));
    OUT_BCS_BATCH(batch,
                  (VDENC_SURFACE_PLANAR_420_8 << 28) |  /* 420 planar YUV surface TODO: 10bits surface */
                  (1 << 27) |                           /* must be 1 for interleave U/V, hardware requirement */
                  ((gpe_resource1->pitch - 1) << 3) |    /* pitch */
                  (0 << 2)  |                           /* must be 0 for interleave U/V */
                  (1 << 1)  |                           /* must be tiled */
                  (I965_TILEWALK_YMAJOR << 0));         /* tile walk, TILEWALK_YMAJOR */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource1->y_cb_offset));         /* y offset for U(cb) */
    OUT_BCS_BATCH(batch,
                  (0 << 16) |                   /* must be 0 for interleave U/V */
                  (gpe_resource1->y_cb_offset));         /* y offset for v(cr) */

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vdenc_pipe_buf_addr_state(VADriverContextP ctx,
                                      struct encode_state *encode_state,
                                      struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct vdenc_vp9_surface *vdenc_vp9_surface0 = NULL, *vdenc_vp9_surface1 = NULL;

    if (vdenc_context->last_ref_obj)
        vdenc_vp9_surface0 = (struct vdenc_vp9_surface *)(vdenc_context->last_ref_obj->private_data);

    if (vdenc_context->golden_ref_obj)
        vdenc_vp9_surface1 = (struct vdenc_vp9_surface *)(vdenc_context->golden_ref_obj->private_data);

    BEGIN_BCS_BATCH(batch, 62);

    OUT_BCS_BATCH(batch, VDENC_PIPE_BUF_ADDR_STATE | (62 - 2));

    /* DW1-6 for DS 8x REF */
    if (vdenc_vp9_surface0 && vdenc_vp9_surface0->scaled_8x_surface_obj)
        OUT_BUFFER_3DW(batch, vdenc_vp9_surface0->scaled_8x_surface_obj->bo, 0, 0, 0);
    else
        OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    if (vdenc_vp9_surface1 && vdenc_vp9_surface1->scaled_8x_surface_obj)
        OUT_BUFFER_3DW(batch, vdenc_vp9_surface1->scaled_8x_surface_obj->bo, 0, 0, 0);
    else
        OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW7-9 for DS BWD REF0. B-frame is not supported */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW10-12 for uncompressed input data */
    OUT_BUFFER_3DW(batch, vdenc_context->uncompressed_input_yuv_surface_res.bo, 0, 0, 0);

    /* DW13-DW15 for streamin data */
    OUT_BUFFER_3DW(batch, vdenc_context->vdenc_streamin_buffer_res.bo, 0, 0, 0);

    /* DW16-DW18 for row scratch buffer */
    OUT_BUFFER_3DW(batch, vdenc_context->vdenc_row_store_scratch_res.bo, 1, 0, 0);

    /* DW19-DW21, mv temporal buffer */
    if (vdenc_context->vp9_frame_type == VP9_FRAME_P) {
        int prev_index = vdenc_context->curr_mv_temporal_index ^ 0x01;
        OUT_BUFFER_3DW(batch, vdenc_context->mv_temporal_buffer_res[prev_index].bo, 1, 0, 0);
    } else
        OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW22-DW30 for Last, Golden, Alt reference */
    OUT_BUFFER_3DW(batch, vdenc_context->last_ref_res.bo, 0, 0, 0);
    OUT_BUFFER_3DW(batch, vdenc_context->golden_ref_res.bo, 0, 0, 0);
    OUT_BUFFER_3DW(batch, vdenc_context->alt_ref_res.bo, 0, 0, 0);

    /* DW31-DW33 for BDW REF0. Ignored*/
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW34-DW36 for VDEnc statistics streamout */
    OUT_BUFFER_3DW(batch, vdenc_context->vdenc_brc_stat_buffer_res.bo, 1, 0, 0);

    /* DW37..DW42. DS 4x REF */
    if (vdenc_vp9_surface0 && vdenc_vp9_surface0->scaled_4x_surface_obj)
        OUT_BUFFER_3DW(batch, vdenc_vp9_surface0->scaled_4x_surface_obj->bo, 0, 0, 0);
    else
        OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    if (vdenc_vp9_surface1 && vdenc_vp9_surface1->scaled_4x_surface_obj)
        OUT_BUFFER_3DW(batch, vdenc_vp9_surface1->scaled_4x_surface_obj->bo, 0, 0, 0);
    else
        OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW43..DW45. Not used */
    OUT_BUFFER_3DW(batch, NULL, 0, 0, 0);

    /* DW46..DW48. streamout for pak object command */
    OUT_BUFFER_3DW(batch, vdenc_context->mb_code_buffer_res.bo, 1, 0, 0);

    /* DW49..DW51. DS 8x */
    OUT_BUFFER_3DW(batch, vdenc_context->scaled_8x_recon_surface_res.bo, 1, 0, 0);

    /* DW52..DW54. DS 4x */
    OUT_BUFFER_3DW(batch, vdenc_context->scaled_4x_recon_surface_res.bo, 1, 0, 0);

    /* DW55..DW57. segment map streamout */
    OUT_BUFFER_3DW(batch, vdenc_context->vdenc_segment_map_stream_out_buffer_res.bo, 1, 0, 0);

    /* DW58..DW60. */
    OUT_BUFFER_3DW(batch, vdenc_context->vdenc_segment_map_stream_out_buffer_res.bo, 1, 0, 0);

    /* DW 61. */
    OUT_BCS_BATCH(batch, 64 * 3);

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_vdecn_weihgtsoffsets_state(VADriverContextP ctx,
                                           struct encode_state *encode_state,
                                           struct intel_encoder_context *encoder_context)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;

    BEGIN_BCS_BATCH(batch, 5);

    OUT_BCS_BATCH(batch, VDENC_WEIGHTSOFFSETS_STATE | (5 - 2));

    OUT_BCS_BATCH(batch, (0 << 24 |
                          1 << 16 |
                          0 << 8 |
                          1 << 0));
    OUT_BCS_BATCH(batch, (0 << 8 |
                          1 << 0));
    OUT_BCS_BATCH(batch, (0 << 24 |
                          1 << 16 |
                          0 << 8 |
                          1 << 0));
    OUT_BCS_BATCH(batch, (0 << 24 |
                          1 << 16 |
                          0 << 8 |
                          1 << 0));

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_vdenc_walker_state(VADriverContextP ctx,
                                   struct encode_state *encode_state,
                                   struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    VAEncPictureParameterBufferVP9 *pic_param = vdenc_context->pic_param;

    BEGIN_BCS_BATCH(batch, 6);
    OUT_BCS_BATCH(batch, VDENC_WALKER_STATE | (6 - 2));

    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch, ((ALIGN(pic_param->frame_width_dst, 64) / 64) << 16 |
                          (ALIGN(pic_param->frame_height_dst, 64) / 64)));
    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch, 0);
    OUT_BCS_BATCH(batch, pic_param->frame_width_dst - 1);

    ADVANCE_BCS_BATCH(batch);
}

static void
gen10_vdenc_vp9_hcp_vdenc_pipeline(VADriverContextP ctx,
                                   struct encode_state *encode_state,
                                   struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct gpe_mi_batch_buffer_start_parameter mi_batch_buffer_start_params;
    struct gpe_mi_flush_dw_parameter mi_flush_dw_params;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    gen10_vdenc_vd_pipeline_flush_param pipeline_flush_params;

    if (vdenc_context->num_passes > 1 && vdenc_context->current_pass == (vdenc_context->num_passes - 1)) {
        struct gpe_mi_conditional_batch_buffer_end_parameter mi_conditional_batch_buffer_end_params;

        memset(&mi_conditional_batch_buffer_end_params, 0, sizeof(mi_conditional_batch_buffer_end_params));
        mi_conditional_batch_buffer_end_params.bo = vdenc_context->huc_status_buffer_res.bo;
        /* Muse use Mask mode */
        mi_conditional_batch_buffer_end_params.compare_mask_mode_disabled = 0;
        gpe->mi_conditional_batch_buffer_end(ctx, batch, &mi_conditional_batch_buffer_end_params);
    }

    gen10_vdenc_vp9_hcp_pipe_mode_select(ctx, encode_state, encoder_context);

    /* MFX_WAIT to make sure previous operation is done */
    gen10_vdenc_mfx_wait(ctx, encode_state, encoder_context);

    gen10_vdenc_vp9_hcp_surface_state(ctx, encoder_context, &vdenc_context->recon_surface_res, 0);
    gen10_vdenc_vp9_hcp_surface_state(ctx, encoder_context, &vdenc_context->uncompressed_input_yuv_surface_res, 1);
    gen10_vdenc_vp9_hcp_surface_state(ctx, encoder_context, &vdenc_context->last_ref_res, 2);
    gen10_vdenc_vp9_hcp_surface_state(ctx, encoder_context, &vdenc_context->golden_ref_res, 3);
    gen10_vdenc_vp9_hcp_surface_state(ctx, encoder_context, &vdenc_context->alt_ref_res, 4);

    gen10_vdenc_vp9_hcp_pipe_buf_addr_state(ctx, encode_state, encoder_context);
    gen10_vdenc_vp9_hcp_ind_obj_base_addr_state(ctx, encode_state, encoder_context);

    gen10_vdenc_vp9_vdenc_pipe_mode_select(ctx, encode_state, encoder_context);
    gen10_vdenc_vp9_vdenc_src_surface_state(ctx, encoder_context, &vdenc_context->uncompressed_input_yuv_surface_res);

    if (vdenc_context->is_key_frame)
        gen10_vdenc_vp9_vdenc_ref_surface_state(ctx, encoder_context, &vdenc_context->recon_surface_res);
    else {
        struct i965_gpe_resource *ref_gpe_res = &vdenc_context->last_ref_res;

        if (!ref_gpe_res->bo)
            ref_gpe_res = &vdenc_context->golden_ref_res;

        if (!ref_gpe_res->bo)
            ref_gpe_res = &vdenc_context->alt_ref_res;

        assert(ref_gpe_res->bo);
        gen10_vdenc_vp9_vdenc_ref_surface_state(ctx, encoder_context, ref_gpe_res);
    }

    gen10_vdenc_vp9_vdenc_ds_ref_surface_state(ctx, encoder_context, &vdenc_context->scaled_8x_recon_surface_res, &vdenc_context->scaled_4x_recon_surface_res);

    gen10_vdenc_vdenc_pipe_buf_addr_state(ctx, encode_state, encoder_context);

    memset(&mi_batch_buffer_start_params, 0, sizeof(mi_batch_buffer_start_params));
    mi_batch_buffer_start_params.is_second_level = 1; /* Must be the second level batch buffer */

    if (vdenc_context->use_huc)
        mi_batch_buffer_start_params.bo = vdenc_context->vdenc_pic_state_output_2nd_batchbuffer_res[0].bo;
    else {
        if (vdenc_context->dys_ref_frame_flag != VP9_REF_NONE && vdenc_context->dys_multiple_pass_enbaled)
            mi_batch_buffer_start_params.bo = vdenc_context->vdenc_dys_pic_state_2nd_batchbuffer_res.bo;
        else
            mi_batch_buffer_start_params.bo = vdenc_context->vdenc_pic_state_input_2nd_batchbuffer_res[vdenc_context->current_pass].bo;
    }

    gpe->mi_batch_buffer_start(ctx, batch, &mi_batch_buffer_start_params);

    if (vdenc_context->use_huc)
        mi_batch_buffer_start_params.bo = vdenc_context->huc_pak_insert_uncompressed_header_output_2nd_batchbuffer_res.bo;
    else {
        mi_batch_buffer_start_params.bo = vdenc_context->huc_pak_insert_uncompressed_header_input_2nd_batchbuffer_res.bo;
    }

    gpe->mi_batch_buffer_start(ctx, batch, &mi_batch_buffer_start_params);

    gen10_vdenc_vp9_vdecn_weihgtsoffsets_state(ctx, encode_state, encoder_context);
    gen10_vdenc_vp9_vdenc_walker_state(ctx, encode_state, encoder_context);

    memset(&pipeline_flush_params, 0, sizeof(pipeline_flush_params));
    pipeline_flush_params.dw1.mfx_pipeline_done = 1;
    pipeline_flush_params.dw1.hevc_pipeline_done = 1;
    pipeline_flush_params.dw1.hevc_pipeline_flush = 1;
    pipeline_flush_params.dw1.vd_cmd_msg_parser_done = 1;
    gen10_vdenc_vd_pipeline_flush(ctx, batch, &pipeline_flush_params);

    memset(&mi_flush_dw_params, 0, sizeof(mi_flush_dw_params));
    mi_flush_dw_params.video_pipeline_cache_invalidate = 1;
    gpe->mi_flush_dw(ctx, batch, &mi_flush_dw_params);
}

static void
gen10_vdenc_vp9_context_brc_prepare(struct encode_state *encode_state,
                                    struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    unsigned int rate_control_mode = encoder_context->rate_control_mode;

    switch (rate_control_mode & 0x7f) {
    case VA_RC_CBR:
    case VA_RC_VBR:
        assert(0); // TODO add support for CBR
        break;

    case VA_RC_CQP:
    default:
        vdenc_context->internal_rate_mode = I965_BRC_CQP;
        break;
    }

    if (encoder_context->quality_level == 0)
        encoder_context->quality_level = ENCODER_DEFAULT_QUALITY_VP9;
}

static void
gen10_vdenc_vp9_read_status(VADriverContextP ctx, struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct gpe_mi_store_register_mem_parameter mi_store_register_mem_params;
    struct gpe_mi_flush_dw_parameter mi_flush_dw_params;
    struct gpe_mi_copy_mem_parameter mi_copy_mem_mem_params;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    unsigned int base_offset = vdenc_context->status_bffuer.base_offset;

    memset(&mi_store_register_mem_params, 0, sizeof(mi_store_register_mem_params));
    mi_store_register_mem_params.mmio_offset = HCP_VP9_BITSTREAM_BYTECOUNT_FRAME_REG;
    mi_store_register_mem_params.bo = vdenc_context->status_bffuer.res.bo;
    mi_store_register_mem_params.offset = base_offset + vdenc_context->status_bffuer.bytes_per_frame_offset;
    gpe->mi_store_register_mem(ctx, batch, &mi_store_register_mem_params);

    memset(&mi_copy_mem_mem_params, 0, sizeof(mi_copy_mem_mem_params));
    mi_copy_mem_mem_params.src_bo = vdenc_context->status_bffuer.res.bo;
    mi_copy_mem_mem_params.src_offset = base_offset + vdenc_context->status_bffuer.bytes_per_frame_offset;
    mi_copy_mem_mem_params.dst_bo = vdenc_context->brc_bitstream_size_buffer_res.bo;
    mi_copy_mem_mem_params.dst_offset = 0;
    gpe->mi_copy_mem_mem(ctx, batch, &mi_copy_mem_mem_params);

    mi_copy_mem_mem_params.dst_bo = vdenc_context->huc_prob_dmem_buffer_res[1].bo;
    mi_copy_mem_mem_params.dst_offset = offsetof(struct vdenc_vp9_huc_prob_dmem, frame_size);
    gpe->mi_copy_mem_mem(ctx, batch, &mi_copy_mem_mem_params);

    memset(&mi_flush_dw_params, 0, sizeof(mi_flush_dw_params));
    gpe->mi_flush_dw(ctx, batch, &mi_flush_dw_params);
}

static VAStatus
gen10_vdenc_vp9_check_capability(VADriverContextP ctx,
                                 struct encode_state *encode_state,
                                 struct intel_encoder_context *encoder_context)
{
    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_dys_ref_frames(VADriverContextP ctx,
                               struct encode_state *encode_state,
                               struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;

    if (vdenc_context->dys_ref_frame_flag == VP9_REF_NONE)
        return VA_STATUS_SUCCESS;

    if (vdenc_context->current_pass == 0) {
        if (vdenc_context->dys_multiple_pass_enbaled) {
            vdenc_context->vdenc_pak_object_streamout_enable = 1; // TODO, double check this flag
        }
    }

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_fill_pak_insert_object_batchbuffer(VADriverContextP ctx,
                                                   struct encode_state *encode_state,
                                                   struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    int uncompressed_header_length;
    unsigned int *cmd_ptr;
    unsigned int dw_length, bits_in_last_dw;

    if (vdenc_context->current_pass)
        return VA_STATUS_SUCCESS;

    assert(vdenc_context->alias_insert_data);
    uncompressed_header_length = vdenc_context->frame_header_length;
    cmd_ptr = i965_map_gpe_resource(&vdenc_context->huc_pak_insert_uncompressed_header_input_2nd_batchbuffer_res);

    if (!cmd_ptr)
        return VA_STATUS_ERROR_UNKNOWN;

    bits_in_last_dw = uncompressed_header_length % 4;
    bits_in_last_dw *= 8;

    if (bits_in_last_dw == 0)
        bits_in_last_dw = 32;

    /* get the DWORD length of the inserted_data */
    dw_length = ALIGN(uncompressed_header_length, 4) / 4;
    *cmd_ptr++ = HCP_INSERT_PAK_OBJECT | dw_length;

    *cmd_ptr++ = ((0 << 31) | /* indirect payload */
                  (0 << 16) | /* the start offset in first DW */
                  (0 << 15) |
                  (bits_in_last_dw << 8) | /* bits_in_last_dw */
                  (0 << 4) |  /* skip emulation byte count. 0 for VP9 */
                  (0 << 3) |  /* emulation flag. 0 for VP9 */
                  (1 << 2) |  /* last header flag. */
                  (1 << 1));  /* End of slice */
    memcpy(cmd_ptr, vdenc_context->alias_insert_data, dw_length * sizeof(unsigned int));

    cmd_ptr += dw_length;

    *cmd_ptr++ = MI_NOOP;
    *cmd_ptr++ = MI_BATCH_BUFFER_END;
    i965_unmap_gpe_resource(&vdenc_context->huc_pak_insert_uncompressed_header_input_2nd_batchbuffer_res);

    return VA_STATUS_SUCCESS;
}

static void
gen10_vdenc_vp9_fill_hcp_vp9_pic_state(VADriverContextP ctx,
                                       struct encode_state *encode_state,
                                       struct intel_encoder_context *encoder_context,
                                       uint32_t *pcmd)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct object_surface *obj_surface;
    gen10_hcp_vp9_pic_state_param *param;
    VAEncPictureParameterBufferVP9 *pic_param;
    VDEncVP9Surface *vdenc_vp9_surface;
    unsigned int use_prev_frame_mvs;
    unsigned int scaled_h = 0, scaled_w = 0;
    unsigned int ref_frame_width, ref_frame_height;

    if (!vdenc_context->pic_param)
        return;

    pic_param = vdenc_context->pic_param;
    assert(sizeof(*param) == (VDENC_HCP_VP9_PIC_STATE_SIZE - 4));
    *pcmd++ = (HCP_VP9_PIC_STATE | (VDENC_HCP_VP9_PIC_STATE_SIZE_IN_DWS - 2));
    param = (gen10_hcp_vp9_pic_state_param *)pcmd;
    memset(param, 0, sizeof(*param));

    param->dw1.frame_width_in_pixels_minus1 = ALIGN(pic_param->frame_width_dst, 8) - 1;
    param->dw1.frame_height_in_pixels_minus1 = ALIGN(pic_param->frame_height_dst, 8) - 1;

    param->dw2.frame_type = !vdenc_context->is_key_frame;
    param->dw2.adapt_probabilities_flag = (!pic_param->pic_flags.bits.error_resilient_mode && !pic_param->pic_flags.bits.frame_parallel_decoding_mode);
    param->dw2.intra_only_flag = pic_param->pic_flags.bits.intra_only;
    param->dw2.allow_high_precision_mv = pic_param->pic_flags.bits.allow_high_precision_mv;
    param->dw2.motion_comp_filter_type = pic_param->pic_flags.bits.mcomp_filter_type;
    param->dw2.ref_frame_sign_bias_last = pic_param->ref_flags.bits.ref_last_sign_bias;
    param->dw2.ref_frame_sign_bias_golden = pic_param->ref_flags.bits.ref_gf_sign_bias;
    param->dw2.ref_frame_sign_bias_altref = pic_param->ref_flags.bits.ref_arf_sign_bias;
    param->dw2.hybrid_prediction_mode = (pic_param->pic_flags.bits.comp_prediction_mode == 2);
    param->dw2.selectable_tx_mode = (vdenc_context->tx_mode == TX_MODE_SELECT);
    param->dw2.refresh_frame_context = pic_param->pic_flags.bits.refresh_frame_context;
    param->dw2.error_resilient_mode = pic_param->pic_flags.bits.error_resilient_mode;
    param->dw2.frame_parallel_decoding_mode = pic_param->pic_flags.bits.frame_parallel_decoding_mode;
    param->dw2.filter_level = pic_param->filter_level;
    param->dw2.sharpness_level = pic_param->sharpness_level;
    param->dw2.segmentation_enabled = pic_param->pic_flags.bits.segmentation_enabled;
    param->dw2.segmentation_update_map = pic_param->pic_flags.bits.segmentation_update_map;
    param->dw2.segmentation_temporal_update = pic_param->pic_flags.bits.segmentation_temporal_update;
    param->dw2.lossless_mode = pic_param->pic_flags.bits.lossless_mode;

    param->dw3.log2_tile_column = pic_param->log2_tile_columns;
    param->dw3.log2_tile_row = pic_param->log2_tile_rows;
    param->dw3.sse_enable = vdenc_context->brc_enabled;

    if (vdenc_context->vp9_frame_type == VP9_FRAME_P) {
        if (pic_param->pic_flags.bits.error_resilient_mode ||
            vdenc_context->last_frame_status.is_key_frame ||
            vdenc_context->last_frame_status.intra_only ||
            !vdenc_context->last_frame_status.show_frame ||
            (pic_param->frame_width_dst != vdenc_context->last_frame_status.frame_width) ||
            (pic_param->frame_height_dst != vdenc_context->last_frame_status.frame_height))
            use_prev_frame_mvs = 0;
        else
            use_prev_frame_mvs = 1;

        param->dw2.last_frame_type = !vdenc_context->last_frame_status.is_key_frame;
        param->dw2.use_prev_in_find_mv_references = use_prev_frame_mvs;

        /* slot 0 is for last reference */
        obj_surface = encode_state->reference_objects[0];
        scaled_w = 0;
        scaled_h = 0;
        ref_frame_width = 1;
        ref_frame_height = 1;

        if (obj_surface && obj_surface->private_data) {
            vdenc_vp9_surface = obj_surface->private_data;
            ref_frame_width = vdenc_vp9_surface->frame_width;
            ref_frame_height = vdenc_vp9_surface->frame_height;
            scaled_w = (ref_frame_width << 14) / pic_param->frame_width_dst;
            scaled_h = (ref_frame_height << 14) / pic_param->frame_height_dst;
        }

        param->dw4.horizontal_scale_factor_for_last = scaled_w;
        param->dw4.vertical_scale_factor_for_last = scaled_h;
        param->dw7.last_frame_width_in_pixels_minus1 = ref_frame_width - 1;
        param->dw7.last_frame_height_in_pixels_minus1 = ref_frame_height - 1;

        /* slot 1 is for golden reference */
        obj_surface = encode_state->reference_objects[1];
        scaled_w = 0;
        scaled_h = 0;
        ref_frame_width = 1;
        ref_frame_height = 1;

        if (obj_surface && obj_surface->private_data) {
            vdenc_vp9_surface = obj_surface->private_data;
            ref_frame_width = vdenc_vp9_surface->frame_width;
            ref_frame_height = vdenc_vp9_surface->frame_height;
            scaled_w = (ref_frame_width << 14) / pic_param->frame_width_dst;
            scaled_h = (ref_frame_height << 14) / pic_param->frame_height_dst;
        }

        param->dw5.horizontal_scale_factor_for_golden = scaled_w;
        param->dw5.vertical_scale_factor_for_golden = scaled_h;
        param->dw8.golden_frame_width_in_pixels_minus1 = ref_frame_width - 1;
        param->dw8.golden_frame_height_in_pixels_minus1 = ref_frame_height - 1;

        /* slot 2 is for alt reference */
        obj_surface = encode_state->reference_objects[2];
        scaled_w = 0;
        scaled_h = 0;
        ref_frame_width = 1;
        ref_frame_height = 1;

        if (obj_surface && obj_surface->private_data) {
            vdenc_vp9_surface = obj_surface->private_data;
            ref_frame_width = vdenc_vp9_surface->frame_width;
            ref_frame_height = vdenc_vp9_surface->frame_height;
            scaled_w = (ref_frame_width << 14) / pic_param->frame_width_dst;
            scaled_h = (ref_frame_height << 14) / pic_param->frame_height_dst;
        }

        param->dw6.horizontal_scale_factor_for_altref = scaled_w;
        param->dw6.vertical_scale_factor_for_altref = scaled_h;
        param->dw9.altref_frame_width_in_pixels_minus1 = ref_frame_width - 1;
        param->dw9.altref_frame_height_in_pixels_minus1 = ref_frame_height - 1;
    }

    param->dw11.motion_comp_scaling = 1;
    param->dw13.base_qindex = pic_param->luma_ac_qindex;
    param->dw13.header_insertion_enable = 1;

    param->dw14.chroma_ac_qindex_delta = intel_convert_sign_mag(pic_param->chroma_ac_qindex_delta, 5);
    param->dw14.chroma_dc_qindex_delta = intel_convert_sign_mag(pic_param->chroma_dc_qindex_delta, 5);
    param->dw14.luma_dc_qindex_delta = intel_convert_sign_mag(pic_param->luma_dc_qindex_delta, 5);

    param->dw15.lf_ref_delta0 = intel_convert_sign_mag(pic_param->ref_lf_delta[0], 7);
    param->dw15.lf_ref_delta1 = intel_convert_sign_mag(pic_param->ref_lf_delta[1], 7);
    param->dw15.lf_ref_delta2 = intel_convert_sign_mag(pic_param->ref_lf_delta[2], 7);
    param->dw15.lf_ref_delta3 = intel_convert_sign_mag(pic_param->ref_lf_delta[3], 7);

    param->dw16.lf_mode_delta0 = intel_convert_sign_mag(pic_param->mode_lf_delta[0], 7);
    param->dw16.lf_mode_delta1 = intel_convert_sign_mag(pic_param->mode_lf_delta[1], 7);

    param->dw17.bit_offset_for_lf_ref_delta = vdenc_context->frame_header.bit_offset_ref_lf_delta;
    param->dw17.bit_offset_for_mode_delta = vdenc_context->frame_header.bit_offset_mode_lf_delta;

    param->dw18.bit_offset_for_lf_level = vdenc_context->frame_header.bit_offset_lf_level;
    param->dw18.bit_offset_for_qindex = vdenc_context->frame_header.bit_offset_qindex;

    param->dw32.bit_offset_for_first_partition_size = vdenc_context->frame_header.bit_offset_first_partition_size;

    param->dw19.vdenc_pak_only_pass = vdenc_context->pak_only_pass_enabled; // TODO, check the flag

    if (vdenc_context->max_bit_rate || vdenc_context->min_bit_rate) {
        param->dw20.frame_bitrate_max = (vdenc_context->max_bit_rate >> 12);
        param->dw20.frame_bitrate_max_unit = 1;
        param->dw21.frame_bitrate_min = (vdenc_context->min_bit_rate >> 12);
        param->dw21.frame_bitrate_min_unit = 1;
    }
}

static void
gen10_vdenc_vp9_fill_hcp_vp9_segment_state(VADriverContextP ctx,
                                           struct encode_state *encode_state,
                                           struct intel_encoder_context *encoder_context,
                                           VAEncSegParamVP9 *seg_param,
                                           int segment_id,
                                           uint32_t *pcmd)
{
    gen10_hcp_vp9_segment_state_param *param;

    assert(sizeof(*param) == (VDENC_HCP_VP9_SEGMENT_STATE_SIZE - 4));
    *pcmd++ = (HCP_VP9_SEGMENT_STATE | (VDENC_HCP_VP9_SEGMENT_STATE_SIZE_IN_DWS - 2));
    param = (gen10_hcp_vp9_segment_state_param *)pcmd;
    memset(param, 0, sizeof(*param));

    param->dw1.segment_id = segment_id;
    param->dw2.segment_skipped = seg_param->seg_flags.bits.segment_reference_skipped;
    param->dw2.segment_reference = seg_param->seg_flags.bits.segment_reference;
    param->dw2.segment_reference_enabled = seg_param->seg_flags.bits.segment_reference_enabled;

    param->dw7.segment_qindex_delta = intel_convert_sign_mag(seg_param->segment_qindex_delta, 9);
    param->dw7.segment_lf_level_delta = intel_convert_sign_mag(seg_param->segment_lf_level_delta, 7);
}

static void
gen10_vdenc_vp9_fill_mi_batchbuffer_end(VADriverContextP ctx,
                                        struct encode_state *encode_state,
                                        struct intel_encoder_context *encoder_context,
                                        uint32_t *pcmd)
{
    *pcmd++ = MI_BATCH_BUFFER_END;
}

static void
gen10_vdenc_vp9_huc_initializer_init(VADriverContextP ctx,
                                     struct encode_state *encode_state,
                                     struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct i965_gpe_resource *dmem_gpe_resource, *data_gpe_resource;
    struct huc_initializer_dmem *pdmem;
    struct huc_initializer_data *pdata;
    struct huc_initializer_input_command1 input_cmd1;
    VAEncPictureParameterBufferVP9 *pic_param;
    double qp_scale, lambda;
    uint8_t qp;
    int i;

    if (!vdenc_context->pic_param)
        return;

    pic_param = vdenc_context->pic_param;

    if (vdenc_context->dys_in_use) {
        dmem_gpe_resource = &vdenc_context->huc_initializer_dys_dmem_buffer_res;
        data_gpe_resource = &vdenc_context->huc_initializer_dys_data_buffer_res;
    } else {
        dmem_gpe_resource = &vdenc_context->huc_initializer_dmem_buffer_res[vdenc_context->current_pass];
        data_gpe_resource = &vdenc_context->huc_initializer_data_buffer_res[vdenc_context->current_pass];
    }

    pdata = i965_map_gpe_resource(data_gpe_resource);

    if (!pdata)
        return;

    memset(pdata, 0, sizeof(struct huc_initializer_data));
    pdata->total_commands = 2;

    pdata->input_command[0].id = 2;
    pdata->input_command[0].size_of_data = 1;

    if (vdenc_context->vp9_frame_type == VP9_FRAME_I)
        qp_scale = 0.31;
    else
        qp_scale = 0.33;

    qp = pic_param->luma_ac_qindex;
    lambda = qp_scale * vdenc_vp9_quant_ac[qp] / 8;

    pdata->input_command[0].data[0] = (uint32_t)(lambda * 4 + 0.5);

    pdata->input_command[1].id = 1;
    pdata->input_command[1].size_of_data = 0x14;

    memset(&input_cmd1, 0, sizeof(input_cmd1));
    input_cmd1.vdenc_streamin_enabled = (!!vdenc_context->segment_param || vdenc_context->hme_enabled);
    input_cmd1.segment_map_streamin_enabled = !!vdenc_context->segment_param;
    input_cmd1.pak_only_multi_pass_enabled = vdenc_context->pak_only_pass_enabled;

    if (vdenc_context->is_key_frame)
        input_cmd1.num_ref_idx_l0_active_minus1 = 0;
    else
        input_cmd1.num_ref_idx_l0_active_minus1 = vdenc_context->num_ref_frames - 1;;

    input_cmd1.sad_qp_lambda = (uint16_t)(lambda * 4 + 0.5);
    input_cmd1.rd_qp_lambda = (uint16_t)(lambda * lambda * 4 + 0.5);

    input_cmd1.dst_frame_width_minus1 = vdenc_context->frame_width - 1;
    input_cmd1.dst_frame_height_minus1 = vdenc_context->frame_height - 1;
    input_cmd1.segment_enabled = pic_param->pic_flags.bits.segmentation_enabled;
    input_cmd1.prev_frame_segment_enabled = vdenc_context->last_frame_status.segment_enabled;
    input_cmd1.luma_ac_qindex = pic_param->luma_ac_qindex;
    input_cmd1.luma_dc_qindex_delta = pic_param->luma_dc_qindex_delta;

    if (input_cmd1.segment_enabled && vdenc_context->segment_param) {
        VAEncMiscParameterTypeVP9PerSegmantParam *segment_param = vdenc_context->segment_param;
        VAEncSegParamVP9 *seg_param;

        for (i = 0; i < VP9_MAX_SEGMENTS; i++) {
            seg_param = &segment_param->seg_data[i];
            input_cmd1.segment_qindex_delta[i] = seg_param->segment_qindex_delta;
        }
    }

    memcpy(pdata->input_command[1].data, &input_cmd1, sizeof(input_cmd1));
    i965_unmap_gpe_resource(data_gpe_resource);

    pdmem = i965_map_gpe_resource(dmem_gpe_resource);

    if (!pdmem)
        return;

    pdmem->total_output_commands = 2;
    pdmem->codec = 1;
    pdmem->target_usage = encoder_context->quality_level;
    pdmem->frame_type = pic_param->pic_flags.bits.frame_type;

    pdmem->output_command[0].id = 2;
    pdmem->output_command[0].type = 1;
    pdmem->output_command[0].start_in_bytes = 0;

    pdmem->output_command[1].id = 1;
    pdmem->output_command[1].type = 1;
    pdmem->output_command[1].start_in_bytes = 544;

    pdmem->output_size = 544 + 148;

    i965_unmap_gpe_resource(dmem_gpe_resource);
}

static void
gen10_vdenc_vp9_huc_initializer(VADriverContextP ctx,
                                struct encode_state *encode_state,
                                struct intel_encoder_context *encoder_context,
                                struct i965_gpe_resource *pic_state_buffer_res)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct gen10_huc_pipe_mode_select_parameter pipe_mode_select_params;
    struct gen10_huc_imem_state_parameter imem_state_params;
    struct gen10_huc_dmem_state_parameter dmem_state_params;
    struct gen10_huc_virtual_addr_parameter virtual_addr_params;
    struct gen10_huc_start_parameter start_params;
    struct gpe_mi_flush_dw_parameter mi_flush_dw_params;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    gen10_vdenc_vd_pipeline_flush_param pipeline_flush_params;

    memset(&imem_state_params, 0, sizeof(imem_state_params));
    imem_state_params.huc_firmware_descriptor = VDENC_VP9_HUC_INITIALIZER;
    gen10_huc_imem_state(ctx, batch, &imem_state_params);

    memset(&pipe_mode_select_params, 0, sizeof(pipe_mode_select_params));
    gen10_huc_pipe_mode_select(ctx, batch, &pipe_mode_select_params);

    gen10_vdenc_vp9_huc_initializer_init(ctx, encode_state, encoder_context);

    memset(&dmem_state_params, 0, sizeof(dmem_state_params));

    if (vdenc_context->dys_in_use) {
        dmem_state_params.huc_data_source_res = &vdenc_context->huc_initializer_dys_dmem_buffer_res;
    } else {
        dmem_state_params.huc_data_source_res = &vdenc_context->huc_initializer_dmem_buffer_res[vdenc_context->current_pass];
    }

    dmem_state_params.huc_data_destination_base_address = VDENC_VP9_HUC_DMEM_DATA_OFFSET;
    dmem_state_params.huc_data_length = ALIGN(sizeof(struct huc_initializer_dmem), 64);
    gen10_huc_dmem_state(ctx, batch, &dmem_state_params);

    memset(&virtual_addr_params, 0, sizeof(virtual_addr_params));

    if (vdenc_context->dys_in_use)
        virtual_addr_params.regions[0].huc_surface_res = &vdenc_context->huc_initializer_dys_data_buffer_res;
    else
        virtual_addr_params.regions[0].huc_surface_res = &vdenc_context->huc_initializer_data_buffer_res[vdenc_context->current_pass];

    virtual_addr_params.regions[1].huc_surface_res = pic_state_buffer_res;
    virtual_addr_params.regions[1].is_target = 1;
    gen10_huc_virtual_addr_state(ctx, batch, &virtual_addr_params);

    memset(&start_params, 0, sizeof(start_params));
    start_params.last_stream_object = 1;
    gen10_huc_start(ctx, batch, &start_params);

    memset(&pipeline_flush_params, 0, sizeof(pipeline_flush_params));
    pipeline_flush_params.dw1.hevc_pipeline_done = 1;
    pipeline_flush_params.dw1.hevc_pipeline_flush = 1;
    gen10_vdenc_vd_pipeline_flush(ctx, batch, &pipeline_flush_params);

    memset(&mi_flush_dw_params, 0, sizeof(mi_flush_dw_params));
    mi_flush_dw_params.video_pipeline_cache_invalidate = 1;
    gpe->mi_flush_dw(ctx, batch, &mi_flush_dw_params);
}

static VAStatus
gen10_vdenc_vp9_fill_pic_state_batchbuffer(VADriverContextP ctx,
                                           struct encode_state *encode_state,
                                           struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct i965_gpe_resource *gpe_resource;
    VAEncPictureParameterBufferVP9 *pic_param;
    VAEncMiscParameterTypeVP9PerSegmantParam *seg_param, tmp_seg_param;
    char *pdata, *pdata_header;
    int i, segment_count;

    if (!vdenc_context->pic_param)
        return VA_STATUS_ERROR_INVALID_PARAMETER;

    pic_param = vdenc_context->pic_param;

    if ((vdenc_context->dys_ref_frame_flag != VP9_REF_NONE) && vdenc_context->dys_multiple_pass_enbaled)
        gpe_resource = &vdenc_context->vdenc_dys_pic_state_2nd_batchbuffer_res;
    else
        gpe_resource = &vdenc_context->vdenc_pic_state_input_2nd_batchbuffer_res[vdenc_context->current_pass];

    pdata_header = pdata = i965_map_gpe_resource(gpe_resource);

    if (!pdata)
        return VA_STATUS_ERROR_UNKNOWN;

    pdata += VDENC_VDENC_CMD0_STATE_SIZE;
    vdenc_context->pic_state_offset_in_2nd_batchbuffer = pdata - pdata_header;
    gen10_vdenc_vp9_fill_hcp_vp9_pic_state(ctx, encode_state, encoder_context, (uint32_t *)pdata);
    pdata += VDENC_HCP_VP9_PIC_STATE_SIZE;

    seg_param = vdenc_context->segment_param;

    if (pic_param->pic_flags.bits.segmentation_enabled && seg_param)
        segment_count = VP9_MAX_SEGMENTS;
    else {
        segment_count = 1;
        memset(&tmp_seg_param, 0, sizeof(tmp_seg_param));
        seg_param = &tmp_seg_param;
    }

    for (i = 0; i < segment_count; i++) {
        gen10_vdenc_vp9_fill_hcp_vp9_segment_state(ctx, encode_state, encoder_context, &seg_param->seg_data[i], i, (uint32_t *)pdata);
        pdata += VDENC_HCP_VP9_SEGMENT_STATE_SIZE;
    }

    if (segment_count < VP9_MAX_SEGMENTS) {
        memset(pdata, 0, VDENC_HCP_VP9_SEGMENT_STATE_SIZE * (VP9_MAX_SEGMENTS - 1));
        pdata += VDENC_HCP_VP9_SEGMENT_STATE_SIZE * (VP9_MAX_SEGMENTS - 1);
    }

    vdenc_context->cmd1_state_offset_in_2nd_batchbuffer = pdata - pdata_header;
    pdata += VDENC_VDENC_CMD1_STATE_SIZE;

    gen10_vdenc_vp9_fill_mi_batchbuffer_end(ctx, encode_state, encoder_context, (uint32_t *)pdata);
    pdata += VDENC_BATCHBUFFER_END_SIZE;

    vdenc_context->huc_2nd_batchbuffer_size = pdata - pdata_header;

    i965_unmap_gpe_resource(gpe_resource);

    gen10_vdenc_vp9_huc_initializer(ctx,
                                    encode_state,
                                    encoder_context,
                                    gpe_resource);

    return VA_STATUS_SUCCESS;
}

static int
gen10_vdenc_vp9_get_reference_index(uint8_t refresh_flags)
{
    int index = 0;

    if (refresh_flags == 0) {
        return 0;
    }

    refresh_flags = ~refresh_flags;

    while (refresh_flags & 1) {
        refresh_flags >>= 1;
        index++;
    }

    return index;
}

static void
gen10_vdenc_vp9_update_huc_vp9_prob_dmem(VADriverContextP ctx, struct intel_encoder_context *encoder_context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    VAEncPictureParameterBufferVP9 *pic_param = vdenc_context->pic_param;
    VAEncMiscParameterTypeVP9PerSegmantParam *seg_param = vdenc_context->segment_param;
    struct vdenc_vp9_huc_prob_dmem *dmem;
    int i;

    if (vdenc_context->current_pass == 0) {
        dmem = (struct vdenc_vp9_huc_prob_dmem *)i965_map_gpe_resource(&vdenc_context->huc_prob_dmem_buffer_res[1]);

        if (!dmem)
            return;

        memcpy(dmem, vdenc_vp9_huc_prob_dmem_data, sizeof(struct vdenc_vp9_huc_prob_dmem));
        i965_unmap_gpe_resource(&vdenc_context->huc_prob_dmem_buffer_res[1]);

        dmem = (struct vdenc_vp9_huc_prob_dmem *)i965_map_gpe_resource(&vdenc_context->huc_prob_dmem_buffer_res[0]);

        if (!dmem)
            return;

        memcpy(dmem, vdenc_vp9_huc_prob_dmem_data, sizeof(struct vdenc_vp9_huc_prob_dmem));
    } else {
        dmem = (struct vdenc_vp9_huc_prob_dmem *)i965_map_gpe_resource(&vdenc_context->huc_prob_dmem_buffer_res[1]);

        if (!dmem)
            return;
    }

    dmem->huc_pass_num = vdenc_context->is_super_frame_huc_pass ? VDENC_VP9_HUC_SUPERFRAME_PASS : !!vdenc_context->current_pass;
    dmem->frame_width = vdenc_context->frame_width;
    dmem->frame_height = vdenc_context->frame_height;

    if (!seg_param) {
        for (i = 0; i < VP9_MAX_SEGMENTS; i++) {
            dmem->segment_ref[i] = 0xff;
            dmem->segment_skip[i] = 0;
        }
    } else {
        for (i = 0; i < VP9_MAX_SEGMENTS; i++) {
            if (seg_param->seg_data[i].seg_flags.bits.segment_reference_enabled)
                dmem->segment_ref[i] = seg_param->seg_data[i].seg_flags.bits.segment_reference;
            else
                dmem->segment_ref[i] = 0xff;

            dmem->segment_skip[i] = seg_param->seg_data[i].seg_flags.bits.segment_reference_skipped;
        }
    }

    dmem->seg_code_abs = 0;
    dmem->seg_temporal_update = pic_param->pic_flags.bits.segmentation_temporal_update;
    dmem->last_ref_index = pic_param->ref_flags.bits.ref_last_idx;
    dmem->golden_ref_index = pic_param->ref_flags.bits.ref_gf_idx;
    dmem->alt_ref_index = pic_param->ref_flags.bits.ref_arf_idx;
    dmem->refresh_frame_flags = pic_param->refresh_frame_flags;
    dmem->ref_frame_flags = vdenc_context->ref_frame_flag;
    dmem->context_frame_types = vdenc_context->context_frame_types[pic_param->pic_flags.bits.frame_context_idx];
    dmem->frame_to_show = gen10_vdenc_vp9_get_reference_index(dmem->refresh_frame_flags);

    dmem->frame_ctrl.frame_type = pic_param->pic_flags.bits.frame_type;
    dmem->frame_ctrl.show_frame = pic_param->pic_flags.bits.show_frame;
    dmem->frame_ctrl.error_resilient_mode = pic_param->pic_flags.bits.error_resilient_mode;
    dmem->frame_ctrl.intra_only = pic_param->pic_flags.bits.intra_only;
    dmem->frame_ctrl.context_reset = pic_param->pic_flags.bits.reset_frame_context;
    dmem->frame_ctrl.last_ref_frame_bias = pic_param->ref_flags.bits.ref_last_sign_bias;
    dmem->frame_ctrl.golden_ref_frame_bias = pic_param->ref_flags.bits.ref_gf_sign_bias;
    dmem->frame_ctrl.alt_ref_frame_bias = pic_param->ref_flags.bits.ref_arf_sign_bias;
    dmem->frame_ctrl.allow_high_precision_mv = pic_param->pic_flags.bits.allow_high_precision_mv;
    dmem->frame_ctrl.mcomp_filter_mode = pic_param->pic_flags.bits.mcomp_filter_type;
    dmem->frame_ctrl.tx_mode = vdenc_context->tx_mode;
    dmem->frame_ctrl.refresh_frame_context = pic_param->pic_flags.bits.refresh_frame_context;
    dmem->frame_ctrl.frame_parallel_decode = pic_param->pic_flags.bits.frame_parallel_decoding_mode;
    dmem->frame_ctrl.comp_pred_mode = pic_param->pic_flags.bits.comp_prediction_mode;
    dmem->frame_ctrl.frame_context_idx = pic_param->pic_flags.bits.frame_context_idx;
    dmem->frame_ctrl.sharpness_level = pic_param->sharpness_level;
    dmem->frame_ctrl.seg_on = pic_param->pic_flags.bits.segmentation_enabled;
    dmem->frame_ctrl.seg_map_update = pic_param->pic_flags.bits.segmentation_update_map;
    dmem->frame_ctrl.seg_update_data = 0; // TODO
    dmem->frame_ctrl.log2tile_cols = pic_param->log2_tile_columns;
    dmem->frame_ctrl.log2tile_rows = pic_param->log2_tile_rows;

    dmem->streamin_segenable = !!seg_param;
    dmem->streamin_enable = !!seg_param; // TODO
    dmem->prev_frame_info.intra_only = vdenc_context->last_frame_status.intra_only;
    dmem->prev_frame_info.frame_width = vdenc_context->last_frame_status.frame_width;
    dmem->prev_frame_info.frame_height = vdenc_context->last_frame_status.frame_height;
    dmem->prev_frame_info.key_frame = vdenc_context->last_frame_status.is_key_frame;
    dmem->prev_frame_info.show_frame = vdenc_context->last_frame_status.show_frame;
    dmem->repak = (vdenc_context->num_passes > 1 && vdenc_context->current_pass == vdenc_context->num_passes - 1);

    if (dmem->repak && vdenc_context->has_adaptive_repak) {
        // do nothing so far
    }

    /* Must use vdenc_context->frame_header.bit_offset_first_partition_size to calculate dmem->uncomp_hdr_total_length_in_bits */
    dmem->loop_filter_level_bit_offset = vdenc_context->frame_header.bit_offset_lf_level;
    dmem->qindex_bit_offset = vdenc_context->frame_header.bit_offset_qindex;
    dmem->seg_bit_offset = vdenc_context->frame_header.bit_offset_segmentation + 1; // exclude segment_enable bit
    dmem->seg_length_in_bits = vdenc_context->frame_header.bit_size_segmentation - 1; // exclude segment_enable bit
    dmem->uncomp_hdr_total_length_in_bits = vdenc_context->frame_header.bit_offset_first_partition_size + 16;
    dmem->pic_state_offset = vdenc_context->pic_state_offset_in_2nd_batchbuffer;
    dmem->slb_block_size = vdenc_context->huc_2nd_batchbuffer_size;
    dmem->ivf_header_size = (vdenc_context->frame_number == 0) ? 44 : 12;

    i965_unmap_gpe_resource(&vdenc_context->huc_prob_dmem_buffer_res[!!vdenc_context->current_pass]);
}

static void
gen10_vdenc_vp9_huc_vp9_prob(VADriverContextP ctx,
                             struct encode_state *encode_state,
                             struct intel_encoder_context *encoder_context)
{
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct gen10_huc_pipe_mode_select_parameter pipe_mode_select_params;
    struct gen10_huc_imem_state_parameter imem_state_params;
    struct gen10_huc_dmem_state_parameter dmem_state_params;
    struct gen10_huc_virtual_addr_parameter virtual_addr_params;
    struct gen10_huc_start_parameter start_params;
    struct gpe_mi_flush_dw_parameter mi_flush_dw_params;
    struct gpe_mi_store_register_mem_parameter mi_store_register_mem_params;
    struct gpe_mi_store_data_imm_parameter mi_store_data_imm_params;
    VAEncPictureParameterBufferVP9 *pic_param;
    struct i965_gpe_table *gpe = vdenc_context->gpe_table;
    gen10_vdenc_vd_pipeline_flush_param pipeline_flush_params;

    if (!vdenc_context->pic_param)
        return;

    pic_param = vdenc_context->pic_param;

    memset(&imem_state_params, 0, sizeof(imem_state_params));
    imem_state_params.huc_firmware_descriptor = VDENC_VP9_HUC_PROB;
    gen10_huc_imem_state(ctx, batch, &imem_state_params);

    memset(&pipe_mode_select_params, 0, sizeof(pipe_mode_select_params));
    gen10_huc_pipe_mode_select(ctx, batch, &pipe_mode_select_params);

    gen10_vdenc_vp9_update_huc_vp9_prob_dmem(ctx, encoder_context);

    memset(&dmem_state_params, 0, sizeof(dmem_state_params));
    dmem_state_params.huc_data_source_res = &vdenc_context->huc_prob_dmem_buffer_res[!!vdenc_context->current_pass];
    dmem_state_params.huc_data_destination_base_address = VDENC_VP9_HUC_DMEM_DATA_OFFSET;
    dmem_state_params.huc_data_length = ALIGN(sizeof(struct vdenc_vp9_huc_prob_dmem), 64);
    gen10_huc_dmem_state(ctx, batch, &dmem_state_params);

    memset(&virtual_addr_params, 0, sizeof(virtual_addr_params));
    virtual_addr_params.regions[0].huc_surface_res = &vdenc_context->prob_buffer_res[pic_param->pic_flags.bits.frame_context_idx];
    virtual_addr_params.regions[0].is_target = 1;

    virtual_addr_params.regions[1].huc_surface_res = &vdenc_context->prob_counter_buffer_res;

    virtual_addr_params.regions[2].huc_surface_res = &vdenc_context->huc_prob_output_buffer_res;
    virtual_addr_params.regions[2].is_target = 1;

    virtual_addr_params.regions[3].huc_surface_res = &vdenc_context->prob_delta_buffer_res;
    virtual_addr_params.regions[3].is_target = 1;

    virtual_addr_params.regions[4].huc_surface_res = &vdenc_context->huc_pak_insert_uncompressed_header_output_2nd_batchbuffer_res;
    virtual_addr_params.regions[4].is_target = 1;

    virtual_addr_params.regions[5].huc_surface_res = &vdenc_context->compressed_header_buffer_res;
    virtual_addr_params.regions[5].is_target = 1;

    virtual_addr_params.regions[6].huc_surface_res = &vdenc_context->vdenc_pic_state_output_2nd_batchbuffer_res[0];
    virtual_addr_params.regions[6].is_target = 1;

    if (vdenc_context->brc_enabled)
        virtual_addr_params.regions[7].huc_surface_res = &vdenc_context->vdenc_pic_state_output_2nd_batchbuffer_res[0];
    else
        virtual_addr_params.regions[7].huc_surface_res = &vdenc_context->vdenc_pic_state_input_2nd_batchbuffer_res[vdenc_context->current_pass];

    virtual_addr_params.regions[8].huc_surface_res = &vdenc_context->huc_pak_insert_uncompressed_header_input_2nd_batchbuffer_res;

    virtual_addr_params.regions[9].huc_surface_res = &vdenc_context->huc_default_prob_buffer_res;

    virtual_addr_params.regions[10].huc_surface_res = &vdenc_context->compressed_bitstream.res;
    virtual_addr_params.regions[10].offset = vdenc_context->compressed_bitstream.start_offset;
    virtual_addr_params.regions[10].is_target = 1;

    virtual_addr_params.regions[11].huc_surface_res = &vdenc_context->vdenc_data_extension_buffer_res;
    virtual_addr_params.regions[11].is_target = 1;

    gen10_huc_virtual_addr_state(ctx, batch, &virtual_addr_params);

    gen10_vdenc_vp9_huc_store_huc_status2(ctx, encoder_context);

    memset(&start_params, 0, sizeof(start_params));
    start_params.last_stream_object = 1;
    gen10_huc_start(ctx, batch, &start_params);

    memset(&pipeline_flush_params, 0, sizeof(pipeline_flush_params));
    pipeline_flush_params.dw1.hevc_pipeline_done = 1;
    pipeline_flush_params.dw1.hevc_pipeline_flush = 1;
    gen10_vdenc_vd_pipeline_flush(ctx, batch, &pipeline_flush_params);

    memset(&mi_flush_dw_params, 0, sizeof(mi_flush_dw_params));
    mi_flush_dw_params.video_pipeline_cache_invalidate = 1;
    gpe->mi_flush_dw(ctx, batch, &mi_flush_dw_params);

    /* Write HUC_STATUS mask (1 << 31) */
    memset(&mi_store_data_imm_params, 0, sizeof(mi_store_data_imm_params));
    mi_store_data_imm_params.bo = vdenc_context->huc_status_buffer_res.bo;
    mi_store_data_imm_params.offset = 4;
    mi_store_data_imm_params.dw0 = (1 << 31);
    gpe->mi_store_data_imm(ctx, batch, &mi_store_data_imm_params);

    /* Store HUC_STATUS */
    memset(&mi_store_register_mem_params, 0, sizeof(mi_store_register_mem_params));
    mi_store_register_mem_params.mmio_offset = VCS0_HUC_STATUS;
    mi_store_register_mem_params.bo = vdenc_context->huc_status_buffer_res.bo;
    gpe->mi_store_register_mem(ctx, batch, &mi_store_register_mem_params);

    if (vdenc_context->is_super_frame_huc_pass) {
        // TODO, enable for super frame
    }
}

static VAStatus
gen10_vdenc_vp9_encode_picture(VADriverContextP ctx,
                               VAProfile profile,
                               struct encode_state *encode_state,
                               struct intel_encoder_context *encoder_context)
{
    VAStatus va_status;
    struct gen10_vdenc_vp9_context *vdenc_context = encoder_context->mfc_context;
    struct intel_batchbuffer *batch = encoder_context->base.batch;
    VAEncPictureParameterBufferVP9 *pic_param;

    va_status = gen10_vdenc_vp9_check_capability(ctx, encode_state, encoder_context);

    if (va_status != VA_STATUS_SUCCESS)
        return va_status;

    gen10_vdenc_vp9_prepare(ctx, profile, encode_state, encoder_context);

    vdenc_context->submit_batchbuffer = 1;
    vdenc_context->vdenc_pak_object_streamout_enable = 0;

    for (vdenc_context->current_pass = 0; vdenc_context->current_pass < vdenc_context->num_passes; vdenc_context->current_pass++) {
        vdenc_context->is_first_pass = (vdenc_context->current_pass == 0);
        vdenc_context->is_last_pass = (vdenc_context->current_pass == (vdenc_context->num_passes - 1));

        if (vdenc_context->submit_batchbuffer) {
            intel_batchbuffer_start_atomic_bcs_override(batch, 0x1000, BSD_RING0);
            intel_batchbuffer_emit_mi_flush(batch);
        }

        if (vdenc_context->dys_ref_frame_flag == VP9_REF_NONE) {
            if (vdenc_context->current_pass == 0 &&
                vdenc_context->brc_enabled) {
                vdenc_context->vdenc_pak_object_streamout_enable = 1;
            }
        } else {
            gen10_vdenc_vp9_dys_ref_frames(ctx, encode_state, encoder_context);
        }

        gen10_vdenc_vp9_fill_pak_insert_object_batchbuffer(ctx, encode_state, encoder_context);
        gen10_vdenc_vp9_fill_pic_state_batchbuffer(ctx, encode_state, encoder_context);

        intel_batchbuffer_emit_mi_flush(batch);

        if ((vdenc_context->current_pass == 0) || (vdenc_context->current_pass == (vdenc_context->num_passes - 1)))
            gen10_vdenc_vp9_huc_vp9_prob(ctx, encode_state, encoder_context);

        gen10_vdenc_vp9_hcp_vdenc_pipeline(ctx, encode_state, encoder_context);
        gen10_vdenc_vp9_read_status(ctx, encoder_context);

        if (vdenc_context->submit_batchbuffer) {
            intel_batchbuffer_end_atomic(batch);
            intel_batchbuffer_flush(batch);
        }
    }

    assert(vdenc_context->pic_param);
    pic_param = vdenc_context->pic_param;
    vdenc_context->last_frame_status.frame_width = pic_param->frame_width_dst;
    vdenc_context->last_frame_status.frame_height = pic_param->frame_height_dst;
    vdenc_context->last_frame_status.is_key_frame = vdenc_context->is_key_frame;
    vdenc_context->last_frame_status.show_frame = pic_param->pic_flags.bits.show_frame;
    vdenc_context->last_frame_status.refresh_frame_context = pic_param->pic_flags.bits.refresh_frame_context;
    vdenc_context->last_frame_status.frame_context_idx = pic_param->pic_flags.bits.frame_context_idx;
    vdenc_context->last_frame_status.intra_only = (vdenc_context->vp9_frame_type == VP9_FRAME_I);
    vdenc_context->last_frame_status.segment_enabled = !!vdenc_context->segment_param;
    vdenc_context->context_frame_types[pic_param->pic_flags.bits.frame_context_idx] = pic_param->pic_flags.bits.frame_type;
    vdenc_context->frame_number++;
    vdenc_context->curr_mv_temporal_index ^= 1;
    vdenc_context->is_first_frame = 0;

    return VA_STATUS_SUCCESS;
}

static VAStatus
gen10_vdenc_vp9_pipeline(VADriverContextP ctx,
                         VAProfile profile,
                         struct encode_state *encode_state,
                         struct intel_encoder_context *encoder_context)
{
    VAStatus vaStatus;

    switch (profile) {
    case VAProfileVP9Profile0:
        vaStatus = gen10_vdenc_vp9_encode_picture(ctx, profile, encode_state, encoder_context);
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    return vaStatus;
}

static void
gen10_vdenc_vp9_free_resources(struct gen10_vdenc_vp9_context *vdenc_context)
{
    int i;

    /* PAK buffer */
    i965_free_gpe_resource(&vdenc_context->brc_history_buffer_res);
    i965_free_gpe_resource(&vdenc_context->brc_constant_data_buffer_res);
    i965_free_gpe_resource(&vdenc_context->brc_bitstream_size_buffer_res);
    i965_free_gpe_resource(&vdenc_context->brc_huc_data_buffer_res);

    /* HME */
    i965_free_gpe_resource(&vdenc_context->s4x_memv_data_buffer_res);
    i965_free_gpe_resource(&vdenc_context->s4x_memv_distortion_buffer_res);
    i965_free_gpe_resource(&vdenc_context->s16x_memv_data_buffer_res);
    i965_free_gpe_resource(&vdenc_context->output_16x16_inter_modes_buffer_res);

    for (i = 0; i < 2; i++) {
        i965_free_gpe_resource(&vdenc_context->mode_decision_buffer_res[i]);
        i965_free_gpe_resource(&vdenc_context->mv_temporal_buffer_res[i]);
    }

    i965_free_gpe_resource(&vdenc_context->mb_code_buffer_res);
    i965_free_gpe_resource(&vdenc_context->mb_segment_map_buffer_res);

    /* PAK resource */
    i965_free_gpe_resource(&vdenc_context->hvd_line_buffer_res);
    i965_free_gpe_resource(&vdenc_context->hvd_tile_line_buffer_res);
    i965_free_gpe_resource(&vdenc_context->deblocking_filter_line_buffer_res);
    i965_free_gpe_resource(&vdenc_context->deblocking_filter_tile_line_buffer_res);
    i965_free_gpe_resource(&vdenc_context->deblocking_filter_tile_col_buffer_res);

    i965_free_gpe_resource(&vdenc_context->metadata_line_buffer_res);
    i965_free_gpe_resource(&vdenc_context->metadata_tile_line_buffer_res);
    i965_free_gpe_resource(&vdenc_context->metadata_tile_col_buffer_res);

    i965_free_gpe_resource(&vdenc_context->segmentid_buffer_res);

    for (i = 0; i < 4; i++) {
        i965_free_gpe_resource(&vdenc_context->prob_buffer_res[i]);
    }

    i965_free_gpe_resource(&vdenc_context->prob_counter_buffer_res);
    i965_free_gpe_resource(&vdenc_context->prob_delta_buffer_res);

    i965_free_gpe_resource(&vdenc_context->compressed_header_buffer_res);
    i965_free_gpe_resource(&vdenc_context->tile_record_streamout_buffer_res);
    i965_free_gpe_resource(&vdenc_context->cu_stat_streamout_buffer_res);

    /* HuC */
    for (i = 0; i < 2; i++) {
        i965_free_gpe_resource(&vdenc_context->huc_prob_dmem_buffer_res[i]);
    }

    i965_free_gpe_resource(&vdenc_context->huc_default_prob_buffer_res);
    i965_free_gpe_resource(&vdenc_context->huc_prob_output_buffer_res);
    i965_free_gpe_resource(&vdenc_context->huc_pak_insert_uncompressed_header_input_2nd_batchbuffer_res);
    i965_free_gpe_resource(&vdenc_context->huc_pak_insert_uncompressed_header_output_2nd_batchbuffer_res);

    /* VDEnc */
    i965_free_gpe_resource(&vdenc_context->vdenc_row_store_scratch_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_brc_stat_buffer_res);

    for (i = 0; i < 4; i++) {
        i965_free_gpe_resource(&vdenc_context->vdenc_pic_state_input_2nd_batchbuffer_res[i]);
        i965_free_gpe_resource(&vdenc_context->vdenc_pic_state_output_2nd_batchbuffer_res[i]);
    }

    i965_free_gpe_resource(&vdenc_context->vdenc_dys_pic_state_2nd_batchbuffer_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_brc_init_reset_dmem_buffer_res);

    for (i = 0; i < VDENC_VP9_BRC_MAX_NUM_OF_PASSES; i++)
        i965_free_gpe_resource(&vdenc_context->vdenc_brc_update_dmem_buffer_res[i]);

    i965_free_gpe_resource(&vdenc_context->vdenc_segment_map_stream_out_buffer_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_brc_pak_stat_buffer_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_sse_src_pixel_row_store_buffer_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_data_extension_buffer_res);
    i965_free_gpe_resource(&vdenc_context->vdenc_streamin_buffer_res);
    i965_free_gpe_resource(&vdenc_context->huc_status2_buffer_res);
    i965_free_gpe_resource(&vdenc_context->huc_status_buffer_res);

    /* Reconstructed picture */
    i965_free_gpe_resource(&vdenc_context->recon_surface_res);
    i965_free_gpe_resource(&vdenc_context->scaled_4x_recon_surface_res);
    i965_free_gpe_resource(&vdenc_context->scaled_8x_recon_surface_res);
    i965_free_gpe_resource(&vdenc_context->scaled_16x_recon_surface_res);

    /* HuC Initializer */
    for (i = 0; i < VDENC_VP9_BRC_MAX_NUM_OF_PASSES; i++) {
        i965_free_gpe_resource(&vdenc_context->huc_initializer_dmem_buffer_res[i]);
        i965_free_gpe_resource(&vdenc_context->huc_initializer_data_buffer_res[i]);
    }

    i965_free_gpe_resource(&vdenc_context->huc_initializer_dys_dmem_buffer_res);
    i965_free_gpe_resource(&vdenc_context->huc_initializer_dys_data_buffer_res);

    /* Reference */
    i965_free_gpe_resource(&vdenc_context->last_ref_res);
    i965_free_gpe_resource(&vdenc_context->golden_ref_res);
    i965_free_gpe_resource(&vdenc_context->alt_ref_res);

    /* Input YUV */
    i965_free_gpe_resource(&vdenc_context->uncompressed_input_yuv_surface_res);

    /* Compressed bitstream */
    i965_free_gpe_resource(&vdenc_context->compressed_bitstream.res);

    /* Status buffer */
    i965_free_gpe_resource(&vdenc_context->status_bffuer.res);

    free(vdenc_context->frame_header_data);
    vdenc_context->frame_header_data = NULL;
}


static void
gen10_vdenc_vp9_context_destroy(void *context)
{
    struct gen10_vdenc_vp9_context *vdenc_context = context;

    gen10_vdenc_vp9_kernel_context_destroy(vdenc_context);
    gen10_vdenc_vp9_free_resources(vdenc_context);

    free(vdenc_context);
}

static VAStatus
gen10_vdenc_vp9_context_get_status(VADriverContextP ctx,
                                   struct intel_encoder_context *encoder_context,
                                   struct i965_coded_buffer_segment *coded_buffer_segment)
{
    struct gen10_vdenc_vp9_status *vdenc_status = (struct gen10_vdenc_vp9_status *)coded_buffer_segment->codec_private_data;

    coded_buffer_segment->base.size = vdenc_status->bytes_per_frame;

    return VA_STATUS_SUCCESS;
}

Bool
gen10_vdenc_vp9_context_init(VADriverContextP ctx, struct intel_encoder_context *encoder_context)
{
    struct i965_driver_data *i965 = i965_driver_data(ctx);
    struct gen10_vdenc_vp9_context *vdenc_context = calloc(1, sizeof(struct gen10_vdenc_vp9_context));

    if (!vdenc_context)
        return False;

    vdenc_context->gpe_table = &i965->gpe_table;
    vdenc_context->brc_initted = 0;
    vdenc_context->brc_need_reset = 0;
    vdenc_context->current_pass = 0;
    vdenc_context->num_passes = 1;
    vdenc_context->vdenc_pak_threshold_check_enable = 0;
    vdenc_context->dys_enabled = 0; // TODO
    vdenc_context->is_first_frame = 1;
    vdenc_context->has_hme = 0;
    vdenc_context->need_hme = 0;
    vdenc_context->hme_enabled = 0;
    vdenc_context->has_hme_16x = 0;
    vdenc_context->hme_16x_enabled = 0;
    vdenc_context->use_huc = 1;
    vdenc_context->multiple_pass_brc_enabled = 0;
    vdenc_context->has_adaptive_repak = 0;
    vdenc_context->use_hw_scoreboard = 1;
    vdenc_context->use_hw_non_stalling_scoreborad = 1; /* default: non-stalling */

    gen10_vdenc_vp9_gpe_context_init(ctx, encoder_context, vdenc_context);

    encoder_context->mfc_context = vdenc_context;
    encoder_context->mfc_context_destroy = gen10_vdenc_vp9_context_destroy;
    encoder_context->mfc_pipeline = gen10_vdenc_vp9_pipeline;
    encoder_context->mfc_brc_prepare = gen10_vdenc_vp9_context_brc_prepare;
    encoder_context->get_status = gen10_vdenc_vp9_context_get_status;

    return True;
}
