/*******************************************************************************

 * Dolby Lightscapes GStreamer Plugins
 * Copyright (C) 2024, Dolby Laboratories

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/

#ifndef DLB_LIGHTSCAPES_H_
#define DLB_LIGHTSCAPES_H_

#include <stddef.h>
#include <stdint.h>

typedef struct dlb_lsr_s dlb_lsr;

typedef struct dlb_lsr_init_info_s
{
    const unsigned char *serialized_conf;
    size_t serialized_conf_size;
    unsigned color_space;
    unsigned max_num_objs;
    unsigned max_num_md;
    unsigned frame_period_us;
} dlb_lsr_init_info;

typedef enum dlb_lsr_error_code_e {
    DLB_LSR_NO_ERROR = 0,
    DLB_LSR_ERROR_MEMORY,
    DLB_LSR_ERROR_INIT,
} dlb_lsr_error_code;

#define LS_OUTPUT_COLOR_FORMAT_RGB      (0)
#define LS_OUTPUT_COLOR_FORMAT_RGBW     (10)
#define LS_OUTPUT_COLOR_FORMAT_RGBWW    (11)

dlb_lsr *   dlb_lsr_new                 (const dlb_lsr_init_info *info);
void        dlb_lsr_free                (dlb_lsr *self);
void        dlb_lsr_process             (dlb_lsr  *self
                                        ,size_t inbuf_size
                                        ,unsigned char *inbuf
                                        ,size_t *outbuf_size
                                        ,unsigned char *outbuf);
void        dlb_lsr_reset               (dlb_lsr *self);

size_t      dlb_lsr_get_max_output_size (dlb_lsr *self);

#endif // DLB_LIGHTSCAPES_H_
