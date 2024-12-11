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

#ifndef _DLB_LIGHT_BASE_SINK_H_
#define _DLB_LIGHT_BASE_SINK_H_

// This is derived from gstaudiobasesinksink.h and gstaudiobasesink.h

#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define DLB_TYPE_LIGHT_BASE_SINK                (dlb_light_base_sink_get_type())
#define DLB_LIGHT_BASE_SINK(obj)                (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_LIGHT_BASE_SINK,DlbLightBaseSink))
#define DLB_LIGHT_BASE_SINK_CAST(obj)           ((DlbLightBaseSink*)obj)
#define DLB_LIGHT_BASE_SINK_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST((klass),DLB_TYPE_LIGHT_BASE_SINK,DlbLightBaseSinkClass))
#define DLB_LIGHT_BASE_SINK_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), DLB_TYPE_LIGHT_BASE_SINK, DlbLightBaseSinkClass))
#define DLB_IS_LIGHT_BASE_SINK(obj)             (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_LIGHT_BASE_SINK))
#define DLB_IS_LIGHT_BASE_SINK_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE((klass),DLB_TYPE_LIGHT_BASE_SINK))

/**
 * DLB_LIGHT_BASE_SINK_PAD:
 * @obj: a #DlbLightBaseSink
 *
 * Get the sink #GstPad of @obj.
 */
#define DLB_LIGHT_BASE_SINK_PAD(obj)     (GST_BASE_SINK (obj)->sinkpad)

typedef struct _DlbLightBaseSink DlbLightBaseSink;
typedef struct _DlbLightBaseSinkClass DlbLightBaseSinkClass;
typedef struct _DlbLightBaseSinkPrivate DlbLightBaseSinkPrivate;

/**
 * DlbLightBaseSink:
 *
 * Opaque #DlbLightBaseSink.
 */
struct _DlbLightBaseSink {
  GstBaseSink         element;

  /*< private >*/
  DlbLightBaseSinkPrivate *priv;

  gpointer _gst_reserved[GST_PADDING];
};

/**
 * DlbLightBaseSinkClass:
 * @parent_class: the parent class.
 *
 * #DlbLightBaseSink class. Override the vmethod to implement
 * functionality.
 */
struct _DlbLightBaseSinkClass {
  GstBaseSinkClass     parent_class;

  GstFlowReturn  (*show_frame) (DlbLightBaseSink *light_sink, GstBuffer *buf);

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

#define DLB_LIGHT_API GST_API_EXPORT

GST_API_EXPORT
GType dlb_light_base_sink_get_type(void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(DlbLightBaseSink, gst_object_unref)

G_END_DECLS

#endif // _DLB_LIGHT_BASE_SINK_H_
