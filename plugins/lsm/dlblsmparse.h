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

#ifndef _DLB_LSM_PARSE_H_
#define _DLB_LSM_PARSE_H_

#include <gst/base/gstbaseparse.h>

G_BEGIN_DECLS
#define DLB_TYPE_LSM_PARSE   (dlb_lsm_parse_get_type())
#define DLB_LSM_PARSE(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),DLB_TYPE_LSM_PARSE,DlbLsmParse))
#define DLB_LSM_PARSE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),DLB_TYPE_LSM_PARSE,DlbLsmParseClass))
#define DLB_IS_LSM_PARSE(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),DLB_TYPE_LSM_PARSE))
#define DLB_IS_LSM_PARSE_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),DLB_TYPE_LSM_PARSE))
typedef struct _DlbLsmParse DlbLsmParse;
typedef struct _DlbLsmParseClass DlbLsmParseClass;

struct _DlbLsmParse
{
    GstBaseParse    base_lsm_parse;
    gboolean        caps_parsed;
    guint8          max_objects;
};

struct _DlbLsmParseClass
{
    GstBaseParseClass base_lsm_parse_class;
};

GType dlb_lsm_parse_get_type (void);

G_END_DECLS
#endif // _DLB_LSM_PARSE_H_
