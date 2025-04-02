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

#include "dlblsmparse.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbaseparse.h>
#include <gst/base/base.h>

GST_DEBUG_CATEGORY_STATIC (dlb_lsm_parse_debug_category);
#define GST_CAT_DEFAULT dlb_lsm_parse_debug_category


/* prototypes */
static gboolean dlb_lsm_parse_start (GstBaseParse * parse);
static gboolean dlb_lsm_parse_stop (GstBaseParse * parse);
static GstFlowReturn dlb_lsm_parse_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize);

/* pad templates */
static GstStaticPadTemplate dlb_lsm_parse_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-lsm, parsed = (boolean) true, "
        " lsm-version = (int) { 0 }, "
        " max-objects = (int) [ 1, 255 ], "
        " frame-period = (int) { 40000 }, "
        " color-space = (int) { 0, 1 }; ")
    );

static GstStaticPadTemplate dlb_lsm_parse_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/octet-stream, uri = (string) { urn:oid:1.2.6.1.4.6729.1.3 }; ")
    );


/* class initialization */
G_DEFINE_TYPE_WITH_CODE (DlbLsmParse, dlb_lsm_parse, GST_TYPE_BASE_PARSE,
    GST_DEBUG_CATEGORY_INIT (dlb_lsm_parse_debug_category, "dlblsmparse", 0,
        "debug category for lsmparse element"));

static void
dlb_lsm_parse_class_init (DlbLsmParseClass * klass)
{
  GstBaseParseClass *base_parse_class = GST_BASE_PARSE_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_lsm_parse_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_lsm_parse_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Dolby LSM Parser",
      "Codec/Parser/Light",
      "Parse LSM light stream",
      "Dolby Support <support@dolby.com>");

  base_parse_class->start = GST_DEBUG_FUNCPTR (dlb_lsm_parse_start);
  base_parse_class->stop = GST_DEBUG_FUNCPTR (dlb_lsm_parse_stop);
  base_parse_class->handle_frame =
      GST_DEBUG_FUNCPTR (dlb_lsm_parse_handle_frame);
}

static void
dlb_lsm_parse_init (DlbLsmParse * lsm_parse)
{
  GST_PAD_SET_ACCEPT_INTERSECT (GST_BASE_PARSE_SINK_PAD (lsm_parse));
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_BASE_PARSE_SINK_PAD (lsm_parse));

  lsm_parse->caps_parsed = FALSE;
  lsm_parse->max_objects = 0;
}

static gboolean
dlb_lsm_parse_start (GstBaseParse * parse)
{
  DlbLsmParse *lsm_parse = DLB_LSM_PARSE (parse);

  GST_DEBUG_OBJECT (lsm_parse, "start");

  lsm_parse->caps_parsed = FALSE;
  lsm_parse->max_objects = 0;

//  gst_base_parse_set_min_frame_size(parse, 24);
//  gst_base_parse_set_has_timing_info(parse, TRUE);

  return TRUE;
}

static gboolean
dlb_lsm_parse_stop (GstBaseParse * parse)
{
  DlbLsmParse *lsm_parse = DLB_LSM_PARSE (parse);
  GST_DEBUG_OBJECT (lsm_parse, "stop");

  return TRUE;
}

