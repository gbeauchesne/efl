/*
 * emotion_hwaccel_opengl.h - Hardware acceleration for Emotion (OpenGL)
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

#include "config.h"
#include "emotion_hwaccel.h"
#include "Ecore_Evas.h"
#include <gmodule.h>
#include <gst/gl/gstgldisplay.h>
#include <gst/gl/gstglcontext.h>
#include "emotion_gstreamer.h"

#define USE_X11         (defined(BUILD_ENGINE_GL_X11) && GST_GL_HAVE_WINDOW_X11)
#define USE_GLX         (!defined(GL_GLES) || GL_GLES == 0)
#define USE_EGL         (defined(GL_GLES) && GL_GLES > 0)

#if USE_EGL
# include <gst/gl/egl/gstgldisplay_egl.h>
#endif

#if USE_X11
# include <gst/gl/x11/gstgldisplay_x11.h>
# include "modules/evas/engines/gl_x11/Evas_Engine_GL_X11.h"
#endif

#if 0
/* FIXME: those are not exposed yet to public API set */
#if USE_GLX
# include <gst/gl/x11/gstglcontext_glx.h>
#else
# include <gst/gl/egl/gstglcontext_egl.h>
#endif
#endif

/* Number of textures used for rendering video frames */
#define NUM_TEXTURES 1

typedef struct emotion_hwaccel_opengl_s         Emotion_HWAccel_OpenGL;

struct emotion_hwaccel_opengl_s {
    Emotion_HWAccel base;

    GstGLAPI gl_api;
    GstGLPlatform gl_platform;
    GstGLDisplayType gl_display_type;
    GstGLDisplay *gl_display;
    GstGLContext *gl_context;
    GstGLContext *evas_gl_context;

    /* Don't use a GstGLBufferPool since BGRA texture formats are not
       supported yet */
    guint texture_id;
    guint textures[NUM_TEXTURES];
    guint width;
    guint height;
};

GST_DEFINE_MINI_OBJECT_TYPE(Emotion_HWAccel_OpenGL, emotion_hwaccel_opengl);

static gboolean
ensure_display_type(Emotion_HWAccel_OpenGL *hwaccel, gint engine_type)
{
    if (hwaccel->gl_display_type)
        return TRUE;

    switch (engine_type) {
    case ECORE_EVAS_ENGINE_OPENGL_X11:
        hwaccel->gl_display_type = GST_GL_DISPLAY_TYPE_X11;
        break;
    default:
        hwaccel->gl_display_type = GST_GL_DISPLAY_TYPE_NONE;
        break;
    }
    return hwaccel->gl_display_type != GST_GL_DISPLAY_TYPE_NONE;
}

static gboolean
ensure_display(Emotion_HWAccel_OpenGL *hwaccel, gint engine_type)
{
    Evas * const evas = evas_object_evas_get(hwaccel->base.emotion_object);

    if (hwaccel->gl_display)
        return TRUE;

    if (!ensure_display_type(hwaccel, engine_type))
        return FALSE;

    switch (engine_type) {
#if USE_X11
    case ECORE_EVAS_ENGINE_OPENGL_X11: {
        Evas_Engine_Info_GL_X11 * const einfo =
            (Evas_Engine_Info_GL_X11 *)evas_engine_info_get(evas);

        hwaccel->gl_display = GST_GL_DISPLAY(
            gst_gl_display_x11_new_with_display(einfo->info.display));
        break;
    }
#endif
    default:
        break;
    }
    return hwaccel->gl_display != NULL;
}

