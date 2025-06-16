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

#include <assert.h>
#include <gst/gst.h>
#include <gst/base/base.h>
#include <gst/base/gstbasetransform.h>

#include "dlblightning.h"
#include "dlb_lightscapes.h"

GST_DEBUG_CATEGORY_STATIC (dlb_lightning_debug_category);
#define GST_CAT_DEFAULT dlb_lightning_debug_category

/* prototypes */
static void dlb_lightning_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void dlb_lightning_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static GstCaps *dlb_lightning_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static gboolean dlb_lightning_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean dlb_lightning_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize);
static gboolean dlb_lightning_start (GstBaseTransform * trans);
static gboolean dlb_lightning_stop (GstBaseTransform * trans);
static GstFlowReturn dlb_lightning_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf); /* Lightning process */

/* helper functions definitions */
static gboolean lightning_open (DlbLightning * lightning);
static void lightning_close (DlbLightning * lightning);
static gboolean lightning_restart (DlbLightning * lightning);
static gboolean lightning_is_opened (DlbLightning * lightning);

enum
{
  PROP_0,
  PROP_CONFIG,
  PROP_LIGHTNESS,
  PROP_ZONE_IMMERSION_LEVEL,
  PROP_ZONE_LOW_IMMERSION,
};

/* pad templates */
static GstStaticPadTemplate dlb_lightning_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-lights, "
        " format = (string) { DLB }; ")
    );

static GstStaticPadTemplate dlb_lightning_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-lsm, parsed = (boolean) true, "
        " lsm-version = (int) { 0 }, "
        " max-objects = (int) [ 1, 255 ], "
        " frame-period = (int) { 40000 }, "
        " color-space = (int) { 0, 1 }; ")
    );

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (DlbLightning, dlb_lightning, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (dlb_lightning_debug_category, "dlblightning", 0,
        "debug category for lightning element"));

