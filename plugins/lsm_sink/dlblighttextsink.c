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

#include "dlblighttextsink.h"
#include "dlb_lightscapes.h"

#include <assert.h>
#include <gst/base/gstbytereader.h>
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

GST_DEBUG_CATEGORY_STATIC (dlb_light_text_sink_debug_category);
#define GST_CAT_DEFAULT dlb_light_text_sink_debug_category

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (DlbLightTextSink, dlb_light_text_sink, DLB_TYPE_LIGHT_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT (dlb_light_text_sink_debug_category, "dlblighttextsink", 0,
        "debug category for dlb_light_text_sink element"));
GST_ELEMENT_REGISTER_DEFINE (dlblighttextsink, "dlblighttextsink", GST_RANK_SECONDARY,
                             DLB_TYPE_LIGHT_TEXT_SINK);

static GstStaticPadTemplate dlb_light_text_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-lights, "
                     " format = (string) { DLB }; ")
    );

static GstFlowReturn
dlb_light_text_sink_show_frame (DlbLightBaseSink * lsink, GstBuffer * buf)
{
  GstByteReader reader;
  GstMapInfo map;
  DlbLightTextSink *self = DLB_LIGHT_TEXT_SINK (lsink);
  GST_DEBUG_OBJECT (self, "show frame");

  if (!buf)
    return GST_FLOW_ERROR;

  gst_buffer_map (buf, &map, GST_MAP_READ);
  gst_byte_reader_init (&reader, map.data, map.size);

  uint16_t num_light_arrs;
  assert(gst_byte_reader_get_uint16_le(&reader, &num_light_arrs));
  GST_INFO_OBJECT(self, "[LSM] # light arrays: %u", num_light_arrs);
  for (uint16_t i = 0; i < num_light_arrs; i++) {
      uint8_t strip_id, out_fmt;
      uint16_t strip_lights;
      assert(gst_byte_reader_get_uint8(&reader, &strip_id) &&
             gst_byte_reader_get_uint16_le(&reader, &strip_lights) &&
             gst_byte_reader_get_uint8(&reader, &out_fmt));
      GST_INFO_OBJECT(self, "[LSM] Strip %u [%u lights, fmt %u]:", strip_id, strip_lights, out_fmt);
      for (uint16_t j = 0; j < strip_lights; j++) {
          uint8_t r, g, b;
          if (out_fmt == LS_OUTPUT_COLOR_FORMAT_RGB) {
              assert(gst_byte_reader_get_uint8(&reader, &r) &&
                     gst_byte_reader_get_uint8(&reader, &g) &&
                     gst_byte_reader_get_uint8(&reader, &b));
          } else {
              GST_INFO_OBJECT(self, "[LSM]     UNKNOWN OUTPUT FORMAT");
              continue;
          }
          GST_INFO_OBJECT(self, "[LSM]     %3u %3u %3u", r, g, b);
      }
  }

  gst_buffer_unmap (buf, &map);

  return GST_FLOW_OK;
}

static void
dlb_light_text_sink_init (DlbLightTextSink * sink)
{
    GST_DEBUG_OBJECT (sink, "init");
}

static void
dlb_light_text_sink_class_init (DlbLightTextSinkClass * klass)
{
  DlbLightBaseSinkClass *lightsink_class = DLB_LIGHT_BASE_SINK_CLASS (klass);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Dolby LSM Text Sink",
      "Sink/Light",
      "Driver for writing light outputs to gst INFO log",
      "Dolby Support <support@dolby.com>");

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_light_text_sink_template);

  lightsink_class->show_frame = dlb_light_text_sink_show_frame;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return GST_ELEMENT_REGISTER (dlblighttextsink, plugin);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlblighttextsink,
    "Dolby Text Light Sink",
    plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
