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

// This is derived from gstvideosink.c and gstaudiobasesink.c

#include "dlblightbasesink.h"

GST_DEBUG_CATEGORY_STATIC (dlb_light_base_sink_debug);
#define GST_CAT_DEFAULT dlb_light_base_sink_debug

enum
{
  PROP_SHOW_PREROLL_FRAME = 1
};

#define DEFAULT_SHOW_PREROLL_FRAME TRUE

struct _DlbLightBaseSinkPrivate
{
  gboolean show_preroll_frame;  /* ATOMIC */
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (dlb_light_base_sink_debug, "lightbasesink", 0, "lightbasesink element");
G_DEFINE_TYPE_WITH_CODE (DlbLightBaseSink, dlb_light_base_sink, GST_TYPE_BASE_SINK, G_ADD_PRIVATE (DlbLightBaseSink) _do_init);

static void dlb_light_base_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void dlb_light_base_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn dlb_light_base_sink_show_preroll_frame (GstBaseSink * bsink, GstBuffer * buf);
static GstFlowReturn dlb_light_base_sink_show_frame (GstBaseSink * bsink, GstBuffer * buf);
static void dlb_light_base_sink_get_times (GstBaseSink * bsink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);

/* Initing stuff */

static void
dlb_light_base_sink_init (DlbLightBaseSink * lightsink)
{
  /* 20ms is more than enough, 80-130ms is noticeable */
  gst_base_sink_set_processing_deadline (GST_BASE_SINK (lightsink),
      15 * GST_MSECOND);
  gst_base_sink_set_max_lateness (GST_BASE_SINK (lightsink), 5 * GST_MSECOND);
  gst_base_sink_set_qos_enabled (GST_BASE_SINK (lightsink), TRUE);

  lightsink->priv = dlb_light_base_sink_get_instance_private (lightsink);
}

static void
dlb_light_base_sink_class_init (DlbLightBaseSinkClass * klass)
{
  GstBaseSinkClass *basesink_class = (GstBaseSinkClass *) klass;
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = dlb_light_base_sink_set_property;
  gobject_class->get_property = dlb_light_base_sink_get_property;

  /**
   * DlbLightBaseSink:show-preroll-frame:
   *
   * Whether to show light frames during preroll. If set to %FALSE, light
   * frames will only be rendered in PLAYING state.
   */
  g_object_class_install_property (gobject_class, PROP_SHOW_PREROLL_FRAME,
      g_param_spec_boolean ("show-preroll-frame", "Show preroll frame",
          "Whether to render light frames during preroll",
          DEFAULT_SHOW_PREROLL_FRAME,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  basesink_class->render = GST_DEBUG_FUNCPTR (dlb_light_base_sink_show_frame);
  basesink_class->preroll = GST_DEBUG_FUNCPTR (dlb_light_base_sink_show_preroll_frame);
  basesink_class->get_times = GST_DEBUG_FUNCPTR (dlb_light_base_sink_get_times);
}

static void
dlb_light_base_sink_get_times (GstBaseSink * bsink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstClockTime timestamp;

  timestamp = GST_BUFFER_DTS_OR_PTS (buffer);
  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    *start = timestamp;
    if (GST_BUFFER_DURATION_IS_VALID (buffer)) {
      *end = timestamp + GST_BUFFER_DURATION (buffer);
    } else if (bsink->segment.rate < 0) {
      /* The end time will be used for clock waiting time position
       * in case of revese playback, and unknown end time will result in
       * never waiting for clock (as if sync=false).
       * Returning timestamp here would be the best effort we can do */
      *end = timestamp;
    }
  }
}

static GstFlowReturn
dlb_light_base_sink_show_preroll_frame (GstBaseSink * bsink, GstBuffer * buf)
{
  GstBaseSinkClass *bclass;
  DlbLightBaseSinkClass *klass;
  DlbLightBaseSink *lsink;
  gboolean do_show;

  bclass = GST_BASE_SINK_GET_CLASS(bsink);
  lsink = DLB_LIGHT_BASE_SINK_CAST (bsink);
  klass = DLB_LIGHT_BASE_SINK_GET_CLASS (lsink);

  do_show = g_atomic_int_get (&lsink->priv->show_preroll_frame);

  if (G_UNLIKELY (!do_show)) {
    GST_DEBUG_OBJECT (bsink, "not rendering frame with ts=%" GST_TIME_FORMAT
        ", preroll rendering disabled",
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));
  }

  if (klass->show_frame == NULL || !do_show) {
    if (bclass->preroll != NULL)
      return bclass->preroll (bsink, buf);
    else
      return GST_FLOW_OK;
  }

  GST_LOG_OBJECT (bsink, "rendering frame, ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

  return klass->show_frame (DLB_LIGHT_BASE_SINK_CAST (bsink), buf);
}

static GstFlowReturn
dlb_light_base_sink_show_frame (GstBaseSink * bsink, GstBuffer * buf)
{
  GstBaseSinkClass *bclass;
  DlbLightBaseSinkClass *klass;

  bclass = GST_BASE_SINK_GET_CLASS(bsink);
  klass = DLB_LIGHT_BASE_SINK_GET_CLASS (bsink);

  if (klass->show_frame == NULL) {
    if (bclass->render != NULL)
      return bclass->render (bsink, buf);
    else
      return GST_FLOW_OK;
  }

  GST_LOG_OBJECT (bsink, "rendering frame, ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

  return klass->show_frame (DLB_LIGHT_BASE_SINK_CAST (bsink), buf);
}

static void
dlb_light_base_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbLightBaseSink *lsink;

  lsink = DLB_LIGHT_BASE_SINK (object);

  switch (prop_id) {
    case PROP_SHOW_PREROLL_FRAME:
      g_atomic_int_set (&lsink->priv->show_preroll_frame,
          g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
dlb_light_base_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  DlbLightBaseSink *lsink;

  lsink = DLB_LIGHT_BASE_SINK (object);

  switch (prop_id) {
    case PROP_SHOW_PREROLL_FRAME:
      g_value_set_boolean (value,
          g_atomic_int_get (&lsink->priv->show_preroll_frame));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}