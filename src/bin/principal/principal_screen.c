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
#include "enjoy_principal.h"
#include <guarana_widgets.h>
#include <Edje.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define EJY_PRINCIPAL_SCREEN_DATA_GET(o, ptr)                           \
    struct ejy_principal_screen_data *ptr = evas_object_smart_data_get(o)

#define EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN(o, ptr)                 \
    EJY_PRINCIPAL_SCREEN_DATA_GET(o, ptr);                              \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return;                                                         \
    }

#define EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN_VAL(o, ptr, val)        \
    EJY_PRINCIPAL_SCREEN_DATA_GET(o, ptr);                              \
    if (!ptr) {                                                         \
        fprintf(stderr, "ERROR: %s(): no widget data for object %p\n",  \
                __FUNCTION__, o);                                       \
        return val;                                                     \
    }

struct ejy_principal_screen_data {
    struct grn_edje_widget_data base;
    Evas_Object *panel;
    Evas_Object *playlist;
};

static struct grn_widget_api _parent_api = {{NULL}};

static const char EDJE_PART_PANEL[] = "gui.panel";
static const char EDJE_PART_PLAYLIST[] = "gui.playlist";
static const char EDJE_DATA_SIZE_HINT[] = "size_hint";
static const char EDJE_DATA_BORDERLESS[] = "borderless";
static const char EDJE_DATA_SIZE_STEP[] = "size_step";

static void
_ejy_principal_screen_theme_changed(Evas_Object *o, grn_widget_callback_t cb_end, void *data)
{
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN(o, priv);

    _parent_api.theme_changed(o, cb_end, data);

    if (priv->panel) {
        grn_widget_theme_changed(priv->panel, NULL, NULL);
        edje_object_part_swallow(
            priv->base.edje, EDJE_PART_PANEL, priv->panel);
    }
}

static Evas_Bool
_ejy_principal_screen_key_down(Evas_Object *o, Evas_Event_Key *info)
{
    if (strcmp(info->keyname, "F9") == 0) {
        evas_object_smart_callback_call(o, "shutdown", NULL);
        return 1;
    } else if (strcmp(info->keyname, "Up") == 0) {
        evas_object_smart_callback_call(o, "pl_up", NULL);
        return 1;
    } else if (strcmp(info->keyname, "Down") == 0) {
        evas_object_smart_callback_call(o, "pl_down", NULL);
        return 1;
    } else if (strcmp(info->keyname, "Return") == 0) {
        evas_object_smart_callback_call(o, "pl_play", NULL);
        return 1;
    } else if (strcmp(info->keyname, "Home") == 0) {
        evas_object_smart_callback_call(o, "pl_home", NULL);
        return 1;
    } else if (strcmp(info->keyname, "End") == 0) {
        evas_object_smart_callback_call(o, "pl_end", NULL);
        return 1;
    } else if (strcmp(info->keyname, "PgUp") == 0) {
        evas_object_smart_callback_call(o, "pl_pgup", NULL);
        return 1;
    } else if (strcmp(info->keyname, "PgDown") == 0) {
        evas_object_smart_callback_call(o, "pl_pgdown", NULL);
        return 1;
    } else if (strcmp(info->keyname, "f") == 0) {
        evas_object_smart_callback_call(o, "fullscreen", NULL);
        return 1;
    }

    return _parent_api.key_down(o, info);
}

static void
_ejy_principal_screen_smart_add(Evas_Object *o)
{
    struct ejy_principal_screen_data *priv;

    priv = grn_widget_setup_private_data(o, sizeof(*priv));
    _parent_api.sc.add(o);

    priv->panel = NULL;
    priv->playlist = NULL;

    grn_edje_widget_group_set(o, "screen/principal");
}

static void
_ejy_principal_screen_smart_del(Evas_Object *o)
{
    EJY_PRINCIPAL_SCREEN_DATA_GET(o, priv);

    if (priv->panel)
        evas_object_del(priv->panel);
    if (priv->playlist)
        evas_object_del(priv->playlist);

    _parent_api.sc.del(o);
}

Evas_Object *
ejy_principal_screen_add(Evas *evas)
{
    static Evas_Smart *smart = NULL;

    if (!smart) {
        static struct grn_widget_api api = {
            {"Principal", EVAS_SMART_CLASS_VERSION}
        };

        grn_edje_widget_smart_set(&api);
        _parent_api = api;

        api.sc.add = _ejy_principal_screen_smart_add;
        api.sc.del = _ejy_principal_screen_smart_del;
        api.theme_changed = _ejy_principal_screen_theme_changed;
        api.key_down = _ejy_principal_screen_key_down;
        smart = evas_smart_class_new(&api.sc);
    }

    return evas_object_smart_add(evas, smart);
}

Evas_Bool
ejy_principal_screen_size_min_get(Evas_Object *o, int *w, int *h)
{
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN_VAL(o, priv, 0);

    edje_object_size_min_get(priv->base.edje, w, h);

    return 1;
}

Evas_Bool
ejy_principal_screen_size_max_get(Evas_Object *o, int *w, int *h)
{
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN_VAL(o, priv, 0);

    edje_object_size_max_get(priv->base.edje, w, h);

    return 1;
}

Evas_Bool
ejy_principal_screen_size_hint_get(Evas_Object *o, int *w, int *h)
{
    const char *size_hint;
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN_VAL(o, priv, 0);

    size_hint = edje_object_data_get(priv->base.edje, EDJE_DATA_SIZE_HINT);
    if (!size_hint)
        return 0;

    sscanf(size_hint, "%d %d", w, h);
    return 1;
}

Evas_Bool
ejy_principal_screen_borderless_get(Evas_Object *o)
{
    const char *borderless;
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN_VAL(o, priv, 0);

    borderless = edje_object_data_get(priv->base.edje, EDJE_DATA_BORDERLESS);
    if (borderless)
        if (strcmp(borderless, "1") == 0)
            return 1;

    return 0;
}

Evas_Bool
ejy_principal_screen_size_step_get(Evas_Object *o, int *w, int *h)
{
    const char *size_step;
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN_VAL(o, priv, 0);

    size_step = edje_object_data_get(priv->base.edje, EDJE_DATA_SIZE_STEP);
    if (!size_step)
        return 0;

    sscanf(size_step, "%d %d", w, h);
    return 1;
}

void
ejy_principal_screen_panel_set(Evas_Object *o, Evas_Object *panel)
{
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN(o, priv);

    if (priv->panel == panel)
        return;

    if (priv->panel) {
        edje_object_part_unswallow(priv->base.edje, priv->panel);
        evas_object_hide(priv->panel);
    }

    priv->panel = panel;
    edje_object_part_swallow(
        priv->base.edje, EDJE_PART_PANEL, priv->panel);
}

void
ejy_principal_screen_playlist_set(Evas_Object *o, Evas_Object *playlist)
{
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN(o, priv);

    if (priv->playlist == playlist)
        return;

    if (priv->playlist) {
        edje_object_part_unswallow(priv->base.edje, priv->playlist);
        evas_object_hide(priv->playlist);
    }

    priv->playlist = playlist;
    edje_object_part_swallow(
        priv->base.edje, EDJE_PART_PLAYLIST, priv->playlist);
}

void
ejy_principal_screen_change_view(Evas_Object *o, const char *emission)
{
    EJY_PRINCIPAL_SCREEN_DATA_GET_OR_RETURN(o, priv);

    edje_object_signal_emit(priv->base.edje, emission, "");
}
