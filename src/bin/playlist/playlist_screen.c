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
#include "enjoy_playlist.h"
#include <guarana_widgets.h>
#include <Edje.h>
#include <Ecore.h>
#include <stdio.h>
#include <string.h>

#define EJY_PLAYLIST_SCREEN_DATA_GET(o, ptr)                            \
    struct ejy_playlist_screen_data *ptr = evas_object_smart_data_get(o)

#define EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN(o, ptr)                  \
    EJY_PLAYLIST_SCREEN_DATA_GET(o, ptr);                               \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return;                                                         \
    }

#define EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN_VAL(o, ptr, val)         \
    EJY_PLAYLIST_SCREEN_DATA_GET(o, ptr);                               \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return val;                                                     \
    }

struct ejy_playlist_screen_data {
    struct grn_edje_widget_data base;
    Evas_Object *list;
    grn_accessor_t *accessor;
    struct {
        Evas_Bool down;
        Evas_Bool is_click;
        Evas_Coord cur_y;
        Evas_Coord threshold;
        int idx_clicked;
        Evas_Bool is_double;
    } mouse;
    struct {
        Ecore_Timer *double_click;
    } timer;
    unsigned int model_count;
};

static struct grn_widget_api _parent_api = {{NULL}};
static const char EDJE_PART_LIST[] = "gui.list";
static const char EDJE_PART_EVENT_AREA[] = "gui.event_area";
static const char EDJE_ACTION_PLAY[] = "gui,action,play";
static const char EDJE_ACTION_STOP[] = "gui,action,stop";
static const char EDJE_ACTION_SCROLLING[] = "gui,action,scrolling";
static const int EDJE_MESSAGE_ID_SCROLL = 1;
static const int EDJE_MESSAGE_ID_SCROLL_RESIZE = 2;

static void
_ejy_playlist_screen_scrollbar_resize(struct ejy_playlist_screen_data *priv)
{
    struct _Edje_Message_Float msgfloat;

    msgfloat.val = grn_base_list_visible_rows_scale_get(priv->list);
    edje_object_message_send(priv->base.edje, EDJE_MESSAGE_FLOAT,
                             EDJE_MESSAGE_ID_SCROLL_RESIZE, &msgfloat);
}

static void
_ejy_playlist_screen_message_handler(void *data, Evas_Object *edje, Edje_Message_Type type, int id, void *msg)
{
    Evas_Object *o = data;
    EJY_PLAYLIST_SCREEN_DATA_GET(o, priv);

    if (id == EDJE_MESSAGE_ID_SCROLL && type == EDJE_MESSAGE_FLOAT) {
        struct _Edje_Message_Float *msgfloat = msg;
        grn_base_list_position_set(priv->list, msgfloat->val);
    }
}

static void
_ejy_playlist_screen_list_changed_position(void *data, Evas_Object *list, void *einfo)
{
    struct _Edje_Message_Float msgfloat;
    Evas_Object *o = data;
    double *values = einfo;
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN(o, priv);

    msgfloat.val = values[1];
    edje_object_message_send(priv->base.edje, EDJE_MESSAGE_FLOAT,
                             EDJE_MESSAGE_ID_SCROLL, &msgfloat);
}

static void
_ejy_playlist_screen_event_area_mouse_down(void *data, Evas *e, Evas_Object *event_area, void *event_info)
{
    Evas_Object *o = data;
    EJY_PLAYLIST_SCREEN_DATA_GET(o, priv);
    Evas_Event_Mouse_Down *event = event_info;

    if (event->button == 1) {
        priv->mouse.down = 1;
        priv->mouse.is_click = 1;
        priv->mouse.cur_y = event->canvas.y;
    }
}

static int
_ejy_playlist_screen_double_click_stop(void *data)
{
    Evas_Object *o = data;
    EJY_PLAYLIST_SCREEN_DATA_GET(o, priv);

    priv->mouse.is_double = 0;
    priv->timer.double_click = NULL;

    return 0;
}

