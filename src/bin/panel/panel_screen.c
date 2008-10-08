/**
 * @file
 *
 * Copyright (C) 2008 by ProFUSION embedded systems
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the  GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 * @author Rafael Antognolli <antognolli@profusion.mobi>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "enjoy_panel.h"
#include <guarana_widgets.h>
#include <Edje.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define EJY_PANEL_SCREEN_DATA_GET(o, ptr)                           \
    struct ejy_panel_screen_data *ptr = evas_object_smart_data_get(o)

#define EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, ptr)                 \
    EJY_PANEL_SCREEN_DATA_GET(o, ptr);                              \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return;                                                         \
    }

#define EJY_PANEL_SCREEN_DATA_GET_OR_RETURN_VAL(o, ptr, val)        \
    EJY_PANEL_SCREEN_DATA_GET(o, ptr);                              \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return val;                                                     \
    }

struct ejy_panel_screen_data {
    struct grn_edje_widget_data base;
    struct {
        const char *title;
        const char *artist;
        const char *album;
        const char *filename;
    }file;
    Evas_Bool playing;
};

static struct grn_widget_api _parent_api = {{NULL}};
static const char EDJE_SIG_PLAY[] = "gui,action,play";
static const char EDJE_SIG_PAUSE[] = "gui,action,pause";
static const char EDJE_SIG_STOP[] = "gui,action,stop";
static const char EDJE_SIG_REWIND[] = "gui,action,rewind";
static const char EDJE_SIG_FORWARD[] = "gui,action,forward";
static const char EDJE_SIG_PREVIOUS[] = "gui,action,previous";
static const char EDJE_SIG_NEXT[] = "gui,action,next";
static const char EDJE_SIG_CHANGE_VIEW[] = "view,change,*";
static const char EDJE_ACTION_PLAYING[] = "gui,action,playing";
static const char EDJE_ACTION_STOPPED[] = "gui,action,stopped";
static const char EDJE_PART_TITLE[] = "gui.text.title";
static const char EDJE_PART_ARTIST[] = "gui.text.artist";
static const char EDJE_PART_ALBUM[] = "gui.text.album";
static const char EDJE_PART_FILENAME[] = "gui.text.filename";
static const int EDJE_MESSAGE_ID_PROGRESS = 1;
static const int EDJE_MESSAGE_ID_VOLUME = 2;
static const char ACTION_PROGRESS_SET[] = "gui,action,progress_set";
static const char ACTION_VOLUME_SET[] = "gui,action,volume_set";
static const char ACTION_CHANGE_VIEW[] = "view,change";

static void
_ejy_panel_screen_title_set(Evas_Object *o, struct ejy_panel_screen_data *priv)
{
    edje_object_part_text_set(
        priv->base.edje, EDJE_PART_TITLE, priv->file.title);
}

static void
_ejy_panel_screen_artist_set(Evas_Object *o, struct ejy_panel_screen_data *priv)
{
    edje_object_part_text_set(
        priv->base.edje, EDJE_PART_ARTIST, priv->file.artist);
}

static void
_ejy_panel_screen_album_set(Evas_Object *o, struct ejy_panel_screen_data *priv)
{
    edje_object_part_text_set(
        priv->base.edje, EDJE_PART_ALBUM, priv->file.album);
}

static void
_ejy_panel_screen_filename_set(Evas_Object *o, struct ejy_panel_screen_data *priv)
{
    edje_object_part_text_set(
        priv->base.edje, EDJE_PART_FILENAME, priv->file.filename);
}

static void
_ejy_panel_screen_message_handler(void *data, Evas_Object *edje, Edje_Message_Type type, int id, void *msg)
{
    Evas_Object *o = data;

    if (id == EDJE_MESSAGE_ID_PROGRESS && type == EDJE_MESSAGE_FLOAT) {
        struct _Edje_Message_Float *msgfloat = msg;
        evas_object_smart_callback_call(o, ACTION_PROGRESS_SET,
                                        &msgfloat->val);
    } else if (id == EDJE_MESSAGE_ID_VOLUME && type == EDJE_MESSAGE_FLOAT) {
        struct _Edje_Message_Float *msgfloat = msg;
        evas_object_smart_callback_call(o, ACTION_VOLUME_SET,
                                        &msgfloat->val);
    }
}

static void
_ejy_panel_screen_change_view_cb(void *data, Evas_Object *edje, const char *emission, const char *source)
{
    Evas_Object *o = data;

    evas_object_smart_callback_call(o, ACTION_CHANGE_VIEW, (char *)emission);
}

static void
_ejy_panel_button_clicked_cb(void *data, Evas_Object *edje, const char *emission, const char *source)
{
    Evas_Object *o = data;

    evas_object_smart_callback_call(o, emission, NULL);
}

static const struct callback_connections {
    const char *signal;
    void (*cb)(void *, Evas_Object *, const char *, const char *);
} *_itr, _items[] = {
    {EDJE_SIG_PLAY, _ejy_panel_button_clicked_cb},
    {EDJE_SIG_PAUSE, _ejy_panel_button_clicked_cb},
    {EDJE_SIG_STOP, _ejy_panel_button_clicked_cb},
    {EDJE_SIG_REWIND, _ejy_panel_button_clicked_cb},
    {EDJE_SIG_FORWARD, _ejy_panel_button_clicked_cb},
    {EDJE_SIG_PREVIOUS, _ejy_panel_button_clicked_cb},
    {EDJE_SIG_NEXT, _ejy_panel_button_clicked_cb},
    {EDJE_SIG_CHANGE_VIEW, _ejy_panel_screen_change_view_cb},
    {NULL, NULL}
};

static void
_ejy_panel_screen_theme_changed(Evas_Object *o, grn_widget_callback_t cb_end, void *data)
{
    EJY_PANEL_SCREEN_DATA_GET(o, priv);

    for (_itr = _items; _itr->signal != NULL; _itr++) {
        if (_itr->cb)
            edje_object_signal_callback_del(priv->base.edje,
                                            _itr->signal, "", _itr->cb);
    }
    edje_object_message_handler_set(
        priv->base.edje, NULL, NULL);

    _parent_api.theme_changed(o, cb_end, data);

    for (_itr = _items; _itr->signal != NULL; _itr++) {
        if (_itr->cb)
            edje_object_signal_callback_add(priv->base.edje,
                                            _itr->signal, "", _itr->cb, o);
    }
    edje_object_message_handler_set(
        priv->base.edje, _ejy_panel_screen_message_handler, o);
}

static Evas_Bool
_ejy_panel_screen_key_down(Evas_Object *o, Evas_Event_Key *info)
{
    return _parent_api.key_down(o, info);
}

static void
_ejy_panel_screen_smart_add(Evas_Object *o)
{
    struct ejy_panel_screen_data *priv;

    priv = grn_widget_setup_private_data(o, sizeof(*priv));
    _parent_api.sc.add(o);

    priv->file.filename = NULL;
    priv->file.title = NULL;
    priv->file.artist = NULL;
    priv->file.album = NULL;

    grn_edje_widget_group_set(o, "screen/panel");
}

Evas_Object *
ejy_panel_screen_add(Evas *evas)
{
    static Evas_Smart *smart = NULL;

    if (!smart) {
        static struct grn_widget_api api = {
            {"Panel", EVAS_SMART_CLASS_VERSION}
        };

        grn_edje_widget_smart_set(&api);
        _parent_api = api;

        api.sc.add = _ejy_panel_screen_smart_add;
        api.theme_changed = _ejy_panel_screen_theme_changed;
        api.key_down = _ejy_panel_screen_key_down;
        smart = evas_smart_class_new(&api.sc);
    }

    return evas_object_smart_add(evas, smart);
}

void
ejy_panel_screen_title_set(Evas_Object *o, const char *title)
{
    EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, priv);
    title = evas_stringshare_add(title);
    evas_stringshare_del(priv->file.title);
    if (priv->file.title == title)
        return;

    priv->file.title = title;
    _ejy_panel_screen_title_set(o, priv);
}

void
ejy_panel_screen_artist_set(Evas_Object *o, const char *artist)
{
    EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, priv);
    artist = evas_stringshare_add(artist);
    evas_stringshare_del(priv->file.artist);
    if (priv->file.artist == artist)
        return;

    priv->file.artist = artist;
    _ejy_panel_screen_artist_set(o, priv);
}

void
ejy_panel_screen_album_set(Evas_Object *o, const char *album)
{
    EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, priv);
    album = evas_stringshare_add(album);
    evas_stringshare_del(priv->file.album);
    if (priv->file.album == album)
        return;

    priv->file.album = album;
    _ejy_panel_screen_album_set(o, priv);
}

void
ejy_panel_screen_filename_set(Evas_Object *o, const char *filename)
{
    EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, priv);
    filename = evas_stringshare_add(filename);
    evas_stringshare_del(priv->file.filename);
    if (priv->file.filename == filename)
        return;

    priv->file.filename = filename;
    _ejy_panel_screen_filename_set(o, priv);
}

void
ejy_panel_screen_play_set(Evas_Object *o, Evas_Bool v)
{
    const char *emission;
    EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, priv);

    if (priv->playing == v)
        return;

    priv->playing = v;
    if (priv->playing)
        emission = EDJE_ACTION_PLAYING;
    else
        emission = EDJE_ACTION_STOPPED;
    edje_object_signal_emit(priv->base.edje, emission, "");
}

void
ejy_panel_screen_progress_update(Evas_Object *o, double progress, double total)
{
    struct {
        Edje_Message_Float_Set base;
        double total;
    } msg = {{2, {progress}}, total};
    EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, priv);

    edje_object_message_send(priv->base.edje, EDJE_MESSAGE_FLOAT_SET,
                             EDJE_MESSAGE_ID_PROGRESS, &msg);
}

void
ejy_panel_screen_volume_set(Evas_Object *o, double volume)
{
    struct _Edje_Message_Float msgfloat;
    EJY_PANEL_SCREEN_DATA_GET_OR_RETURN(o, priv);

    msgfloat.val = volume;
    edje_object_message_send(priv->base.edje, EDJE_MESSAGE_FLOAT,
                             EDJE_MESSAGE_ID_VOLUME, &msgfloat);
}
