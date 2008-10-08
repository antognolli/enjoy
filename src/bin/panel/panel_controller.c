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

#include "enjoy_panel.h"
#include <enjoy_principal.h>
#include <enjoy_playlist.h>
#include <guarana.h>
#include <guarana_utils.h>
#include <Evas.h>
#include <Ecore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct ejy_panel_controller {
    grn_controller_t base;
    struct {
        struct ejy_playlist_model_child *model;
        double length;
        double elapsed;
    } current;
};

static const char EDJE_SIG_PLAY[] = "gui,action,play";
static const char EDJE_SIG_PAUSE[] = "gui,action,pause";
static const char EDJE_SIG_STOP[] = "gui,action,stop";
static const char EDJE_SIG_REWIND[] = "gui,action,rewind";
static const char EDJE_SIG_FORWARD[] = "gui,action,forward";
static const char EDJE_SIG_PREVIOUS[] = "gui,action,previous";
static const char EDJE_SIG_NEXT[] = "gui,action,next";
static const char EDJE_SIG_CHANGE_VIEW[] = "view,change,*";
static const char EDJE_ACTION_PROGRESS_SET[] = "gui,action,progress_set";
static const char EDJE_ACTION_VOLUME_SET[] = "gui,action,volume_set";
static const char ACTION_CHANGE_VIEW[] = "view,change";

static int
_ejy_panel_controller_suspend(grn_controller_t *controller)
{
    struct ejy_panel_controller *c;

    c = (struct ejy_panel_controller *)controller;

    if (c->current.model) {
        grn_model_unref(&c->current.model->base);
        c->current.model = NULL;
    }

    evas_object_del(controller->view);
    controller->view = NULL;

    return 1;
}

static int
_ejy_panel_controller_del(grn_controller_t *controller)
{
    if (controller->view)
        grn_controller_suspend(controller);

    return 1;
}

int
ejy_panel_controller_update_info(grn_controller_t *controller, double elapsed)
{
    struct ejy_panel_controller *c;
    Evas_Object *v;
    int int_elapsed, int_new;

    c = (struct ejy_panel_controller *)controller;
    v = c->base.view;


    int_elapsed = (int)c->current.elapsed;
    c->current.elapsed = elapsed;
    int_new = (int)elapsed;
    if (int_new == int_elapsed)
        return 0;

    ejy_panel_screen_progress_update(c->base.view, c->current.elapsed,
                                     c->current.length);

    return 1;
}

static void
_ejy_panel_controller_fill_info(grn_controller_t *controller, struct ejy_playlist_model_child *child)
{
    struct ejy_panel_controller *c;
    double volume;
    Evas_Object *v = controller->view;

    c = (struct ejy_panel_controller *)controller;
    c->current.length = ejy_principal_controller_player_length_get(
                            controller->parent);
    c->current.elapsed = -1;

    volume = ejy_principal_controller_player_volume_get(
                controller->parent);

    ejy_panel_screen_title_set(v, child->file.title);
    ejy_panel_screen_artist_set(v, child->file.artist);
    ejy_panel_screen_album_set(v, child->file.album);
    ejy_panel_screen_filename_set(v, child->file.path);
    ejy_panel_screen_volume_set(v, volume);

    ejy_panel_controller_update_info(controller, 0);
}

static void
_ejy_panel_controller_play_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *controller = data;

    ejy_principal_controller_player_play(controller->parent);
    ejy_panel_screen_play_set(controller->view, 1);
}

static void
_ejy_panel_controller_stop_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *c = data;

    ejy_principal_controller_player_stop(c->parent);
    ejy_panel_screen_play_set(c->view, 0);
}

static void
_ejy_panel_controller_pause_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *c = data;

    ejy_principal_controller_player_pause(c->parent);
    ejy_panel_screen_play_set(c->view, 0);
}

static void
_ejy_panel_controller_previous_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *c = data;

    ejy_principal_controller_player_previous(c->parent);
}

static void
_ejy_panel_controller_next_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *c = data;

    ejy_principal_controller_player_next(c->parent);
}

static void
_ejy_panel_controller_rewind_cb(void *data, Evas_Object *o, void *einfo)
{
    fputs("INFO: rewind not implemented yet...\n", stderr);
}