static gboolean
ensure_current_context(Emotion_HWAccel_OpenGL *hwaccel)
{
    GModule *module = NULL;
    GstGLDisplay *gl_display = NULL;
    GstGLDisplayType gl_display_type = hwaccel->gl_display_type;
    GstGLContext *gl_context;
    guintptr gl_context_handle;
#if !GST_CHECK_VERSION(1,5,0)
    guintptr gl_display_handle;
    guintptr (*get_current_context)(void) = 0;
    guintptr (*get_current_display)(void) = 0;
    const gchar **gl_symbols = NULL;
#endif

    if (hwaccel->evas_gl_context)
        return TRUE;

#if USE_GLX
    hwaccel->gl_platform = GST_GL_PLATFORM_GLX;
#elif USE_EGL
    hwaccel->gl_platform = GST_GL_PLATFORM_EGL;
    gl_display_type = GST_GL_DISPLAY_TYPE_EGL;
#else
    hwaccel->gl_platform = GST_GL_PLATFORM_NONE;
#endif
    if (hwaccel->gl_platform == GST_GL_PLATFORM_NONE)
        return FALSE;

#if GST_CHECK_VERSION(1,5,0)
    gl_context_handle =
        gst_gl_context_get_current_gl_context(hwaccel->gl_platform);
    if (!gl_context_handle)
        goto error_load_handles;

    hwaccel->gl_api = gst_gl_context_get_current_gl_api(NULL, NULL);

    gl_display = gst_object_ref(hwaccel->gl_display);
    if (!gl_display)
        goto error_wrap_display;
    gst_gl_display_filter_gl_api(gl_display, hwaccel->gl_api);
#else
    /* FIXME: ensure that we are running in the Evas thread */
#if USE_GLX
    if (!gl_symbols) {
        static const gchar *glx_symbols[] =
            { "glXGetCurrentDisplay", "glXGetCurrentContext" };
        gl_symbols = glx_symbols;
    }
#endif
#if USE_EGL
    if (!gl_symbols) {
        static const gchar *egl_symbols[] =
            { "eglGetCurrentDisplay", "eglGetCurrentContext" };
        gl_symbols = egl_symbols;
    }
#endif
    if (!gl_symbols)
        goto error_load_symbols;

    module = g_module_open(NULL, G_MODULE_BIND_LOCAL|G_MODULE_BIND_LAZY);
    if (!module)
        return FALSE;

    if (!g_module_symbol(module, gl_symbols[0], (gpointer *)
             &get_current_display))
        goto error_load_symbols;
    if (!g_module_symbol(module, gl_symbols[1], (gpointer *)
             &get_current_context))
        goto error_load_symbols;

    gl_display_handle = get_current_display();
    gl_context_handle = get_current_context();
    g_module_close(module);
    module = NULL;

    if (!gl_display_handle || !gl_context_handle)
        goto error_load_handles;

    if (gst_gl_display_get_handle(hwaccel->gl_display) == gl_display_handle)
        gl_display = gst_object_ref(hwaccel->gl_display);
    else {
        switch (gl_display_type) {
#if USE_EGL
        case GST_GL_DISPLAY_TYPE_EGL:
            gl_display = GST_GL_DISPLAY(
                gst_gl_display_egl_new_with_egl_display(
                    GSIZE_TO_POINTER(gl_display_handle)));
            break;
#endif
#if USE_GLX
        case GST_GL_DISPLAY_TYPE_X11:
            gl_display = GST_GL_DISPLAY(
                gst_gl_display_x11_new_with_display(
                    GSIZE_TO_POINTER(gl_display_handle)));
        break;
#endif
        default:
            gl_display = NULL;
            break;
        }
        if (!gl_display)
            goto error_wrap_display;
    }

    /* FIXME: ensure GL API from Evas GL */
#if defined(GL_GLES)
    hwaccel->gl_api = GST_GL_API_GLES2;
#else
    hwaccel->gl_api = GST_GL_API_OPENGL;
#endif
#endif

    gl_context = gst_gl_context_new_wrapped(gl_display, gl_context_handle,
        hwaccel->gl_platform, hwaccel->gl_api);
    g_clear_object(&gl_display);
    if (!gl_context)
        goto error_wrap_context;

    hwaccel->evas_gl_context = gl_context;
    return TRUE;

    /* ERRORS */
error_load_handles:
    GST_ERROR("failed to resolve current GL handles (display, context)");
    goto error_cleanup;
#if !GST_CHECK_VERSION(1,5,0)
error_load_symbols:
    GST_ERROR("failed to load GL platform symbols");
    goto error_cleanup;
#endif
error_wrap_display:
    GST_ERROR("failed to wrap Evas GL display");
    goto error_cleanup;
error_wrap_context:
    GST_ERROR("failed to wrap Evas GL context");
    goto error_cleanup;
error_cleanup:
    if (module)
        g_module_close(module);
    g_clear_object(&gl_display);
    return FALSE;
}

static gboolean
ensure_context(Emotion_HWAccel_OpenGL *hwaccel)
{
    GstGLContext *gl_context;
    GError *error;

    if (hwaccel->gl_context)
        return TRUE;

    if (!ensure_current_context(hwaccel))
        return FALSE;

    gl_context = gst_gl_context_new(hwaccel->gl_display);
    if (!gl_context)
        return FALSE;

    error = NULL;
    if (!gst_gl_context_create(gl_context, hwaccel->evas_gl_context, &error))
        goto error_create_context;

    hwaccel->gl_context = gl_context;
    return TRUE;

    /* ERRORS */
error_create_context:
    GST_ERROR("failed to create GL context: %s", error->message);
    gst_object_unref(gl_context);
    g_error_free(error);
    return FALSE;
}

