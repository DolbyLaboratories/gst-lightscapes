/*******************************************************************************

 * Lightscapes GStreamer Plugins
 * Copyright (C) 2024-2026, Dolby Laboratories

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
        " lsm-version = (int) { 1 }, "
        " max-objects = (int) [ 1, 255 ], "
        " frame-period = (int) [ 1, 1000000 ], "
        " color-space = (int) { 0, 1 }, "
        " lscp-profile = (int) [ 0, 15 ], "
        " lscp-level = (int) [ 0, 15 ]; ")
    );

static GstStaticPadTemplate dlb_lsm_parse_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/octet-stream, uri = (string) { urn:oid:1.2.6.1.4.6729.1.3 }, "
        " es_version = (int) { 1 }, "
        " frame_period = (int) [ 1, 1000 ], "
        " max_objects = (int) [ 1, 255 ], "
        " colour_type = (string) { nclx, rICC, prof }, "
        " colour_primaries = (int) { 9 }, "
        " transfer_characteristics = (int) {15, 16 }, "
        " matrix_coefficients = (int) { 9 }, "
        " full_range = (boolean) false; ")
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
  lsm_parse->profile = 0;
  lsm_parse->level = 1;
}

static gboolean
dlb_lsm_parse_start (GstBaseParse * parse)
{
  DlbLsmParse *lsm_parse = DLB_LSM_PARSE (parse);

  GST_DEBUG_OBJECT (lsm_parse, "start");

  lsm_parse->caps_parsed = FALSE;
  lsm_parse->max_objects = 0;
  lsm_parse->profile = 0;
  lsm_parse->level = 1;

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

        if (gst_structure_has_field (s, "lscc-box") && gst_structure_has_field (s, "colr-box") &&  gst_structure_has_field (s, "btrt-box")) {
            guint8                version;
            guint8                color_space;
            guint8                box_version = 0;
            guint32               frame_period_ms;

            const GValue *lscc_box = gst_structure_get_value (s, "lscc-box");
            if (GST_VALUE_HOLDS_BUFFER (lscc_box)) {
                GST_DEBUG_OBJECT (lsm_parse, "Found 'lscc' box");
                GstBuffer *lscc_box_buf = gst_value_get_buffer (lscc_box);
                GstMapInfo map;
                GstByteReader reader;
                gst_buffer_map (lscc_box_buf, &map, GST_MAP_READ);
                gst_byte_reader_init (&reader, map.data, map.size);
                /* FullBox layout: [0..3] size, [4..7] type, [8] version, [9..11] flags */
                if (gst_byte_reader_skip(&reader, 8) &&
                    gst_byte_reader_get_uint8(&reader, &box_version) &&
                    gst_byte_reader_skip(&reader, 3) &&
                    gst_byte_reader_get_uint32_be(&reader, &frame_period_ms) &&
                    gst_byte_reader_get_uint8(&reader, &lsm_parse->max_objects) &&
                    gst_byte_reader_get_uint8(&reader, &version)) {
                        if (box_version != 0) {
                            GST_ERROR_OBJECT (lsm_parse, "lscc FullBox version %d is not supported (expected 0)", box_version);
                            gst_buffer_unmap (lscc_box_buf, &map);
                            return 3;
                        }
                        if (version != 1) {
                            GST_ERROR_OBJECT (lsm_parse, "LSM bitstream es_version %d is not supported (expected 1)", version);
                            gst_buffer_unmap (lscc_box_buf, &map);
                            return 3;
                        }
                        GST_DEBUG_OBJECT (lsm_parse, "frame_period %d ms (%d us)", frame_period_ms, frame_period_ms * 1000);
                        GST_DEBUG_OBJECT (lsm_parse, "max_objects %d", lsm_parse->max_objects);
                        GST_DEBUG_OBJECT (lsm_parse, "es_version %d", version);
                }
                gst_buffer_unmap (lscc_box_buf, &map);
            }
            const GValue *colr_box = gst_structure_get_value (s, "colr-box");
            if (GST_VALUE_HOLDS_BUFFER (colr_box)) {
                GST_DEBUG_OBJECT (lsm_parse, "Found 'colr' box");
                GstBuffer *colr_box_buf = gst_value_get_buffer (colr_box);
                GstMapInfo map;
                GstByteReader reader;
                gst_buffer_map (colr_box_buf, &map, GST_MAP_READ);
                gst_byte_reader_init (&reader, map.data, map.size);
                guint32 colour_type;
                guint16 colour_primaries;
                guint16 transfer_characteristics;
                guint16 matrix_coefficients;
                guint8 full_range_flag;

                if (gst_byte_reader_skip(&reader, 8) &&
                    gst_byte_reader_get_uint32_be(&reader, &colour_type) &&
                    gst_byte_reader_get_uint16_be(&reader, &colour_primaries) &&
                    gst_byte_reader_get_uint16_be(&reader, &transfer_characteristics) &&
                    gst_byte_reader_get_uint16_be(&reader, &matrix_coefficients) &&
                    gst_byte_reader_get_uint8(&reader, &full_range_flag)) {
                        GST_DEBUG_OBJECT (lsm_parse, "color_type '%c%c%c%c'",(char)((colour_type >> 24) & 0xff),
                                                                             (char)((colour_type >> 16) & 0xff),
                                                                             (char)((colour_type >>  8) & 0xff),
                                                                             (char)((colour_type      ) & 0xff));
                        GST_DEBUG_OBJECT (lsm_parse, "color_primaries %d", colour_primaries);
                        GST_DEBUG_OBJECT (lsm_parse, "transfer_characteristics %d", transfer_characteristics);
                        GST_DEBUG_OBJECT (lsm_parse, "matrix_coefficients %d", matrix_coefficients);
                        GST_DEBUG_OBJECT (lsm_parse, "full_range_flag %d", full_range_flag);

                        color_space = transfer_characteristics == 16 ? 1 : 0; /* 0=Rec.2020 linear, 1=Rec.2020 PQ*/
                    }
                gst_buffer_unmap (colr_box_buf, &map);
            }
            const GValue *btrt_box = gst_structure_get_value (s, "btrt-box");
            if (GST_VALUE_HOLDS_BUFFER (btrt_box)) {
                GST_DEBUG_OBJECT (lsm_parse, "Found 'btrt' box");
                GstBuffer *btrt_box_buf = gst_value_get_buffer (btrt_box);
                GstMapInfo map;
                GstByteReader reader;
                gst_buffer_map (btrt_box_buf, &map, GST_MAP_READ);
                gst_byte_reader_init (&reader, map.data, map.size);
                guint32 max_sample_size;
                guint32 avg_bitrate;
                guint32 max_bitrate;

                if (gst_byte_reader_skip(&reader, 8) &&
                    gst_byte_reader_get_uint32_be(&reader, &max_sample_size) &&
                    gst_byte_reader_get_uint32_be(&reader, &avg_bitrate) &&
                    gst_byte_reader_get_uint32_be(&reader, &max_bitrate)) {
                        GST_DEBUG_OBJECT (lsm_parse, "max_sample_size %d bytes", max_sample_size);
                        GST_DEBUG_OBJECT (lsm_parse, "avg_bitrate %d bits/s", avg_bitrate);
                        GST_DEBUG_OBJECT (lsm_parse, "max_bitrate %d bits/s", max_bitrate);
                }
                gst_buffer_unmap (btrt_box_buf, &map);
            }

            gint lscp_profile_val = 0;
            gint lscp_level_val = 1;
            gst_structure_get_int (s, "lscp-profile", &lscp_profile_val);
            gst_structure_get_int (s, "lscp-level", &lscp_level_val);
            lsm_parse->profile = (guint8) lscp_profile_val;
            lsm_parse->level = (guint8) lscp_level_val;
            GST_DEBUG_OBJECT (lsm_parse, "lscp-profile %d, lscp-level %d",
                lsm_parse->profile, lsm_parse->level);

            GstCaps *caps = gst_caps_new_simple ("application/x-lsm",
                                            "parsed", G_TYPE_BOOLEAN, TRUE,
                                            "lsm-version", G_TYPE_INT, version,
                                            "max-objects", G_TYPE_INT, lsm_parse->max_objects,
                                            "color-space", G_TYPE_INT, color_space,
                                            "frame-period", G_TYPE_INT, frame_period_ms * 1000,
                                            "lscp-profile", G_TYPE_INT, (gint) lsm_parse->profile,
                                            "lscp-level", G_TYPE_INT, (gint) lsm_parse->level,
                                            NULL);

            GST_INFO_OBJECT (parse, "src caps %" GST_PTR_FORMAT, caps);
            gst_base_parse_set_frame_rate(parse, 1000, frame_period_ms, 0, 0);
            gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (lsm_parse), caps);
            gst_caps_unref (caps);
            lsm_parse->caps_parsed = TRUE;
            return 0;
        }

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
                    if (version != 1) {
                        GST_ERROR_OBJECT (lsm_parse, "LSM bitstream version %d is not supported (expected 1)", version);
                        gst_buffer_unmap (init_box_buf, &map);
                        return 3;
                    }
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
  if (caps_error == 3) {
      ret = GST_FLOW_ERROR;
      goto done;
  } else if (caps_error) {
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

done:
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
