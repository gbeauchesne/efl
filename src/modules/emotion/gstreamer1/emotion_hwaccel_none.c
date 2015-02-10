/*
 * emotion_hwaccel_none.c - No hardware acceleration for Emotion (default)
 *
 * Copyright (C) 2002-2012 Carsten Haitzler, Dan Sinclair, Mike Blumenkrantz,
 * Samsung Electronics and various contributors (see AUTHORS)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "emotion_hwaccel.h"
#include "emotion_gstreamer.h"

GST_DEFINE_MINI_OBJECT_TYPE(Emotion_HWAccel, emotion_hwaccel_none);

static gboolean
emotion_hwaccel_none_is_available(Evas_Object *obj EINA_UNUSED,
    gint engine_type EINA_UNUSED)
{
    /* This is not a hardware accelerated backend, so don't try to
       auto-plug it */
    return FALSE;
}

static gboolean
emotion_hwaccel_none_dispose(Emotion_HWAccel *hwaccel EINA_UNUSED)
{
    return TRUE;
}

static void
emotion_hwaccel_none_free(Emotion_HWAccel *hwaccel)
{
    emotion_hwaccel_clear(hwaccel);
    g_slice_free(Emotion_HWAccel, hwaccel);
}

static gboolean
emotion_hwaccel_none_init(Emotion_HWAccel *hwaccel, Evas_Object *obj)
{
    gst_mini_object_init(GST_MINI_OBJECT_CAST(hwaccel), 0,
        emotion_hwaccel_none_get_type(),
        (GstMiniObjectCopyFunction)NULL,
        (GstMiniObjectDisposeFunction)emotion_hwaccel_none_dispose,
        (GstMiniObjectFreeFunction)emotion_hwaccel_none_free);

    hwaccel->info = &emotion_hwaccel_info_none;
    hwaccel->emotion_object = obj;
    return TRUE;
}

static Emotion_HWAccel *
emotion_hwaccel_none_new(Evas_Object *obj, gint engine_type EINA_UNUSED)
{
    Emotion_HWAccel *hwaccel;

    hwaccel = g_slice_new0(Emotion_HWAccel);
    if (!emotion_hwaccel_none_init(hwaccel, obj))
        goto error;
    return hwaccel;

error:
    emotion_hwaccel_none_free(hwaccel);
    return NULL;
}

static gboolean
emotion_hwaccel_none_propose_allocation(Emotion_HWAccel *hwaccel EINA_UNUSED,
    GstQuery *query)
{
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    return TRUE;
}

gboolean
emotion_hwaccel_none_upload(Emotion_HWAccel *hwaccel EINA_UNUSED,
    Emotion_Gstreamer_Buffer *emotion_buffer, Evas_Object *image)
{
    GstVideoFrame gst_frame;
    guint width, height;
    unsigned char *evas_data;

    width = GST_VIDEO_INFO_WIDTH(&emotion_buffer->info);
    height = GST_VIDEO_INFO_HEIGHT(&emotion_buffer->info);

    // XXX: need to map buffer and KEEP MAPPED until we set new video data or
    // on the evas image object or release the object
    if (!gst_video_frame_map(&gst_frame, &emotion_buffer->info,
             emotion_buffer->frame, GST_MAP_READ))
        return FALSE;

    INF("sink main render [%i, %i]", width, height);

    evas_object_image_alpha_set(image, 0);
    evas_object_image_colorspace_set(image, emotion_buffer->eformat);
    evas_object_image_size_set(image, width, height);

    // XXX: need to handle GstVideoCropMeta to get video cropping right

    evas_data = evas_object_image_data_get(image, 1);

    if (emotion_buffer->func)
        emotion_buffer->func(evas_data, &gst_frame);
    else
        WRN("No way to decode %x colorspace !", emotion_buffer->eformat);

    // XXX: this unmap here is broken
    gst_video_frame_unmap(&gst_frame);

    evas_object_image_data_set(image, evas_data);
    evas_object_image_data_update_add(image, 0, 0, width, height);
    evas_object_image_pixels_dirty_set(image, 0);
    return TRUE;
}

const Emotion_HWAccel_Info emotion_hwaccel_info_none = {
    .type = EMOTION_HWACCEL_TYPE_NONE,
    .name = "emotion-hwaccel-none",

    .is_available = emotion_hwaccel_none_is_available,
    .allocate = emotion_hwaccel_none_new,
    .propose_allocation = (Emotion_HWAccel_ProposeAllocation_Func)
        emotion_hwaccel_none_propose_allocation,
    .upload = (Emotion_HWAccel_Upload_Func)
        emotion_hwaccel_none_upload,
};
