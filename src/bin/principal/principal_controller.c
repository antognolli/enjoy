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

#define _GNU_SOURCE
#include "enjoy_principal.h"
#include <eet_loader.h>
#include <enjoy_panel.h>
#include <enjoy_playlist.h>
#include <guarana.h>
#include <guarana_utils.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Emotion.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define EJY_PRINCIPAL_CONTROLLER_CONFIG_GET(ctrl, cfg)                      \
    struct ejy_cfg *cfg = ((struct ejy_principal_model *)(ctrl)->model)->config

struct ejy_principal_controller {
    grn_controller_t base;
    grn_controller_t *panel;
    grn_controller_t *playlist;
    struct ejy_playlist_model_child *current;
    Evas_Object *emotion;
    struct {
        Ecore_Timer *progress;
    } timer;
    int playlist_count;
};

static const char EDJE_SIG_NOFULLSCREEN[] = "view,change,nofullscreen";
static const char EDJE_SIG_FULLSCREEN[] = "view,change,fullscreen";

static Evas *
_ejy_principal_controller_get_evas(grn_controller_t *controller)
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

static int
_ejy_principal_controller_progress_update(void *data)
{
    struct ejy_principal_controller *c = data;
    int elapsed;

    elapsed = emotion_object_position_get(c->emotion);
    ejy_panel_controller_update_info(c->panel, elapsed);
    return 1;
}

static void
_ejy_principal_controller_progress_timer_start(struct ejy_principal_controller *c)
{
    if (c->timer.progress)
        ecore_timer_del(c->timer.progress);

    c->timer.progress = ecore_timer_add(
        1, _ejy_principal_controller_progress_update, c);
}

static void
_ejy_principal_controller_progress_timer_stop(struct ejy_principal_controller *c)
{
    if (c->timer.progress)
        ecore_timer_del(c->timer.progress);

    c->timer.progress = NULL;
}

static int
_ejy_principal_controller_suspend(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;

    evas_object_del(controller->view);
    controller->view = NULL;

    c = (struct ejy_principal_controller *)controller;

    _ejy_principal_controller_progress_timer_stop(c);

    if (c->panel) {
        grn_controller_suspend(c->panel);
        grn_controller_unref(c->panel);
        c->panel = NULL;
    }

    if (c->playlist) {
        grn_controller_suspend(c->playlist);
        grn_controller_unref(c->playlist);
        c->playlist = NULL;
    }

    if (c->emotion) {
        evas_object_del(c->emotion);
        c->emotion = NULL;
    }

    if (c->current)
        grn_model_unref(&c->current->base);

    grn_model_unref(controller->model);
    return grn_model_unload(controller->model);
}

static int
_ejy_principal_controller_del(grn_controller_t *controller)
{
    if (controller->view)
        grn_controller_suspend(controller);

    return 1;
}

static void
_ejy_principal_controller_show_panel(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    Evas_Object *v;

    c = (struct ejy_principal_controller *)controller;

    grn_controller_resume(c->panel);
    v = controller->view;

    ejy_principal_screen_panel_set(v, c->panel->view);
}

static int
_ejy_principal_controller_create_panel(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;

    c = (struct ejy_principal_controller *)controller;
    c->panel = ejy_panel_controller_new(NULL, controller);
    if (!c->panel)
        return 0;

    return 1;
}

static void
_ejy_principal_controller_show_playlist(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    Evas_Object *v;

    c = (struct ejy_principal_controller *)controller;

    grn_controller_resume(c->playlist);
    v = controller->view;

    ejy_principal_screen_playlist_set(v, c->playlist->view);
}

static int
_ejy_principal_controller_create_playlist(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    grn_model_t *model;

    model = ejy_playlist_model_new(
                "SELECT files.id, path, title, trackno, rating, playcnt, "
                "audio_albums.name, audio_genres.name, audio_artists.name "
                "FROM files, audios, audio_albums, audio_genres, audio_artists "
                "WHERE files.id = audios.id "
                "AND audio_albums.id = audios.album_id "
                "AND audio_genres.id = audios.genre_id "
                "AND audio_artists.id = audio_albums.artist_id");
    c = (struct ejy_principal_controller *)controller;
    c->playlist = ejy_playlist_controller_new(model, controller);
    if (!c->playlist)
        return 0;

    return 1;
}