static gboolean
ensure_textures(Emotion_HWAccel_OpenGL *hwaccel, guint width, guint height)
{
    GstGLFuncs * const gl = hwaccel->gl_context->gl_vtable;
    gboolean reset_textures;
    guint i, texture;

    reset_textures = hwaccel->width != width || hwaccel->height != height;
    if (!reset_textures) {
        for (i = 0; i < NUM_TEXTURES; i++)
            reset_textures |= !hwaccel->textures[i];
        if (!reset_textures)
            return TRUE;
    }

    gl->DeleteTextures(NUM_TEXTURES, hwaccel->textures);

    for (i = 0; i < NUM_TEXTURES; i++) {
        gl->GenTextures(1, &texture);
        gl->BindTexture(GL_TEXTURE_2D, texture);
        gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA,
            GL_UNSIGNED_BYTE, NULL);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        hwaccel->textures[i] = texture;
    }

    hwaccel->width = width;
    hwaccel->height = height;
    return TRUE;
}

static gboolean
emotion_hwaccel_opengl_is_available(Evas_Object *obj EINA_UNUSED,
    gint engine_type)
{
    switch (engine_type) {
#if USE_X11 && defined(BUILD_ECORE_EVAS_OPENGL_X11)
    case ECORE_EVAS_ENGINE_OPENGL_X11:
        return TRUE;
#endif
    }
    return FALSE;
}

static gboolean
emotion_hwaccel_opengl_dispose(Emotion_HWAccel_OpenGL *hwaccel EINA_UNUSED)
{
    return TRUE;
}

static void
emotion_hwaccel_opengl_free(Emotion_HWAccel_OpenGL *hwaccel)
{
    g_clear_object(&hwaccel->evas_gl_context);
    g_clear_object(&hwaccel->gl_context);
    g_clear_object(&hwaccel->gl_display);
    emotion_hwaccel_clear(&hwaccel->base);
    g_slice_free(Emotion_HWAccel_OpenGL, hwaccel);
}

static gboolean
emotion_hwaccel_opengl_init(Emotion_HWAccel_OpenGL *hwaccel, Evas_Object *obj,
    gint engine_type)
{
    gst_mini_object_init(GST_MINI_OBJECT_CAST(hwaccel), 0,
        emotion_hwaccel_opengl_get_type(),
        (GstMiniObjectCopyFunction)NULL,
        (GstMiniObjectDisposeFunction)emotion_hwaccel_opengl_dispose,
        (GstMiniObjectFreeFunction)emotion_hwaccel_opengl_free);

    hwaccel->base.info = &emotion_hwaccel_info_opengl;
    hwaccel->base.emotion_object = obj;

    if (!ensure_display(hwaccel, engine_type))
        return FALSE;
    if (!ensure_context(hwaccel))
        return FALSE;
    return TRUE;
}

static Emotion_HWAccel *
emotion_hwaccel_opengl_new(Evas_Object *obj, gint engine_type)
{
    Emotion_HWAccel_OpenGL *hwaccel;

    hwaccel = g_slice_new0(Emotion_HWAccel_OpenGL);
    if (!emotion_hwaccel_opengl_init(hwaccel, obj, engine_type))
        goto error;
    return EMOTION_HWACCEL(hwaccel);

error:
    emotion_hwaccel_opengl_free(hwaccel);
    return NULL;
}

static gboolean
emotion_hwaccel_opengl_propose_allocation(Emotion_HWAccel_OpenGL *hwaccel,
    GstQuery *query)
{
    GstStructure *gl_context_structure;
    gchar *gl_api_string;
    gchar *gl_platform_string;

    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);

    gl_api_string = gst_gl_api_to_string(hwaccel->gl_api);
    if (!gl_api_string)
        return FALSE;

    gl_platform_string = gst_gl_platform_to_string(hwaccel->gl_platform);
    if (!gl_platform_string)
        return FALSE;

    gl_context_structure = gst_structure_new("GstVideoGLTextureUploadMeta",
        "gst.gl.GstGLContext", GST_GL_TYPE_CONTEXT, hwaccel->gl_context,
        "gst.gl.context.handle", G_TYPE_POINTER,
        GSIZE_TO_POINTER(gst_gl_context_get_gl_context(hwaccel->gl_context)),
        "gst.gl.context.type", G_TYPE_STRING, gl_platform_string,
        "gst.gl.context.apis", G_TYPE_STRING, gl_api_string, NULL);

    gst_query_add_allocation_meta(query,
        GST_VIDEO_GL_TEXTURE_UPLOAD_META_API_TYPE, gl_context_structure);

    g_free(gl_api_string);
    g_free(gl_platform_string);
    gst_structure_free (gl_context_structure);
    return TRUE;
}

