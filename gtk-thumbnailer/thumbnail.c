/**
 * Copyright (C) 2014 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include <gtk/gtk.h>
#include <metacity-private/theme-parser.h>
#include <metacity-private/preview-widget.h>

#include  "fix_old_gtk_version.h"

typedef struct _ThumbData {
    int width;
    int height;

    char* dest;
    char* background;
} ThumbData;

static GtkWidget* get_preview_from_meta(const char* name);
static void padding_thumbnail(const GtkFixed* fixed);
static void capture(GtkOffscreenWindow* w, GdkEvent* ev, gpointer user_data);

static gboolean init = FALSE;
int
try_init()
{
    if (init) {
        return 0;
    }

    init = TRUE;
    return gtk_init_check(NULL, NULL)?0:-1;
}

int
gtk_thumbnail(const char* name, const char* dest, const char* bg,
              int width, int height)
{
    if (!name || !dest) {
        g_warning("Invalid theme name or dest");
        return -1;
    }

    GtkWidget* w = gtk_offscreen_window_new();
    if (!w) {
        g_warning("New offscreen widnow failed");
        return -1;
    }
    gtk_widget_set_size_request(w, width, height);
    GtkWidget* preview = get_preview_from_meta(name);
    if (!preview) {
        g_warning("get metacity theme preview failed");
        return -1;
    }

    gtk_container_add(GTK_CONTAINER(w), preview);
    GtkWidget* fixed = gtk_fixed_new();
    if (!fixed) {
        g_warning("New fixed failed");
        return -1;
    }
    gtk_container_add(GTK_CONTAINER(preview), fixed);
    padding_thumbnail(GTK_FIXED(fixed));

    ThumbData data;
    data.width = width;
    data.height = height;
    data.dest = (char*)dest;
    data.background = (char*)bg;
    g_signal_connect(G_OBJECT(w), "damage-event",
                     G_CALLBACK(capture), &data);
    gtk_widget_realize(fixed);
    gtk_widget_show_all(w);

    gtk_main();
    return 0;
}

static GtkWidget*
get_preview_from_meta(const char* name)
{
    if (!name) {
        g_warning("Theme name is null");
        return NULL;
    }

    // Init meta_current_theme, otherwise segmentation in metacity
    meta_theme_set_current("", TRUE);

    GError* error = NULL;
    MetaTheme* meta = NULL;
    meta = meta_theme_load(name, &error);
    if (error) {
        g_warning("Load meta theme '%s' failed: %s",
                  name, error->message);
        g_error_free(error);
        return NULL;
    }

    GtkWidget* preview = NULL;
    preview = meta_preview_new();
    if (!preview) {
        g_warning("New metacity preview failed");
        return NULL;
    }

    meta_preview_set_theme((MetaPreview*)preview, meta);
    /*meta_theme_free(meta);*/
    meta_preview_set_title((MetaPreview*)preview, "");

    return preview;
}

static void
padding_thumbnail(const GtkFixed* fixed)
{
    //TODO: Should handle gtk2/gtk3 themes
}

static void
capture(GtkOffscreenWindow* w, GdkEvent* ev, gpointer user_data)
{
    ThumbData* data = (ThumbData*)user_data;
    int width = data->width;
    int height = data->height;
    char* dest = data->dest;
    char* bg = data->background;

    cairo_surface_t* surface = NULL;
    if (bg) {
        surface = cairo_image_surface_create_from_png(bg);
    } else {
        surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            width, height);
    }
    if (!surface) {
        g_warning("Create surface failed\n");
        return;
    }

    GdkWindow* tmp_window = gtk_widget_get_window (GTK_WIDGET (w));
    cairo_surface_t* tmp_surface = gdk_offscreen_window_get_surface (tmp_window);
    if (!tmp_surface) {
        g_warning("Get offscreen surface failed");
        return;
    }
    GdkPixbuf*  pbuf = gdk_pixbuf_get_from_surface (tmp_surface,
                                                    0, 0,
                                                    gdk_window_get_width (tmp_window),
                                                    gdk_window_get_height (tmp_window));
    /*GdkPixbuf* pbuf = gtk_offscreen_window_get_pixbuf(w);*/
    cairo_t* cairo = cairo_create(surface);
    if (pbuf) {
        gdk_cairo_set_source_pixbuf(cairo, pbuf, -15, 15);
        cairo_paint(cairo);
        cairo_surface_write_to_png(surface, dest);

        g_object_unref(G_OBJECT(pbuf));
    }

    cairo_destroy(cairo);
    cairo_surface_destroy(surface);

    gtk_main_quit();
}