static void
_ejy_principal_controller_screen_shutdown(void *data, Evas_Object *o, void *event_info)
{
    ecore_main_loop_quit();
}

void
ejy_principal_controller_file_set(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    struct ejy_playlist_model_child *child;
    int index;
    double volume;

    c = (struct ejy_principal_controller *)controller;
    index = ejy_playlist_controller_loaded_get(c->playlist);
    fprintf(stderr, "index = %d\n", index);
    child = ejy_playlist_controller_model_get_by_index(c->playlist, index);
    if (!child)
        return;

    if (c->current)
        grn_model_unref(&c->current->base);
    c->current = child;
    grn_model_ref(&child->base);

    emotion_object_file_set(c->emotion, child->file.path);
    volume = ejy_principal_controller_player_volume_get(controller);
    ejy_principal_controller_player_volume_set(controller, volume);
    ejy_panel_controller_file_set(c->panel, child);
}

static void
_ejy_principal_controller_screen_playlist_up(void *data, Evas_Object *o, void *event_info)
{
    struct ejy_principal_controller *c = data;
    ejy_playlist_controller_selection_previous(c->playlist);
}

static void
_ejy_principal_controller_screen_playlist_down(void *data, Evas_Object *o, void *event_info)
{
    struct ejy_principal_controller *c = data;
    ejy_playlist_controller_selection_next(c->playlist);
}

static void
_ejy_principal_controller_screen_playlist_home(void *data, Evas_Object *o, void *event_info)
{
    struct ejy_principal_controller *c = data;
    ejy_playlist_controller_selection_set(c->playlist, 0, 1);
}

static void
_ejy_principal_controller_screen_playlist_end(void *data, Evas_Object *o, void *event_info)
{
    struct ejy_principal_controller *c = data;
    ejy_playlist_controller_selection_set(c->playlist,
                                          c->playlist_count - 1, 1);
}

static void
_ejy_principal_controller_screen_playlist_play(void *data, Evas_Object *o, void *event_info)
{
    struct ejy_principal_controller *c = data;
    int index;

    index = ejy_playlist_controller_selection_get(c->playlist);
    if (index < 0)
        return;

    ejy_playlist_controller_loaded_set(c->playlist, index, 1);
    ejy_principal_controller_file_set(&c->base);
    emotion_object_play_set(c->emotion, 1);
    ejy_panel_controller_play_set(c->panel, 1);
    _ejy_principal_controller_progress_timer_start(c);
}

void
ejy_principal_controller_fullscreen_set(grn_controller_t *controller, int val)
{
    EJY_PRINCIPAL_CONTROLLER_CONFIG_GET(controller, cfg);
    Ecore_Evas *ee;
    Evas *evas = _ejy_principal_controller_get_evas(controller);
    ee = ecore_evas_ecore_evas_get(evas);

    cfg->screen.fullscreen = val;
    ecore_evas_fullscreen_set(ee, val);
}

static void
_ejy_principal_controller_screen_fullscreen(void *data, Evas_Object *o, void *event_info)
{
    grn_controller_t *controller = data;
    EJY_PRINCIPAL_CONTROLLER_CONFIG_GET(controller, cfg);

    ejy_principal_controller_fullscreen_set(
        controller, (cfg->screen.fullscreen + 1) % 2);
}

static void
_ejy_principal_controller_connect_view(grn_controller_t *controller)
{
    Evas_Object *view = controller->view;
    const struct view_connections {
        const char *signal;
        void (*cb)(void *data, Evas_Object *o, void *einfo);
    } *itr, items[] = {
        {"shutdown", _ejy_principal_controller_screen_shutdown},
        {"pl_up", _ejy_principal_controller_screen_playlist_up},
        {"pl_down", _ejy_principal_controller_screen_playlist_down},
        {"fullscreen", _ejy_principal_controller_screen_fullscreen},
        {"pl_play", _ejy_principal_controller_screen_playlist_play},
        {"pl_home", _ejy_principal_controller_screen_playlist_home},
        {"pl_end", _ejy_principal_controller_screen_playlist_end},
        {NULL, NULL}
    };

    for (itr = items; itr->signal != NULL; itr++)
        evas_object_smart_callback_add(view, itr->signal, itr->cb, controller);
}