typedef struct {
    Emotion_HWAccel_OpenGL *hwaccel;
    GstBuffer *buffer;
    Evas_Native_Surface surface;
    gboolean result;
} Emotion_HWAccel_OpenGL_Upload_Data;

static gboolean
emotion_hwaccel_opengl_do_upload_buffer(Emotion_HWAccel_OpenGL *hwaccel,
    GstBuffer *buffer, Evas_Native_Surface *surface)
{
    GstVideoGLTextureUploadMeta *gl_texture_upload_meta;
    guint texture_ids[GST_VIDEO_MAX_PLANES];

    gl_texture_upload_meta =
        gst_buffer_get_video_gl_texture_upload_meta(buffer);
    if (!gl_texture_upload_meta)
        goto error_no_upload_meta;

    gl_texture_upload_meta->texture_orientation =
        GST_VIDEO_GL_TEXTURE_ORIENTATION_X_NORMAL_Y_FLIP;

    texture_ids[0] = surface->data.opengl.texture_id;
    return gst_video_gl_texture_upload_meta_upload(gl_texture_upload_meta,
               texture_ids);

    /* ERRORS */
error_no_upload_meta:
    GST_ERROR("no GstVideoGLTextureUploadMeta in buffer %p", buffer);
    return FALSE;
}

static void
emotion_hwaccel_opengl_do_upload(GstGLContext *gl_context EINA_UNUSED,
    Emotion_HWAccel_OpenGL_Upload_Data *data)
{
    data->result = emotion_hwaccel_opengl_do_upload_buffer(data->hwaccel,
        data->buffer, &data->surface);
}

static gboolean
emotion_hwaccel_opengl_upload(Emotion_HWAccel_OpenGL *hwaccel,
    Emotion_HWAccel_Buffer *emotion_buffer, Evas_Object *image)
{
    Emotion_HWAccel_OpenGL_Upload_Data upload_data;
    Evas_Native_Surface * const ns = &upload_data.surface;
    guint width, height;

    width = GST_VIDEO_INFO_WIDTH(&emotion_buffer->info);
    height = GST_VIDEO_INFO_HEIGHT(&emotion_buffer->info);

    if (!ensure_textures(hwaccel, width, height))
        return FALSE;

    ns->version = EVAS_NATIVE_SURFACE_VERSION;
    ns->type = EVAS_NATIVE_SURFACE_OPENGL;
    ns->data.opengl.texture_id = hwaccel->textures[hwaccel->texture_id];
    ns->data.opengl.framebuffer_id = 0;
    ns->data.opengl.internal_format = GL_RGBA;
    ns->data.opengl.format = GL_BGRA;
    ns->data.opengl.x = 0;
    ns->data.opengl.y = 0;
    ns->data.opengl.w = width;
    ns->data.opengl.h = height;

    upload_data.hwaccel = hwaccel;
    upload_data.buffer = emotion_buffer->frame;
    gst_gl_context_thread_add(hwaccel->gl_context,
        (GstGLContextThreadFunc)emotion_hwaccel_opengl_do_upload, &upload_data);

    hwaccel->texture_id = (hwaccel->texture_id + 1) % NUM_TEXTURES;
    if (!upload_data.result)
        return FALSE;

    evas_object_image_alpha_set(image, 0);
    evas_object_image_colorspace_set(image, emotion_buffer->eformat);
    evas_object_image_size_set(image, width, height);
    evas_object_image_native_surface_set(image, ns);
    evas_object_image_data_update_add(image, 0, 0, width, height);
    evas_object_image_pixels_dirty_set(image, 0);
    return TRUE;
}

const Emotion_HWAccel_Info emotion_hwaccel_info_opengl = {
    .type = EMOTION_HWACCEL_TYPE_OPENGL,
    .name = "emotion-hwaccel-opengl",

    .is_available = emotion_hwaccel_opengl_is_available,
    .allocate = emotion_hwaccel_opengl_new,
    .propose_allocation = (Emotion_HWAccel_ProposeAllocation_Func)
        emotion_hwaccel_opengl_propose_allocation,
    .upload = (Emotion_HWAccel_Upload_Func)
        emotion_hwaccel_opengl_upload,
};
