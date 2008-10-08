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
#define _GNU_SOURCE
#include "enjoy_playlist.h"
#include <guarana_widgets.h>
#include <guarana_utils.h>
#include <Edje.h>
#include <stdio.h>
#include <string.h>

#define EJY_PLAYLIST_SCREEN_LIST_DATA_GET(o, ptr)                       \
    struct ejy_playlist_screen_list_data *ptr = evas_object_smart_data_get(o)

#define EJY_PLAYLIST_SCREEN_LIST_DATA_GET_OR_RETURN(o, ptr)             \
    EJY_PLAYLIST_SCREEN_LIST_DATA_GET(o, ptr);                          \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return;                                                         \
    }

#define EJY_PLAYLIST_SCREEN_LIST_DATA_GET_OR_RETURN_VAL(o, ptr, val)    \
    EJY_PLAYLIST_SCREEN_LIST_DATA_GET(o, ptr);                          \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return val;                                                     \
    }

static struct grn_base_list_api _parent_list_api;

static const char EDJE_PART_TITLE[] = "gui.title";
static const char EDJE_ACTION_ROW_ODD[] = "gui,action,row_odd";
static const char EDJE_ACTION_ROW_EVEN[] = "gui,action,row_even";

struct ejy_playlist_screen_list_data {
    struct grn_single_selection_list_data base;
    int idx_loaded;
};

static void
_ejy_playlist_screen_list_renderer_value_set(Evas_Object *list, Evas_Object *renderer, int row_idx, const void *row_data, void *data)
{
    const struct ejy_playlist_model_child *child = row_data;
    const char *emission;
    EJY_PLAYLIST_SCREEN_LIST_DATA_GET(list, priv);

    edje_object_part_text_set(renderer, EDJE_PART_TITLE,
                              child->file.title);

    if (row_idx % 2)
        emission = EDJE_ACTION_ROW_ODD;
    else
        emission = EDJE_ACTION_ROW_EVEN;

    if (row_idx == priv->idx_loaded)
        edje_object_signal_emit(renderer, "gui,action,loaded", "");
    else
        edje_object_signal_emit(renderer, "gui,action,unloaded", "");

    edje_object_signal_emit(renderer, emission, "");
}

static Evas_Bool
_ejy_playlist_screen_list_key_down(Evas_Object *o, Evas_Event_Key *info)
{
    if (strcmp(info->keyname, "Up") == 0) {
        evas_object_smart_callback_call(o, "previous", NULL);
        return 1;
    } else if (strcmp(info->keyname, "Down") == 0) {
        evas_object_smart_callback_call(o, "next", NULL);
        return 1;
    } else if (strcmp(info->keyname, "Escape") == 0) {
        evas_object_smart_callback_call(o, "back", NULL);
        return 1;
    }
    return 0;
}

static void
_ejy_playlist_screen_list_theme_changed(Evas_Object *o, grn_widget_callback_t cb_end, void *data)
{
    Evas_Coord height;

    grn_base_list_freeze(o);
    _parent_list_api.widget.theme_changed(o, NULL, NULL);
    height = grn_base_list_renderer_edje_height_get(o);
    grn_base_list_renderer_height_set(o, height);
    grn_base_list_thaw(o);

    GRN_WIDGET_CB_CALL(cb_end, o, data);
}

/*
static void
_ejy_playlist_screen_list_renderer_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
    fprintf(stderr, "%p: clicked!\n", o);
}

static Evas_Bool
_ejy_playlist_screen_list_reconfigure(Evas_Object *o, struct grn_base_list_data *priv, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
    Evas_List *l;

    for (l = priv->renderers.list; l; l = l->next) {
        Evas_Object *renderer = l->data;
        edje_object_signal_callback_del(
            renderer, "gui,action,clicked", "",
            _ejy_playlist_screen_list_renderer_clicked_cb);
    }

    _parent_list_api.reconfigure(o, priv, x, y, w, h);

    for (l = priv->renderers.list; l; l = l->next) {
        Evas_Object *renderer = l->data;
        edje_object_signal_callback_add(
            renderer, "gui,action,clicked", "",
            _ejy_playlist_screen_list_renderer_clicked_cb, o);
    }

    return 1;
}
*/

static void
_ejy_playlist_screen_list_smart_add(Evas_Object *o)
{
    struct ejy_playlist_screen_list_data *priv;

    priv = grn_widget_setup_private_data(o, sizeof(*priv));
    _parent_list_api.widget.sc.add(o);

    priv->idx_loaded = -1;
}

Evas_Object *
ejy_playlist_screen_list_add_to(Evas_Object *parent)
{
    static Evas_Smart *smart = NULL;
    Evas *e;
    Evas_Object *o;
    Evas_Coord height;
    static struct grn_single_selection_list_renderer_api renderer_api = {
        {NULL, NULL, GRN_BASE_LIST_RENDERER_API_VERSION, NULL,
         _ejy_playlist_screen_list_renderer_value_set,
         NULL,
        },
        NULL,
    };
    static struct grn_base_list_model_api model_api = {
        NULL,
        NULL,
        GRN_BASE_LIST_MODEL_API_VERSION
    };

    if (!renderer_api.base.new)
        grn_single_selection_list_renderer_api_edje_set(
            &renderer_api, "screen/playlist/row_renderer");
    if (!model_api.accessor_get)
        grn_base_list_model_api_model_folder_set(&model_api);

    if (!smart) {
        static struct grn_base_list_api api = {
            {{"Playlist_List", EVAS_SMART_CLASS_VERSION}}
        };
        grn_single_selection_list_smart_set(&api);
        _parent_list_api = api;

        // api.reconfigure = _ejy_playlist_screen_list_reconfigure;
        api.widget.key_down = _ejy_playlist_screen_list_key_down;
        api.widget.theme_changed = _ejy_playlist_screen_list_theme_changed;
        api.widget.sc.add = _ejy_playlist_screen_list_smart_add;

        smart = evas_smart_class_new(&api.widget.sc);
    }

    e = evas_object_evas_get(parent);
    o = evas_object_smart_add(e, smart);
    grn_widget_parent_set(o, parent, NULL, NULL);
    grn_single_selection_list_renderer_api_set(o, 0, &renderer_api);
    grn_base_list_model_api_set(o, &model_api);
    height = grn_base_list_renderer_edje_height_get(o);
    grn_base_list_renderer_height_set(o, height);

    return o;
}

void
ejy_playlist_screen_list_loaded_set(Evas_Object *o, int index, Evas_Bool value)
{
    EJY_PLAYLIST_SCREEN_LIST_DATA_GET_OR_RETURN(o, priv);

    // TODO: set on playlist theme and unset old played
    if (value) {
        priv->idx_loaded = index;
        grn_base_list_make_visible(o, index);
    } else if (priv->idx_loaded == index)
        priv->idx_loaded = -1;

    grn_base_list_model_updated(o);
}
