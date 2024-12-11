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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlb_lightscapes.h"
#include "common_shim.h"

typedef struct dlb_lightscapes_dispatch_table_s
{
  dlb_lsr *   (*new) (const dlb_lsr_init_info *info);
  void        (*free) (dlb_lsr *self);
  void        (*process) (dlb_lsr *self, size_t inbuf_size, unsigned char *inbuf, size_t *outbuf_size, unsigned char *outbuf);
  void        (*reset) (dlb_lsr *self);
  size_t      (*get_max_output_size) (dlb_lsr *self);
} dlb_lightscapes_dispatch_table;

static dlb_lightscapes_dispatch_table dispatch_table;

int
dlb_lightscapes_try_open_dynlib (void)
{
  void *liblightscapes = open_dynamic_lib (DLB_LIGHTSCAPES_LIBNAME);
  if (!liblightscapes)
    return 1;

  dispatch_table.new = get_proc_address (liblightscapes, "dlb_lsr_new");
  dispatch_table.free = get_proc_address (liblightscapes, "dlb_lsr_free");
  dispatch_table.process = get_proc_address (liblightscapes, "dlb_lsr_process");
  dispatch_table.reset = get_proc_address (liblightscapes, "dlb_lsr_reset");
  dispatch_table.get_max_output_size = get_proc_address (liblightscapes, "dlb_lsr_get_max_output_size");

  return 0;
}

dlb_lsr *
dlb_lsr_new (const dlb_lsr_init_info * info)
{
  return dispatch_table.new (info);
}

void
dlb_lsr_free (dlb_lsr * self)
{
  dispatch_table.free (self);
}

void
dlb_lsr_process (dlb_lsr * self, size_t inbuf_size, unsigned char *inbuf, size_t *outbuf_size, unsigned char *outbuf)
{
  dispatch_table.process (self, inbuf_size, inbuf, outbuf_size, outbuf);
}

void
dlb_lsr_reset (dlb_lsr * self)
{
  dispatch_table.reset (self);
}

size_t
dlb_lsr_get_max_output_size (dlb_lsr * self)
{
  return dispatch_table.get_max_output_size (self);
}
