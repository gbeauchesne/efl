/*
 * emotion_hwaccel.h - Hardware acceleration for Emotion
 *
 * Copyright (C) 2014-2015 Intel Corporation
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#ifndef EMOTION_HWACCEL_H
#define EMOTION_HWACCEL_H

#include <Eina.h>
#include <Evas.h>
#include <gst/gst.h>
#include <gst/video/gstvideometa.h>

#define EMOTION_HWACCEL(obj) \
    ((Emotion_HWAccel *)(obj))

typedef struct _Emotion_Gstreamer_Buffer        Emotion_HWAccel_Buffer;
typedef enum emotion_hwaccel_type               Emotion_HWAccel_Type;
typedef struct emotion_hwaccel_s                Emotion_HWAccel;
typedef struct emotion_hwaccel_info_s           Emotion_HWAccel_Info;

typedef gboolean (*Emotion_HWAccel_IsAvailable_Func)(
    Evas_Object *obj, gint engine_type);
typedef Emotion_HWAccel *(*Emotion_HWAccel_Allocate_Func)(
    Evas_Object *obj, gint engine_type);
typedef gboolean (*Emotion_HWAccel_ProposeAllocation_Func)(
    Emotion_HWAccel *hwaccel, GstQuery *query);
typedef gboolean (*Emotion_HWAccel_Upload_Func)(
    Emotion_HWAccel *hwaccel, Emotion_HWAccel_Buffer *buffer,
    Evas_Object *image);

enum emotion_hwaccel_type {
    EMOTION_HWACCEL_TYPE_NONE = 0,
    EMOTION_HWACCEL_TYPE_OPENGL,
};

struct emotion_hwaccel_info_s {
    Emotion_HWAccel_Type type;
    const gchar *name;

    Emotion_HWAccel_IsAvailable_Func is_available;
    Emotion_HWAccel_Allocate_Func allocate;
    Emotion_HWAccel_ProposeAllocation_Func propose_allocation;
    Emotion_HWAccel_Upload_Func upload;
};

struct emotion_hwaccel_s {
    GstMiniObject mini_object;

    const Emotion_HWAccel_Info *info;
    Evas_Object *emotion_object;
    Evas_Object *evas_image;
    GstBuffer *buffer;
};

const Emotion_HWAccel_Info **
emotion_hwaccel_info_get_all(void);

const Emotion_HWAccel_Info *
emotion_hwaccel_info_get(Emotion_HWAccel_Type type);

const Emotion_HWAccel_Info emotion_hwaccel_info_none;
const Emotion_HWAccel_Info emotion_hwaccel_info_opengl;

#define emotion_hwaccel_ref(hwaccel) \
    EMOTION_HWACCEL(gst_mini_object_ref(GST_MINI_OBJECT_CAST(hwaccel)))
#define emotion_hwaccel_unref(hwaccel) \
    gst_mini_object_unref(GST_MINI_OBJECT_CAST(hwaccel))
#define emotion_hwaccel_replace(old_hwaccel_ptr, new_hwaccel) \
    gst_mini_object_replace((GstMiniObject **)(old_hwaccel_ptr), \
        GST_MINI_OBJECT_CAST(new_hwaccel))

gboolean
emotion_hwaccel_is_available(Emotion_HWAccel_Type type, Evas_Object  *obj);

Emotion_HWAccel *
emotion_hwaccel_new(Emotion_HWAccel_Type type, Evas_Object *obj);

gboolean
emotion_hwaccel_propose_allocation(Emotion_HWAccel *hwaccel, GstQuery *query);

gboolean
emotion_hwaccel_upload(Emotion_HWAccel *hwaccel,
    Emotion_HWAccel_Buffer *buffer, Evas_Object *image);

/* private */
void
emotion_hwaccel_clear(Emotion_HWAccel *hwaccel);

/* ------------------------------------------------------------------------- */
// OpenGL

#if GST_CHECK_VERSION(1,2,0)
# define USE_EMOTION_HWACCEL_OPENGL 1

# define EMOTION_HWACCEL_OPENGL_CAPS \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES( \
        GST_CAPS_FEATURE_META_GST_VIDEO_GL_TEXTURE_UPLOAD_META, "BGRA")
#else
# define USE_EMOTION_HWACCEL_OPENGL 0
#endif

#endif /* EMOTION_HWACCEL_H */