static void
_ejy_playlist_screen_event_area_mouse_up(void *data, Evas *e, Evas_Object *event_area, void *event_info)
{
    Evas_Object *o = data;
    EJY_PLAYLIST_SCREEN_DATA_GET(o, priv);
    Evas_Event_Mouse_Up *event = event_info;

    if (event->button != 1)
        return;

    priv->mouse.down = 0;

    if (priv->mouse.is_click) {
        int index = grn_base_list_index_at_y(priv->list, event->canvas.y);
        if (priv->mouse.is_double && index == priv->mouse.idx_clicked) {
            priv->mouse.is_double = 0;
            evas_object_smart_callback_call(o, "double_clicked", &index);
        } else {
            priv->mouse.is_double = 1;
            priv->mouse.idx_clicked = index;
            if (priv->timer.double_click)
                ecore_timer_del(priv->timer.double_click);
            priv->timer.double_click = ecore_timer_add(
                1, _ejy_playlist_screen_double_click_stop, o);
            evas_object_smart_callback_call(o, "selected", &index);
        }
    }
}

static void
_ejy_playlist_screen_event_area_mouse_move(void *data, Evas *e, Evas_Object *event_area, void *event_info)
{
    Evas_Object *o = data;
    EJY_PLAYLIST_SCREEN_DATA_GET(o, priv);
    Evas_Event_Mouse_Move *event = event_info;
    Evas_Coord dy;

    if (!priv->mouse.down)
        return;

    dy = event->cur.canvas.y - priv->mouse.cur_y;
    if (priv->mouse.is_click)
        if (dy > priv->mouse.threshold || dy < -priv->mouse.threshold) {
            priv->mouse.is_click = 0;
            priv->mouse.is_double = 0;
        }

    if (!priv->mouse.is_click) {
        edje_object_signal_emit(priv->base.edje, EDJE_ACTION_SCROLLING, "");
        grn_base_list_move_offset(priv->list, dy);
        priv->mouse.cur_y = event->cur.canvas.y;
    }
}

static void
_ejy_playlist_screen_theme_changed(Evas_Object *o, grn_widget_callback_t cb_end, void *data)
{
    EJY_PLAYLIST_SCREEN_DATA_GET(o, priv);
    Evas_Object *event_area;

    edje_object_message_handler_set(
        priv->base.edje, NULL, NULL);
    evas_object_smart_callback_del(
        priv->list, "changed,position",
        _ejy_playlist_screen_list_changed_position);
    event_area = (Evas_Object *)edje_object_part_object_get(
        priv->base.edje, EDJE_PART_EVENT_AREA);
    evas_object_event_callback_del(
        event_area, EVAS_CALLBACK_MOUSE_DOWN,
        _ejy_playlist_screen_event_area_mouse_down);
    evas_object_event_callback_del(
        event_area, EVAS_CALLBACK_MOUSE_UP,
        _ejy_playlist_screen_event_area_mouse_up);
    evas_object_event_callback_del(
        event_area, EVAS_CALLBACK_MOUSE_MOVE,
        _ejy_playlist_screen_event_area_mouse_move);

    _parent_api.theme_changed(o, cb_end, data);

    grn_widget_theme_changed(priv->list, NULL, NULL);
    edje_object_part_swallow(priv->base.edje, EDJE_PART_LIST, priv->list);
    event_area = (Evas_Object *)edje_object_part_object_get(
        priv->base.edje, EDJE_PART_EVENT_AREA);

    edje_object_message_handler_set(
        priv->base.edje, _ejy_playlist_screen_message_handler, o);
    evas_object_smart_callback_add(
        priv->list, "changed,position",
        _ejy_playlist_screen_list_changed_position, o);
    evas_object_event_callback_add(
        event_area, EVAS_CALLBACK_MOUSE_DOWN,
        _ejy_playlist_screen_event_area_mouse_down, o);
    evas_object_event_callback_add(
        event_area, EVAS_CALLBACK_MOUSE_UP,
        _ejy_playlist_screen_event_area_mouse_up, o);
    evas_object_event_callback_add(
        event_area, EVAS_CALLBACK_MOUSE_MOVE,
        _ejy_playlist_screen_event_area_mouse_move, o);
}