static void
dlb_lightning_class_init (DlbLightningClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_lightning_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &dlb_lightning_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Lightscapes renderer implementation", "Light",
      "Plugin for rendering of Dolby Lightscapes",
      "Dolby Support <support@dolby.com>");

  gobject_class->set_property = GST_DEBUG_FUNCPTR (dlb_lightning_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (dlb_lightning_get_property);
  base_transform_class->transform_caps = GST_DEBUG_FUNCPTR (dlb_lightning_transform_caps);
  base_transform_class->set_caps = GST_DEBUG_FUNCPTR (dlb_lightning_set_caps);
  base_transform_class->transform_size = GST_DEBUG_FUNCPTR (dlb_lightning_transform_size);
  base_transform_class->start = GST_DEBUG_FUNCPTR (dlb_lightning_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (dlb_lightning_stop);
  base_transform_class->transform = GST_DEBUG_FUNCPTR (dlb_lightning_transform);

  /* install properties */
  g_object_class_install_property (gobject_class, PROP_CONFIG,
      g_param_spec_string ("config", "Lightscapes configuration",
          "Serialized Lightscapes configuration file", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT |
          GST_PARAM_MUTABLE_PLAYING));
  
  g_object_class_install_property (gobject_class, PROP_LIGHTNESS,
      g_param_spec_float ("lightness", "Global lightness", "Global lightness value", 0.0, 1.0, 1.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT |
          GST_PARAM_MUTABLE_PLAYING));
  
  g_object_class_install_property (gobject_class, PROP_ZONE_IMMERSION_LEVEL,
      gst_param_spec_array ("zone-immersion-levels", "Immersion",
          "Personalisation zone immersion level (0-1)",
          g_param_spec_int ("zone-immersion-levels", "zones", "zones", 0, 100, 100, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT |
          GST_PARAM_MUTABLE_PLAYING));
  
  g_object_class_install_property (gobject_class, PROP_ZONE_LOW_IMMERSION,
      gst_param_spec_array ("zone-low-immersions", "Immersion",
          "Personalisation zone immersion high (0) or low (1)",
          g_param_spec_int ("zone-low-immersions", "zones", "zones", 0, 1, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT |
          GST_PARAM_MUTABLE_PLAYING));
}

static void
dlb_lightning_init (DlbLightning * lightning)
{
    GST_DEBUG_OBJECT (lightning, "lightning_init");
  lightning->config_path = NULL;
  lightning->renderer_instance = NULL;
  lightning->renderer_config.serialized_conf = NULL;
  lightning->renderer_config.serialized_conf_size = 0;
  lightning->renderer_config.color_space = 0;
  lightning->renderer_config.max_num_objs = 0;
  lightning->renderer_config.max_num_md = 0;
  lightning->max_output_size = 0;
  
  lightning->global_lightness = 1.0f;
  for (unsigned i = 0; i < MAX_NUM_PERSONALIZATION_ZONES; i++) {
    lightning->a_zone_immersion_levels[i] = 1.0f;
    lightning->a_zone_low_immersion[i] = 0;
  }
}

static void
dlb_lightning_set_low_immersion (DlbLightning * lightning, const GValue * value)
{
  guint nb_entries = gst_value_array_get_size(value);
  if (nb_entries > MAX_NUM_PERSONALIZATION_ZONES)
  {
      g_warning ("Too many immersion zones specified (max %u)", MAX_NUM_PERSONALIZATION_ZONES);
      return;
  }
  for (unsigned i = 0; i < MAX_NUM_PERSONALIZATION_ZONES && i < nb_entries; i++) {
    lightning->a_zone_low_immersion[i] = g_value_get_int(gst_value_array_get_value (value, i));
  }
}

static void
dlb_lightning_set_immersion_levels (DlbLightning * lightning, const GValue * value)
{
  guint nb_entries = gst_value_array_get_size(value);
  if (nb_entries > MAX_NUM_PERSONALIZATION_ZONES)
  {
      g_warning ("Too many immersion zones specified (max %u)", MAX_NUM_PERSONALIZATION_ZONES);
      return;
  }
  for (unsigned i = 0; i < MAX_NUM_PERSONALIZATION_ZONES && i < nb_entries; i++) {
    lightning->a_zone_immersion_levels[i] = ((float)g_value_get_int(gst_value_array_get_value (value, i))) / 100.0f;
  }
}

void
dlb_lightning_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  DlbLightning *lightning = DLB_LIGHTNING (object);
  gboolean reneg = FALSE;

  GST_DEBUG_OBJECT (lightning, "set_property %u", property_id);
  GST_OBJECT_LOCK (lightning);

  switch (property_id) {
    case PROP_CONFIG:
      if(lightning->config_path)
        g_free (lightning->config_path);
      lightning->config_path = g_strdup (g_value_get_string (value));
      reneg = TRUE;
      break;
    case PROP_LIGHTNESS:
      lightning->global_lightness = g_value_get_float (value);
      break;
    case PROP_ZONE_IMMERSION_LEVEL:
      dlb_lightning_set_immersion_levels (lightning, value);
      break;
    case PROP_ZONE_LOW_IMMERSION:
      dlb_lightning_set_low_immersion (lightning, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (lightning);

  if (reneg && lightning_is_opened (lightning)) {
    lightning_restart (lightning);
    gst_base_transform_reconfigure_src (GST_BASE_TRANSFORM_CAST (lightning));
  }
}

void
dlb_lightning_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  DlbLightning *lightning = DLB_LIGHTNING (object);

  GST_DEBUG_OBJECT (lightning, "get_property");
  GST_OBJECT_LOCK (lightning);

  switch (property_id) {
    case PROP_CONFIG:
      g_value_set_string (value, lightning->config_path);
      break;
    case PROP_LIGHTNESS:
      g_value_set_float (value, lightning->global_lightness);
      break;
    case PROP_ZONE_IMMERSION_LEVEL:
    case PROP_ZONE_LOW_IMMERSION:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (lightning);
}

static gboolean
lightning_is_opened (DlbLightning * lightning)
{
  return (lightning->renderer_instance != NULL);
}

static GstCaps *
dlb_lightning_transform_caps (GstBaseTransform * trans, GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  DlbLightning *lightning = DLB_LIGHTNING (trans);
  GstCaps *othercaps;
  GST_DEBUG_OBJECT (lightning, "transform_caps");

  /* Simply return pad template for given direction */
  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */
    othercaps = gst_static_pad_template_get_caps (&dlb_lightning_sink_template);
  } else {
    /* transform caps going downstream */
    othercaps = gst_static_pad_template_get_caps (&dlb_lightning_src_template);
  }

  othercaps = gst_caps_make_writable (othercaps);

  GST_DEBUG_OBJECT (lightning, "transformed %" GST_PTR_FORMAT " into %" GST_PTR_FORMAT, caps, othercaps);

  if (filter) {
    GstCaps *intersect;

    GST_DEBUG_OBJECT (lightning, "Using filter caps %" GST_PTR_FORMAT, filter);
    intersect = gst_caps_intersect (othercaps, filter);
    gst_caps_unref (othercaps);
    GST_DEBUG_OBJECT (lightning, "Intersection %" GST_PTR_FORMAT, intersect);

    return intersect;
  }

  return othercaps;
}

static gboolean
dlb_lightning_set_caps (GstBaseTransform * trans, GstCaps * incaps, GstCaps * outcaps)
{
  DlbLightning *lightning = DLB_LIGHTNING (trans);

  GST_DEBUG_OBJECT (lightning, "incaps %" GST_PTR_FORMAT ", outcaps %" GST_PTR_FORMAT,
      incaps, outcaps);

  if (incaps) {
      GstStructure *s = gst_caps_get_structure (incaps, 0);
      gboolean ret_max_num_objs = FALSE, ret_color_space = FALSE, ret_frame_period = FALSE;
      if (!(ret_max_num_objs = gst_structure_get_int(s, "max-objects", &lightning->renderer_config.max_num_objs)) ||
          !(ret_color_space = gst_structure_get_int(s, "color-space", &lightning->renderer_config.color_space)) ||
          !(ret_frame_period = gst_structure_get_int(s, "frame-period", &lightning->renderer_config.frame_period_us))) {
          GST_ERROR_OBJECT(trans, "Input caps does not contain the required information (max-objects, color-space, frame-period)");
          GST_ERROR_OBJECT(trans, "Ret %s %s %s", ret_max_num_objs == TRUE ? "TRUE" : "FALSE", ret_color_space == TRUE ? "TRUE" : "FALSE", ret_frame_period == TRUE ? "TRUE" : "FALSE");
          return FALSE;
      }
  }

  lightning->renderer_config.max_num_md = 1;

  if (!lightning_is_opened (lightning)) {
      if (lightning_open (lightning) == FALSE) {
          lightning_close(lightning);
          return FALSE;
      }
  }
  return TRUE;
}

/* transform size */
static gboolean
dlb_lightning_transform_size (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, gsize size, GstCaps * othercaps, gsize * othersize)
{
  DlbLightning *lightning = DLB_LIGHTNING (trans);
  GST_DEBUG_OBJECT (lightning, "transform_size size %ld", size);
  *othersize = lightning->max_output_size * lightning->renderer_config.max_num_md;
  return TRUE;
}

/* states */
static gboolean
dlb_lightning_start (GstBaseTransform * trans)
{
  DlbLightning *lightning = DLB_LIGHTNING (trans);
  GST_DEBUG_OBJECT (lightning, "start");

  return TRUE;
}

static gboolean
dlb_lightning_stop (GstBaseTransform * trans)
{
  DlbLightning *lightning = DLB_LIGHTNING (trans);
  GST_DEBUG_OBJECT (lightning, "stop");

  lightning_close (lightning);
  return TRUE;
}

/* transform */
static GstFlowReturn
dlb_lightning_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  DlbLightning *lightning = DLB_LIGHTNING (trans);
  GstMapInfo outbuf_map, inbuf_map;

  GST_OBJECT_LOCK (trans);

  gst_buffer_map (inbuf, &inbuf_map, GST_MAP_READ);
  GST_DEBUG_OBJECT(lightning, "Input buffer size %ld", inbuf_map.size);

  if (G_UNLIKELY(inbuf_map.size == 0)) {
      GST_LOG_OBJECT(lightning, "Input buffer empty, producing no output");
      gst_buffer_unmap (inbuf, &inbuf_map);
      goto no_output;
  }

  gst_buffer_map (outbuf, &outbuf_map, GST_MAP_READWRITE);
  gsize outsize = outbuf_map.size;
  dlb_lsr_process (lightning->renderer_instance, inbuf_map.size, inbuf_map.data, &outsize, outbuf_map.data, lightning->a_zone_immersion_levels, lightning->a_zone_low_immersion, lightning->global_lightness);
  GST_DEBUG_OBJECT(lightning, "Output buffer size %ld", outsize);

  gst_buffer_resize (outbuf, 0, outsize);
  gst_buffer_unmap (outbuf, &outbuf_map);
  gst_buffer_unmap (inbuf, &inbuf_map);

  GST_OBJECT_UNLOCK (trans);
  return GST_FLOW_OK;

no_output:
  GST_OBJECT_UNLOCK (trans);
  return GST_BASE_TRANSFORM_FLOW_DROPPED;
}

static gboolean
lightning_open (DlbLightning * lightning)
{
  GError *error = NULL;
  gchar *conf = NULL;
  GST_DEBUG_OBJECT (lightning, "open");

  GST_INFO("checking device config path");
  if (!lightning->config_path)
  {
    GST_ELEMENT_ERROR (lightning, LIBRARY, INIT, (NULL), ("device-config property cannot be empty"));
    return FALSE;
  }

  GST_INFO("getting device config");
  g_file_get_contents (lightning->config_path, &conf, &lightning->renderer_config.serialized_conf_size, &error);
  if (error != NULL)
  {
    GST_ELEMENT_ERROR (lightning, LIBRARY, INIT, (NULL), ("device-config could not be read"));
    return FALSE;
  }
  lightning->renderer_config.serialized_conf = (guint8*)conf;

  GST_INFO("opening lightning");
  lightning->renderer_instance = dlb_lsr_new (&lightning->renderer_config);
  if (lightning->renderer_instance == NULL)
  {
    GST_ELEMENT_ERROR (lightning, LIBRARY, INIT, (NULL), ("lightning could not be created"));
    g_free (conf);

    return FALSE;
  }
  lightning->max_output_size = dlb_lsr_get_max_output_size(lightning->renderer_instance);
  GST_DEBUG_OBJECT (lightning, "max output size %zu", lightning->max_output_size);

  GST_INFO("releasing memory");
  g_free (conf);

  return TRUE;
}

static void
lightning_close (DlbLightning * lightning)
{
  GST_DEBUG_OBJECT (lightning, "close");

  if (lightning->renderer_instance){
    GST_INFO("free-ing lightning");
    dlb_lsr_free (lightning->renderer_instance);
  }
  if (lightning->config_path){
    GST_INFO("free-ing device config string %s", lightning->config_path);
    g_free(lightning->config_path);
  }

  lightning->renderer_instance = NULL;
  lightning->config_path = NULL;
}

static gboolean
lightning_restart (DlbLightning * lightning)
{
  GST_DEBUG_OBJECT (lightning, "restart");

  if (lightning->renderer_instance) {
      GST_INFO("free-ing lightning");
      dlb_lsr_free(lightning->renderer_instance);
      lightning->renderer_instance = NULL;
  }

  return lightning_open (lightning);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  #ifdef DLB_LIGHTNING_OPEN_DYNLIB
  if (dlb_lightning_try_open_dynlib ())
    return FALSE;
  #endif

  if (!gst_element_register (plugin, "dlblightning", GST_RANK_PRIMARY, DLB_TYPE_LIGHTNING))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dlblightning,
    "Lightscapes renderer implementation", plugin_init, VERSION, LICENSE, PACKAGE, ORIGIN)
