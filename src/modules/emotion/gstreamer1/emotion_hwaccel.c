/*
 * emotion_hwaccel.c - Hardware acceleration for Emotion
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

#include "Ecore_Evas.h"
#include "emotion_hwaccel.h"
#include "emotion_gstreamer.h"

static const Emotion_HWAccel_Info *g_emotion_hwaccel_info[] = {
    &emotion_hwaccel_info_none,
#if USE_EMOTION_HWACCEL_OPENGL
    &emotion_hwaccel_info_opengl,
#endif
    NULL
};

const Emotion_HWAccel_Info **
emotion_hwaccel_info_get_all(void)
{
    return g_emotion_hwaccel_info;
}

const Emotion_HWAccel_Info *
emotion_hwaccel_info_get(Emotion_HWAccel_Type type)
{
    const Emotion_HWAccel_Info **info_list;

    for (info_list = g_emotion_hwaccel_info; *info_list != NULL; info_list++) {
        if ((*info_list)->type == type)
            return *info_list;
    }
    return NULL;
}

static gchar *
get_engine_name(Evas_Object *obj)
{
    gchar *engine = NULL;
    const gchar *ename;
    Eina_List *l, *engines;

    engines = evas_render_method_list();
    if (!engines)
        return NULL;

    EINA_LIST_FOREACH(engines, l, ename)
    {
        if (evas_render_method_lookup(ename) ==
            evas_output_method_get(evas_object_evas_get(obj)))
        {
            engine = g_strdup(ename);
            break;
        }
    }
    evas_render_method_list_free(engines);
    return engine;
}

static gboolean
get_engine_type(Evas_Object *obj, Ecore_Evas_Engine_Type *engine_type_ptr)
{
    gchar *ename;

    struct map {
        const gchar *name;
        Ecore_Evas_Engine_Type type;
    };

    static const struct map g_map[] = {
        { "gl_x11",             ECORE_EVAS_ENGINE_OPENGL_X11    },
        { NULL, }
    };
    const struct map *m;

    ename = get_engine_name(obj);
    if (!ename)
        return FALSE;

    for (m = g_map; m->name != NULL; m++) {
        if (g_ascii_strcasecmp(m->name, ename) == 0)
            break;
    }
    g_free(ename);

    if (m->name) {
        if (engine_type_ptr)
            *engine_type_ptr = m->type;
        return TRUE;
    }
    return FALSE;
}

gboolean
emotion_hwaccel_is_available(Emotion_HWAccel_Type type, Evas_Object  *obj)
{
    const Emotion_HWAccel_Info * const info = emotion_hwaccel_info_get(type);
    Ecore_Evas_Engine_Type etype;

    g_return_val_if_fail(obj != NULL, FALSE);

    if (!info || !info->is_available || !get_engine_type(obj, &etype))
        return FALSE;
    return info->is_available(obj, etype);
}

Emotion_HWAccel *
emotion_hwaccel_new(Emotion_HWAccel_Type type, Evas_Object *obj)
{
    const Emotion_HWAccel_Info * const info = emotion_hwaccel_info_get(type);
    Ecore_Evas_Engine_Type etype;

    g_return_val_if_fail(obj != NULL, NULL);

    if (!info || !info->allocate)
        return NULL;

    if (type == EMOTION_HWACCEL_TYPE_NONE)
        etype = (Ecore_Evas_Engine_Type)-1; /* allow anything */
    else if (!get_engine_type(obj, &etype))
        return NULL;
    return info->allocate(obj, etype);
}

static void
emotion_hwaccel_clear_internal(Emotion_HWAccel *hwaccel)
{
    if (hwaccel->evas_image) {
        evas_object_unref(hwaccel->evas_image);
        hwaccel->evas_image = NULL;
    }

    gst_buffer_replace(&hwaccel->buffer, NULL);
}

void
emotion_hwaccel_clear(Emotion_HWAccel *hwaccel)
{
    emotion_hwaccel_clear_internal(hwaccel);
}

gboolean
emotion_hwaccel_propose_allocation(Emotion_HWAccel *hwaccel, GstQuery *query)
{
    g_return_val_if_fail (hwaccel != NULL, FALSE);
    g_return_val_if_fail (hwaccel->info != NULL, FALSE);

    return hwaccel->info->propose_allocation &&
        hwaccel->info->propose_allocation(hwaccel, query);
}

gboolean
emotion_hwaccel_upload(Emotion_HWAccel *hwaccel,
    Emotion_HWAccel_Buffer *emotion_buffer, Evas_Object *image)
{
    const Emotion_HWAccel_Info *info;
    gboolean success;

    g_return_val_if_fail (hwaccel != NULL, FALSE);
    g_return_val_if_fail (hwaccel->info != NULL, FALSE);

    emotion_hwaccel_clear_internal(hwaccel);

    info = hwaccel->info;
    success = info->upload && info->upload(hwaccel, emotion_buffer, image);
    if (!success)
        return FALSE;

    hwaccel->buffer = gst_buffer_ref(emotion_buffer->frame);
    hwaccel->evas_image = image;
    return TRUE;
}