static void
_ejy_playlist_screen_smart_add(Evas_Object *o)
{
    struct ejy_playlist_screen_data *priv;

    priv = grn_widget_setup_private_data(o, sizeof(*priv));
    _parent_api.sc.add(o);

    priv->accessor = NULL;
    priv->list = ejy_playlist_screen_list_add_to(o);
    priv->mouse.down = 0;
    priv->mouse.threshold = 10;
    priv->mouse.is_double = 0;
    priv->model_count = 0;
    priv->timer.double_click = NULL;

    grn_edje_widget_group_set(o, "screen/playlist");
}

static void
_ejy_playlist_screen_smart_del(Evas_Object *o)
{
    EJY_PLAYLIST_SCREEN_DATA_GET(o, priv);

    if (priv->accessor)
        grn_accessor_free(priv->accessor);
    evas_object_del(priv->list);

    if (priv->timer.double_click)
        ecore_timer_del(priv->timer.double_click);

    _parent_api.sc.del(o);
}

static Evas_Bool
_ejy_playlist_screen_key_down(Evas_Object *o, Evas_Event_Key *info)
{
    return 1;
}

Evas_Object *
ejy_playlist_screen_add(Evas *evas)
{
    static Evas_Smart *smart = NULL;

    if (!smart) {
        static struct grn_widget_api api = {
            {"Playlist", EVAS_SMART_CLASS_VERSION}
        };

        grn_edje_widget_smart_set(&api);
        _parent_api = api;

        api.sc.add = _ejy_playlist_screen_smart_add;
        api.sc.del = _ejy_playlist_screen_smart_del;
        api.theme_changed = _ejy_playlist_screen_theme_changed;
        api.key_down = _ejy_playlist_screen_key_down;
        smart = evas_smart_class_new(&api.sc);
    }

    return evas_object_smart_add(evas, smart);
}

void
ejy_playlist_screen_model_updated(Evas_Object *o)
{
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN(o, priv);

    if (priv->accessor)
        grn_accessor_free(priv->accessor);

    priv->accessor = grn_model_folder_accessor(
        grn_base_list_model_get(priv->list));

    _ejy_playlist_screen_scrollbar_resize(priv);

    priv->model_count = grn_model_folder_count(
        grn_base_list_model_get(priv->list));
}

void
ejy_playlist_screen_model_set(Evas_Object *o, grn_model_folder_t *model)
{
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN(o, priv);
    grn_base_list_model_set(priv->list, model);
    ejy_playlist_screen_model_updated(o);
}

void
ejy_playlist_screen_selection_set(Evas_Object *o, int index, Evas_Bool value)
{
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN(o, priv);
    grn_single_selection_list_selection_set(priv->list, index, value);
    if (value)
        grn_base_list_make_visible(priv->list, index);
}

int
ejy_playlist_screen_selection_get(Evas_Object *o)
{
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN_VAL(o, priv, -2);
    return grn_single_selection_list_selection_get(priv->list);
}

void
ejy_playlist_screen_loaded_set(Evas_Object *o, int index, Evas_Bool value)
{
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN(o, priv);

    ejy_playlist_screen_list_loaded_set(priv->list, index, value);
}

struct ejy_playlist_model_child *
ejy_playlist_screen_model_get_by_index(Evas_Object *o, int index)
{
    struct ejy_playlist_model_child *child;
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN_VAL(o, priv, NULL);

    if (!priv->accessor)
        return NULL;

    if (!grn_accessor_get(priv->accessor, index, (void **)&child)) {
        fprintf(stderr, "ERROR: could not get playlist entry for index %d\n",
                index);
        return NULL;
    }

    return child;
}

void
ejy_playlist_screen_change_view(Evas_Object *o, const char *emission)
{
    EJY_PLAYLIST_SCREEN_DATA_GET_OR_RETURN(o, priv);

    edje_object_signal_emit(priv->base.edje, emission, "");
}