static int check_caps(GstBaseParse * parse)
{
    DlbLsmParse *lsm_parse = DLB_LSM_PARSE (parse);
    if (lsm_parse->caps_parsed) {
        return 0;
    }

    GstCaps *sink_caps = gst_pad_get_current_caps (GST_BASE_PARSE_SINK_PAD (parse));

    if (sink_caps) {
      GST_INFO_OBJECT (parse, "sink caps %" GST_PTR_FORMAT, sink_caps);
      GstStructure *s = gst_caps_get_structure (sink_caps, 0);

      if (gst_structure_has_field (s, "uri-init-box")) {
          const GValue *uri_init_box = gst_structure_get_value (s, "uri-init-box");

          if (GST_VALUE_HOLDS_BUFFER (uri_init_box)) {
              GST_DEBUG_OBJECT (lsm_parse, "Found URI init box");
              GstBuffer *init_box_buf = gst_value_get_buffer (uri_init_box);
              GstMapInfo map;
              GstByteReader reader;

              gst_buffer_map (init_box_buf, &map, GST_MAP_READ);
              gst_byte_reader_init (&reader, map.data, map.size);

              guint8                version;
              guint8                color_space;
              guint32               frame_period_ms;

              if (gst_byte_reader_skip(&reader, 12) &&
                  gst_byte_reader_get_uint8(&reader, &version) &&
                  gst_byte_reader_get_uint32_be(&reader, &frame_period_ms) &&
                  gst_byte_reader_get_uint8(&reader, &lsm_parse->max_objects) &&
                  gst_byte_reader_get_uint8(&reader, (guint8*) &color_space)) {
                  GST_DEBUG_OBJECT (lsm_parse, "LSM version %d", version);
                  GST_DEBUG_OBJECT (lsm_parse, "Max objects %d", lsm_parse->max_objects);
                  GST_DEBUG_OBJECT (lsm_parse, "Color space %d", color_space);
                  GST_DEBUG_OBJECT (lsm_parse, "Frame period %d ms (%d us)", frame_period_ms, frame_period_ms * 1000);

                  GstCaps *caps = gst_caps_new_simple ("application/x-lsm",
                                                       "parsed", G_TYPE_BOOLEAN, TRUE,
                                                       "lsm-version", G_TYPE_INT, version,
                                                       "max-objects", G_TYPE_INT, lsm_parse->max_objects,
                                                       "color-space", G_TYPE_INT, color_space,
                                                       "frame-period", G_TYPE_INT, frame_period_ms * 1000,
                                                       NULL);

                  GST_INFO_OBJECT (parse, "src caps %" GST_PTR_FORMAT, caps);
                  gst_base_parse_set_frame_rate(parse, 1000, frame_period_ms, 0, 0);
                  gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (lsm_parse), caps);
                  gst_caps_unref (caps);
                  lsm_parse->caps_parsed = TRUE;

                  gst_buffer_unmap (init_box_buf, &map);
                  return 0;
              }

              GST_ERROR_OBJECT (lsm_parse, "URI init box is not big enough for LSM");
              gst_buffer_unmap (init_box_buf, &map);
              return 2;
          }
      }
    }
    return 1;
}

static GstFlowReturn
dlb_lsm_parse_handle_frame (GstBaseParse * parse, GstBaseParseFrame * frame,
    gint * skipsize)
{
  DlbLsmParse *lsm_parse = DLB_LSM_PARSE (parse);
  GstMapInfo map;
  int status = 0;

  GstFlowReturn ret = GST_FLOW_OK;
  GST_LOG_OBJECT (lsm_parse, "handle_frame");

  int caps_error = check_caps(parse);
  if (caps_error) {
      GST_ERROR_OBJECT (lsm_parse, "No valid metadata found to initialise LSM capabilities");
      *skipsize = (gint) 0;
      goto cleanup;
  }

  gst_buffer_map (frame->buffer, &map, GST_MAP_READ);

  guint8 do_skip = map.data[0];
  
  if (do_skip) {
      GST_LOG_OBJECT (lsm_parse, "found LSM skip frame (%ld bytes)", map.size);
  } else {
      guint8 num_objects = map.data[1];
    
      if (num_objects > lsm_parse->max_objects) {
          GST_WARNING_OBJECT (parse, "Frame contains %d objects, header indicated max %d objects", num_objects, lsm_parse->max_objects);
          *skipsize = (gint) map.size;
          status = 1;
          goto cleanup;
      }
    
      GST_LOG_OBJECT (lsm_parse, "found LSM frame (%ld bytes) with %d objects", map.size, num_objects);
  }

cleanup:
  gst_buffer_unmap (frame->buffer, &map);

  if (status == 0) {
      ret = gst_base_parse_finish_frame (parse, frame, map.size);
  }

  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "dlblsmparse", GST_RANK_PRIMARY + 2,
      DLB_TYPE_LSM_PARSE);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlblsmparse,
    "Dolby LSM Parser",
    plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