void
ejy_principal_controller_playlist_select(grn_controller_t *controller, int index)
{
    struct ejy_principal_controller *c;

    c = (struct ejy_principal_controller *)controller;
    ejy_playlist_controller_selection_set(c->playlist, index, 1);
}

void
ejy_principal_controller_playlist_load(grn_controller_t *controller, int index)
{
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    ejy_playlist_controller_loaded_set(c->playlist, index, 1);
    ejy_principal_controller_file_set(controller);

    if (!c->current)
        return;
    emotion_object_play_set(c->emotion, 1);
    ejy_panel_controller_play_set(c->panel, 1);
    _ejy_principal_controller_progress_timer_start(c);
}

void
ejy_principal_controller_player_play(grn_controller_t *controller)
{
    int index;
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    if (!c->current) {
        index = ejy_playlist_controller_loaded_get(c->playlist);
        if (index < 0)
            ejy_playlist_controller_loaded_set(c->playlist, 0, 1);

        ejy_principal_controller_file_set(controller);
    }

    if (!c->current)
        return;

    emotion_object_play_set(c->emotion, 1);
    ejy_panel_controller_play_set(c->panel, 1);
    _ejy_principal_controller_progress_timer_start(c);
}

void
ejy_principal_controller_player_pause(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    if (!c->current)
        return;

    emotion_object_play_set(c->emotion, 0);
    ejy_panel_controller_play_set(c->panel, 0);
    _ejy_principal_controller_progress_timer_stop(c);
}

void
ejy_principal_controller_player_stop(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    if (!c->current)
        return;

    ejy_principal_controller_player_pause(controller);
    emotion_object_position_set(c->emotion, 0);
}

void
ejy_principal_controller_player_previous(grn_controller_t *controller)
{
    int index;
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    index = ejy_playlist_controller_loaded_previous(c->playlist);
    fprintf(stderr, "index = %d\n", index);
    if (index >= 0)
        ejy_principal_controller_file_set(controller);
}

void
ejy_principal_controller_player_next(grn_controller_t *controller)
{
    int index;
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    index = ejy_playlist_controller_loaded_next(c->playlist);
    if (index >= 0)
        ejy_principal_controller_file_set(controller);
}

void
ejy_principal_controller_player_progress_set(grn_controller_t *controller, double position)
{
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    if (!c->current)
        return;

    emotion_object_position_set(c->emotion, position);
}

void
ejy_principal_controller_player_volume_set(grn_controller_t *controller, double volume)
{
    struct ejy_principal_controller *c;
    struct ejy_principal_model *m;
    c = (struct ejy_principal_controller *)controller;
    m = (struct ejy_principal_model *)controller->model;

    m->config->player.volume = volume;
    if (!c->current)
        return;

    emotion_object_audio_volume_set(c->emotion, volume);
}

double
ejy_principal_controller_player_length_get(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    if (!c->current)
        return 0;

    return emotion_object_play_length_get(c->emotion);
}

double
ejy_principal_controller_player_volume_get(grn_controller_t *controller)
{
    struct ejy_principal_model *m;
    m = (struct ejy_principal_model *)controller->model;

    return m->config->player.volume;
}

void
ejy_principal_controller_change_view(grn_controller_t *controller, const char *emission)
{
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    if (!controller)
        return;

    ejy_principal_screen_change_view(controller->view, emission);
    ejy_playlist_controller_change_view(c->playlist, emission);
    if (strcmp(emission, EDJE_SIG_FULLSCREEN) == 0)
        ejy_principal_controller_fullscreen_set(controller, 1);
    else if (strcmp(emission, EDJE_SIG_NOFULLSCREEN) == 0)
        ejy_principal_controller_fullscreen_set(controller, 0);
}