static void
_ejy_panel_controller_forward_cb(void *data, Evas_Object *o, void *einfo)
{
    fputs("INFO: fast forward not implemented yet...\n", stderr);
}

static void
_ejy_panel_controller_change_view_cb(void *data, Evas_Object *o, void *einfo)
{
    const char *emission = einfo;
    grn_controller_t *c = data;
    fprintf(stderr, "changing view: %s\n", emission);
    ejy_principal_controller_change_view(c->parent, emission);
}

static void
_ejy_panel_controller_progress_set(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *c = data;
    struct ejy_panel_controller *controller = data;
    double *value = (double *)einfo;

    ejy_principal_controller_player_progress_set(
        c->parent, *value * controller->current.length);
    return;
}

static void
_ejy_panel_controller_volume_set(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *c = data;
    double *value = (double *)einfo;

    ejy_principal_controller_player_volume_set(c->parent, *value);
    return;
}

static void
_ejy_panel_controller_connect_view(grn_controller_t *controller)
{
    Evas_Object *view = controller->view;
    const struct view_connections {
        const char *signal;
        void (*cb)(void *data, Evas_Object *o, void *einfo);
    } *itr, items[] = {
        {EDJE_SIG_PLAY, _ejy_panel_controller_play_cb},
        {EDJE_SIG_STOP, _ejy_panel_controller_stop_cb},
        {EDJE_SIG_PAUSE, _ejy_panel_controller_pause_cb},
        {EDJE_SIG_PREVIOUS, _ejy_panel_controller_previous_cb},
        {EDJE_SIG_NEXT, _ejy_panel_controller_next_cb},
        {EDJE_SIG_REWIND, _ejy_panel_controller_rewind_cb},
        {EDJE_SIG_FORWARD, _ejy_panel_controller_forward_cb},
        {ACTION_CHANGE_VIEW, _ejy_panel_controller_change_view_cb},
        {EDJE_ACTION_PROGRESS_SET, _ejy_panel_controller_progress_set},
        {EDJE_ACTION_VOLUME_SET, _ejy_panel_controller_volume_set},
        {NULL, NULL}
    };

    for (itr = items; itr->signal != NULL; itr++)
        evas_object_smart_callback_add(view, itr->signal, itr->cb, controller);
}

static Evas *
_ejy_panel_controller_get_evas(grn_controller_t *controller)
{
    Evas_Object *parent_view;

    if (controller->parent)
        parent_view = grn_controller_view_get(controller->parent);
    else
        parent_view = NULL;

    if (parent_view)
        return evas_object_evas_get(parent_view);
    else
        return grn_manager_global_default_data_get("evas");
}

void
ejy_panel_controller_file_set(grn_controller_t *controller, struct ejy_playlist_model_child *child)
{
    _ejy_panel_controller_fill_info(controller, child);
}

void
ejy_panel_controller_play_set(grn_controller_t *controller, Evas_Bool value)
{
    ejy_panel_screen_play_set(controller->view, value);
}

static int
_ejy_panel_controller_resume(grn_controller_t *controller)
{
    Evas *e;
    double volume;

    if (controller->view)
        return 1;

    e = _ejy_panel_controller_get_evas(controller);

    controller->view = ejy_panel_screen_add(e);
    if (!controller->view) {
        fputs("ERROR: could not create ejy_panel_screen.\n", stderr);
        return 0;
    }

    _ejy_panel_controller_connect_view(controller);

    volume = ejy_principal_controller_player_volume_get(controller->parent);
    ejy_panel_screen_volume_set(controller->view, volume);

    return 1;
}

grn_controller_t *
ejy_panel_controller_new(grn_model_t *model, grn_controller_t *parent)
{
    static struct grn_controller_api api = {
        "Controller/Panel",
        sizeof(struct ejy_panel_controller),
        NULL, NULL, NULL
    };
    grn_controller_t *controller;

    if (!api.del) {
        grn_controller_api_default_set(&api);
        api.del = _ejy_panel_controller_del;
        api.resume = _ejy_panel_controller_resume;
        api.suspend = _ejy_panel_controller_suspend;
    }

    controller = grn_controller_new(&api);
    if (!controller)
        return NULL;

    grn_controller_parent_set(controller, parent);
    grn_controller_model_set(controller, model);

    return controller;
}
