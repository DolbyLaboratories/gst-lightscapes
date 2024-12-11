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

#ifndef _DLB_LIGHTNING_H
#define _DLB_LIGHTNING_H

#include <gst/base/gstbasetransform.h>
#include "dlb_lightscapes.h"

G_BEGIN_DECLS
#define DLB_TYPE_LIGHTNING   (dlb_lightning_get_type())
#define DLB_LIGHTNING(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_LIGHTNING,DlbLightning))
#define DLB_LIGHTNING_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),DLB_TYPE_LIGHTNING,DlbLightningClass))
#define DLB_IS_LIGHTNING(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_LIGHTNING))
#define DLB_IS_LIGHTNING_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),DLB_TYPE_LIGHTNING))
typedef struct _DlbLightning DlbLightning;
typedef struct _DlbLightningClass DlbLightningClass;

struct _DlbLightning
{
  GstBaseTransform base_lightning;

  /* Pointer to lightscapes state */
  dlb_lsr *renderer_instance;
  dlb_lsr_init_info renderer_config;
  size_t max_output_size;

  /* config */
  gchar *config_path;
};

struct _DlbLightningClass
{
  GstBaseTransformClass base_lightning_class;
};

GType dlb_lightning_get_type (void);

G_END_DECLS
#endif