static void
_ejy_principal_controller_decode_stop_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *controller = data;
    struct ejy_principal_controller *c = data;
    int index;

    index = ejy_playlist_controller_loaded_next(c->playlist);
    if (index < 0)
        return;

    ejy_principal_controller_file_set(controller);
    if (c->current)
        emotion_object_play_set(c->emotion, 1);
}

static void
_ejy_principal_controller_connect_emotion(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    const struct emotion_connections {
        const char *signal;
        void (*cb)(void *data, Evas_Object *, void *einfo);
    } *itr, items[] = {
        {"decode_stop", _ejy_principal_controller_decode_stop_cb},
        {NULL, NULL}
    };

    c = (struct ejy_principal_controller *)controller;

    for (itr = items; itr->signal != NULL; itr++)
        evas_object_smart_callback_add(c->emotion, itr->signal,
                                       itr->cb, controller);
}

static int
_ejy_principal_controller_emotion_init(struct ejy_principal_controller *c)
{
    struct ejy_stringitem *stringitem;

    stringitem = ejy_stringitem_config_read("/emotion/plugin");
    if (!stringitem)
        return 0;

    fprintf(stderr, "INFO: emotion plugin:\n");
    fprintf(stderr, "%s\n", stringitem->value);

    emotion_object_init(c->emotion, stringitem->value);
    emotion_object_video_mute_set(c->emotion, 1);

    evas_stringshare_del(stringitem->value);
    free(stringitem);

    return 1;
}

static int
_ejy_principal_controller_resume(grn_controller_t *controller)
{
    Evas *e;
    struct ejy_principal_controller *c;

    if (controller->view)
        return 1;

    e = _ejy_principal_controller_get_evas(controller);
    controller->view = ejy_principal_screen_add(e);
    if (!controller->view) {
        fputs("ERROR: could not create ejy_principal_screen.\n", stderr);
        return 0;
    }

    c = (struct ejy_principal_controller *)controller;
    c->emotion = emotion_object_add(e);
    if (!c->emotion) {
        fputs("ERROR: could not create emotion object. \n", stderr);
        return 0;
    }

    if (!_ejy_principal_controller_emotion_init(c)) {
        fputs("ERROR: could not initialize emotion.\n", stderr);
        return 0;
    }

    _ejy_principal_controller_connect_emotion(controller);
    c->current = NULL;
    c->timer.progress = NULL;
    c->playlist_count = 0;

    if (!grn_model_load(controller->model)) {
        fputs("ERROR: could not load model.\n", stderr);
        return 0;
    }

    _ejy_principal_controller_connect_view(controller);

    if (!_ejy_principal_controller_create_panel(controller)) {
        fputs("ERROR: could not create panel controller.\n", stderr);
        return 0;
    }

    if (!_ejy_principal_controller_create_playlist(controller)) {
        fputs("ERROR: could not create playlist controller.\n", stderr);
        return 0;
    }

    _ejy_principal_controller_show_panel(controller);
    _ejy_principal_controller_show_playlist(controller);

    return 1;
}

grn_controller_t *
ejy_principal_controller_new(grn_model_t *model, grn_controller_t *parent)
{
    static struct grn_controller_api api = {
        "Controller/Principal",
        sizeof(struct ejy_principal_controller),
        NULL, NULL, NULL
    };
    grn_controller_t *controller;

    if (!api.del) {
        grn_controller_api_default_set(&api);
        api.del = _ejy_principal_controller_del;
        api.resume = _ejy_principal_controller_resume;
        api.suspend = _ejy_principal_controller_suspend;
    }

    controller = grn_controller_new(&api);
    if (!controller)
        return NULL;

    grn_controller_parent_set(controller, parent);
    grn_controller_model_set(controller, model);

    return controller;
}

void
ejy_principal_controller_playlist_updated(grn_controller_t *controller)
{
    struct ejy_principal_controller *c;
    c = (struct ejy_principal_controller *)controller;

    c->playlist_count = grn_model_folder_count(
        (grn_model_folder_t *)c->playlist->model);
    ejy_playlist_controller_loaded_set(c->playlist, 0, 1);
    ejy_principal_controller_file_set(controller);
}
