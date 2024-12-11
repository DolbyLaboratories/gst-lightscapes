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

#ifndef _DLB_LIGHT_TEXT_SINK_H_
#define _DLB_LIGHT_TEXT_SINK_H_

#include "dlblightbasesink.h"

G_BEGIN_DECLS

#define DLB_TYPE_LIGHT_TEXT_SINK \
  (dlb_light_text_sink_get_type())
#define DLB_LIGHT_TEXT_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), DLB_TYPE_LIGHT_TEXT_SINK, DlbLightTextSink))
#define GST_DLB_LIGHT_TEXT_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), DLB_TYPE_LIGHT_TEXT_SINK, DlbLightTextSinkClass))
#define DLB_IS_LIGHT_TEXT_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), DLB_TYPE_LIGHT_TEXT_SINK))
#define DLB_IS_LIGHT_TEXT_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), DLB_TYPE_LIGHT_TEXT_SINK))

typedef struct _DlbLightTextSink DlbLightTextSink;
typedef struct _DlbLightTextSinkClass DlbLightTextSinkClass;

struct _DlbLightTextSink {
  DlbLightBaseSink lightsink;
};

struct _DlbLightTextSinkClass {
  DlbLightBaseSinkClass parent_class;
};

GType dlb_light_text_sink_get_type (void);
GST_ELEMENT_REGISTER_DECLARE (dlblighttextsink);
G_END_DECLS

#endif // _DLB_LIGHT_TEXT_SINK_H_
