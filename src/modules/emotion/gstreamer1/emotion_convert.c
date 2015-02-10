#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "emotion_gstreamer.h"

static void
_evas_video_bgrx(unsigned char *evas_data, GstVideoFrame *gst_frame)
{
    GstVideoInfo * const vip = &gst_frame->info;
    const guchar *gst_line = gst_frame->data[0];
    guint x, y, w, h, stride, step;

    w = GST_VIDEO_INFO_WIDTH(vip);
    h = GST_VIDEO_INFO_HEIGHT(vip);
    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, 0);
    step = GST_VIDEO_INFO_COMP_PSTRIDE(vip, 0);

    for (y = 0; y < h; y++) {
        const guchar *gst_data = gst_line;
        for (x = 0; x < w; x++) {
            evas_data[0] = gst_data[0];
            evas_data[1] = gst_data[1];
            evas_data[2] = gst_data[2];
            evas_data[3] = 255;
            gst_data += step;
            evas_data += 4;
        }
        gst_line += stride;
    }
}

static void
_evas_video_bgra(unsigned char *evas_data, GstVideoFrame *gst_frame)
{
    GstVideoInfo * const vip = &gst_frame->info;
    const guchar *gst_line = gst_frame->data[0];
    guint x, y, w, h, stride;
    guint8 alpha;

    w = GST_VIDEO_INFO_WIDTH(vip);
    h = GST_VIDEO_INFO_HEIGHT(vip);
    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, 0);

    for (y = 0; y < h; y++) {
        const guchar *gst_data = gst_line;
        for (x = 0; x < w; x++) {
            alpha = gst_data[3];
            evas_data[0] = (gst_data[0] * alpha) / 255;
            evas_data[1] = (gst_data[1] * alpha) / 255;
            evas_data[2] = (gst_data[2] * alpha) / 255;
            evas_data[3] = alpha;
            gst_data += 4;
            evas_data += 4;
        }
        gst_line += stride;
    }
}

static void
_evas_video_yuv420p(unsigned char *evas_data, GstVideoFrame *gst_frame)
{
    GstVideoInfo * const vip = &gst_frame->info;
    const guchar ** const rows = (const guchar **)evas_data;
    const guchar *gst_data;
    guint i, j, c, h, stride;

    h = GST_VIDEO_INFO_HEIGHT(vip);
    c = 0;

    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, c);
    gst_data = (guchar *)gst_frame->data[c];
    for (i = 0; i < h; i++)
        rows[i] = &gst_data[i * stride];

    h = (h + 1) / 2;
    c = 1 + !(GST_VIDEO_INFO_FORMAT(vip) == GST_VIDEO_FORMAT_I420);

    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, c);
    gst_data = (guchar *)gst_frame->data[c];
    for (j = 0; j < h; j++, i++)
        rows[i] = &gst_data[j * stride];

    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, c ^ 3);
    gst_data = (guchar *)gst_frame->data[c ^ 3];
    for (j = 0; j < h; j++, i++)
        rows[i] = &gst_data[j * stride];
}

static void
_evas_video_yuy2(unsigned char *evas_data, GstVideoFrame *gst_frame)
{
    GstVideoInfo * const vip = &gst_frame->info;
    const guchar ** const rows = (const guchar **)evas_data;
    const guchar *gst_data;
    guint i, h, stride;

    h = GST_VIDEO_INFO_HEIGHT(vip);

    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, 0);
    gst_data = (guchar *)gst_frame->data[0];

    for (i = 0; i < h; i++)
        rows[i] = &gst_data[i * stride];
}

static void
_evas_video_nv12(unsigned char *evas_data, GstVideoFrame *gst_frame)
{
    GstVideoInfo * const vip = &gst_frame->info;
    const guchar ** const rows = (const guchar **)evas_data;
    const guchar *gst_data;
    guint i, j, h, stride;

    h = GST_VIDEO_INFO_HEIGHT(vip);

    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, 0);
    gst_data = (guchar *)gst_frame->data[0];

    for (i = 0; i < h; i++)
        rows[i] = &gst_data[i * stride];

    h = (h + 1) / 2;

    stride = GST_VIDEO_INFO_COMP_STRIDE(vip, 1);
    gst_data = (guchar *)gst_frame->data[1];

    for (j = 0; j < h; j++, i++)
        rows[i] = &gst_data[j * stride];
}

const ColorSpace_Format_Convertion colorspace_format_convertion[] = {
  { "I420", GST_VIDEO_FORMAT_I420, EVAS_COLORSPACE_YCBCR422P601_PL, _evas_video_yuv420p, EINA_TRUE },
  { "YV12", GST_VIDEO_FORMAT_YV12, EVAS_COLORSPACE_YCBCR422P601_PL, _evas_video_yuv420p, EINA_TRUE },
  { "YUY2", GST_VIDEO_FORMAT_YUY2, EVAS_COLORSPACE_YCBCR422601_PL, _evas_video_yuy2, EINA_FALSE },
  { "NV12", GST_VIDEO_FORMAT_NV12, EVAS_COLORSPACE_YCBCR420NV12601_PL, _evas_video_nv12, EINA_TRUE },
  { "BGR", GST_VIDEO_FORMAT_BGR, EVAS_COLORSPACE_ARGB8888, _evas_video_bgrx, EINA_FALSE },
  { "BGRx", GST_VIDEO_FORMAT_BGRx, EVAS_COLORSPACE_ARGB8888, _evas_video_bgrx, EINA_FALSE },
  { "BGRA", GST_VIDEO_FORMAT_BGRA, EVAS_COLORSPACE_ARGB8888, _evas_video_bgra, EINA_FALSE },
  { NULL, 0, 0, NULL, 0 }
};

