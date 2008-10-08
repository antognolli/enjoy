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
#include "enjoy_playlist.h"
#include <eet_loader.h>
#include <guarana.h>
#include <guarana_utils.h>
#include <lightmediascanner.h>
#include <Ecore.h>
#include <Evas.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

enum {
    ENJOY_SCANNER_END
};

struct ejy_playlist_controller {
    grn_controller_t base;
    Ecore_Timer *timer;
    struct {
        lms_t *lms;
        pthread_t thread;
        Evas_Bool thread_running;
        Evas_Bool scanning;
        Evas_Bool proceed;
        int pipe[2];
        Ecore_Fd_Handler *fd_handler;
    } scanner;
    struct {
        int loaded;
        int sel;
        int count;
    } list;
};

void
_ejy_playlist_controller_scan_stop(struct ejy_playlist_controller *c)
{
    c->scanner.proceed = 0;
    while (c->scanner.scanning) {
        lms_stop_processing(c->scanner.lms);
        sched_yield();
    }

    if (c->scanner.thread_running)
        pthread_join(c->scanner.thread, NULL);

    if (c->scanner.fd_handler) {
        ecore_main_fd_handler_del(c->scanner.fd_handler);
        c->scanner.fd_handler = NULL;
    }
}

static int
_ejy_playlist_controller_suspend(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;
    c = (struct ejy_playlist_controller *)controller;

    evas_object_del(controller->view);
    controller->view = NULL;

    if (c->timer) {
        ecore_timer_del(c->timer);
        c->timer = NULL;
    }

    _ejy_playlist_controller_scan_stop(c);

    grn_model_unref(controller->model);

    return grn_model_unload(controller->model);
}

static int
_ejy_playlist_controller_del(grn_controller_t *controller)
{
    if (controller->view)
        grn_controller_suspend(controller);

    return 1;
}

static void
_ejy_playlist_controller_model_updated(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;
    c = (struct ejy_playlist_controller *)controller;

    c->list.count = grn_model_folder_count((grn_model_folder_t *)c->base.model);
    ejy_principal_controller_playlist_updated(c->base.parent);
}

static Evas *
_ejy_playlist_controller_get_evas(grn_controller_t *controller)
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
_ejy_playlist_controller_scan_fd_handler(void *data, Ecore_Fd_Handler *fd_handler)
{
    struct ejy_playlist_controller *c = data;
    char buf;
    ssize_t count;

    if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR)) {
        fputs("ERROR: playlist: fd error, remove fd handler.\n", stderr);
        c->scanner.fd_handler = NULL;
        return 0;
    }

    count = read(c->scanner.pipe[0], &buf, 1);
    if (count && buf == ENJOY_SCANNER_END) {
        pthread_join(c->scanner.thread, NULL);
        grn_model_unload(c->base.model);
        if (grn_model_load(c->base.model)) {
            ejy_playlist_screen_model_set(
                c->base.view, (grn_model_folder_t *)c->base.model);
            _ejy_playlist_controller_model_updated(&c->base);
        }
        ecore_main_fd_handler_del(fd_handler);
        c->scanner.fd_handler = NULL;
        c->scanner.thread_running = 0;
    }


    return 1;
}

static Evas_Bool
_ejy_playlist_controller_parser_list_get(struct ejy_playlist_controller *c)
{
    struct ejy_stringlist *stringlist;
    Evas_List *l;
    Evas_Bool found_parser = 0;

    stringlist = ejy_stringlist_config_read("/lms/parsers");
    if (!stringlist)
        return 0;

    l = stringlist->list;
    while (l) {
        struct ejy_stringitem *item = l->data;
        if (lms_parser_find_and_add(c->scanner.lms, item->value))
            found_parser = 1;
        evas_stringshare_del(item->value);
        l = evas_list_remove_list(l, l);
    }

    return found_parser;
}

static void
_ejy_playlist_controller_charset_get(struct ejy_playlist_controller *c)
{
    struct ejy_stringlist *stringlist;
    Evas_List *l;

    stringlist = ejy_stringlist_config_read("/lms/charsets");
    if (!stringlist)
        return;

    l = stringlist->list;
    while (l) {
        struct ejy_stringitem *item = l->data;
        lms_charset_add(c->scanner.lms, item->value);
        evas_stringshare_del(item->value);
        l = evas_list_remove_list(l, l);
    }
}

static void *
_ejy_playlist_controller_media_scan(void *data)
{
    char *db_path, *scan_path;
    lms_t *lms;
    struct ejy_playlist_controller *c = data;
    char status_end = ENJOY_SCANNER_END;

    db_path = grn_manager_global_default_data_get("dbpath");
    scan_path = grn_manager_global_default_data_get("scanpath");

    if (!db_path) {
        fputs("ERROR: can't access enjoy media database.\n", stderr);
        return NULL;
    }

    if (!scan_path) {
        fputs("INFO: no path to scan.\n", stderr);
        return NULL;
    }

    fprintf(stderr, "scanning...\n");
    lms = lms_new(db_path);
    c->scanner.lms = lms;
    if (!lms) {
        fprintf(stderr,
                "ERROR: could not create light media scanner for DB \"%s\".\n",
                db_path);
        return NULL;
    }

    if (!_ejy_playlist_controller_parser_list_get(c)) {
        fprintf(stderr, "ERROR: could not find any parser.\n");
        lms_free(lms);
        return NULL;
    }

    _ejy_playlist_controller_charset_get(c);

    c->scanner.scanning = 1;
    if (c->scanner.proceed && lms_check(lms, scan_path) != 0) {
        fprintf(stderr, "ERROR: checking \"%s\".\n", scan_path);
        lms_free(lms);
        return NULL;
    }

    if (c->scanner.proceed && lms_process(lms, scan_path) != 0) {
        fprintf(stderr, "ERROR: processing \"%s\".\n", scan_path);
        lms_free(lms);
        return NULL;
    }
    c->scanner.scanning = 0;

    if (lms_free(lms) != 0) {
        fprintf(stderr, "ERROR: could not close light media scanner.\n");
        return NULL;
    }

    fputs("scanned!\n", stderr);

    write(c->scanner.pipe[1], &status_end, 1);

    return NULL;
}

static int
_ejy_playlist_controller_scan_start(void *data)
{
    int r;
    struct ejy_playlist_controller *c = data;

    if (pipe(c->scanner.pipe) == -1) {
        fputs("ERROR: could not create pipe.\n", stderr);
        return 0;
    }

    c->scanner.fd_handler = ecore_main_fd_handler_add(
        c->scanner.pipe[0], ECORE_FD_READ | ECORE_FD_ERROR,
        _ejy_playlist_controller_scan_fd_handler, c, NULL, NULL);
    if (!c->scanner.fd_handler) {
        fputs("ERROR: could not create fd handler for scan.\n", stderr);
        return 0;
    }

    r = pthread_create(&c->scanner.thread, NULL,
                       _ejy_playlist_controller_media_scan, data);
    if (r) {
        fputs("ERROR: could not create scan thread.\n", stderr);
        return 0;
    }
    c->scanner.thread_running = 1;

    return 1;
}

static int
_ejy_playlist_controller_model_load(void *data)
{
    grn_controller_t *controller = data;
    struct ejy_playlist_controller *c = data;

    if (grn_model_load(controller->model)) {
        ejy_playlist_screen_model_set(controller->view,
                                      (grn_model_folder_t *)controller->model);
        _ejy_playlist_controller_model_updated(controller);
    }
    else
        fputs("ERROR: could not load playlist model.\n", stderr);

    _ejy_playlist_controller_scan_start(data);

    if (c->timer) {
        ecore_timer_del(c->timer);
        c->timer = NULL;
    }

    return 0;
}

static int
_ejy_playlist_controller_list_index(struct ejy_playlist_controller *c, int index, int repeat)
{
    if (!c->list.count)
        return -1;

    if (repeat) {
        if (index < 0)
            index = c->list.count + index;
        else
            index = index % c->list.count;
    } else {
        if (index < 0)
            index = -1;
        else if (index >= c->list.count)
            index = -1;
    }

    return index;
}

void
ejy_playlist_controller_selection_set(grn_controller_t *controller, int index, Evas_Bool value)
{
    struct ejy_playlist_controller *c;

    c = (struct ejy_playlist_controller *)controller;
    c->list.sel = _ejy_playlist_controller_list_index(c, index, 0);
    ejy_playlist_screen_selection_set(controller->view, c->list.sel, value);
}

int
ejy_playlist_controller_selection_get(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;

    c = (struct ejy_playlist_controller *)controller;
    return c->list.sel;
}

int
ejy_playlist_controller_selection_next(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;
    int index;

    c = (struct ejy_playlist_controller *)controller;
    index = _ejy_playlist_controller_list_index(c, c->list.sel + 1, 0);
    if (index >= 0)
        ejy_playlist_controller_selection_set(controller, index, 1);

    return index;
}

int
ejy_playlist_controller_selection_previous(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;
    int index;

    c = (struct ejy_playlist_controller *)controller;
    index = _ejy_playlist_controller_list_index(c, c->list.sel - 1, 0);
    if (index >= 0)
        ejy_playlist_controller_selection_set(controller, index, 1);

    return index;
}

void
ejy_playlist_controller_loaded_set(grn_controller_t *controller, int index, Evas_Bool value)
{
    struct ejy_playlist_controller *c;

    c = (struct ejy_playlist_controller *)controller;
    c->list.loaded = _ejy_playlist_controller_list_index(c, index, 0);
    ejy_playlist_screen_loaded_set(controller->view, c->list.loaded, value);
}

int
ejy_playlist_controller_loaded_get(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;

    c = (struct ejy_playlist_controller *)controller;
    return c->list.loaded;
}

int
ejy_playlist_controller_loaded_next(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;
    int index;

    c = (struct ejy_playlist_controller *)controller;
    index = _ejy_playlist_controller_list_index(c, c->list.loaded + 1, 0);
    if (index >= 0)
        ejy_playlist_controller_loaded_set(controller, index, 1);

    return index;
}

int
ejy_playlist_controller_loaded_previous(grn_controller_t *controller)
{
    struct ejy_playlist_controller *c;
    int index;

    c = (struct ejy_playlist_controller *)controller;
    index = _ejy_playlist_controller_list_index(c, c->list.loaded - 1, 0);
    if (index >= 0)
        ejy_playlist_controller_loaded_set(controller, index, 1);

    return index;
}

struct ejy_playlist_model_child *
ejy_playlist_controller_model_get_by_index(grn_controller_t *controller, int index)
{
    return ejy_playlist_screen_model_get_by_index(controller->view, index);
}

void
ejy_playlist_controller_change_view(grn_controller_t *controller, const char *emission)
{
    ejy_playlist_screen_change_view(controller->view, emission);
}

static void
_ejy_playlist_controller_selected_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *controller = data;
    int *index = einfo;

    ejy_principal_controller_playlist_select(controller->parent, *index);
}

static void
_ejy_playlist_controller_double_clicked_cb(void *data, Evas_Object *o, void *einfo)
{
    grn_controller_t *controller = data;
    int *index = einfo;

    ejy_principal_controller_playlist_load(controller->parent, *index);
}

static void
_ejy_playlist_controller_connect_view(grn_controller_t *controller)
{
    Evas_Object *view = controller->view;
    const struct view_connections {
        const char *signal;
        void (*cb)(void *data, Evas_Object *o, void *einfo);
    } *itr, items[] = {
        {"selected", _ejy_playlist_controller_selected_cb},
        {"double_clicked", _ejy_playlist_controller_double_clicked_cb},
        {NULL, NULL}
    };

    for (itr = items; itr->signal != NULL; itr++)
        evas_object_smart_callback_add(view, itr->signal, itr->cb, controller);
}

static int
_ejy_playlist_controller_resume(grn_controller_t *controller)
{
    Evas *e;
    struct ejy_playlist_controller *c;

    if (controller->view)
        return 1;

    e = _ejy_playlist_controller_get_evas(controller);

    controller->view = ejy_playlist_screen_add(e);
    if (!controller->view) {
        fputs("ERROR: could not create ejy_playlist_screen.\n", stderr);
        return 0;
    }

    c = (struct ejy_playlist_controller *)controller;
    c->scanner.scanning = 0;
    c->scanner.proceed = 1;
    c->scanner.lms = NULL;
    c->scanner.fd_handler = NULL;
    c->scanner.thread_running = 0;
    c->timer = ecore_timer_add(0.1,
                               _ejy_playlist_controller_model_load, c);
    c->list.sel = -1;
    c->list.loaded = -1;
    c->list.count = 0;

    _ejy_playlist_controller_connect_view(controller);

    return 1;
}

grn_controller_t *
ejy_playlist_controller_new(grn_model_t *model, grn_controller_t *parent)
{
    static struct grn_controller_api api = {
        "Controller/Playlist",
        sizeof(struct ejy_playlist_controller),
        NULL, NULL, NULL
    };
    grn_controller_t *controller;

    if (!api.del) {
        grn_controller_api_default_set(&api);
        api.del = _ejy_playlist_controller_del;
        api.resume = _ejy_playlist_controller_resume;
        api.suspend = _ejy_playlist_controller_suspend;
    }

    controller = grn_controller_new(&api);
    if (!controller)
        return NULL;

    grn_controller_parent_set(controller, parent);
    grn_controller_model_set(controller, model);

    return controller;
}

int
ejy_playlist_controller_model_count(grn_controller_t *controller)
{
    return grn_model_folder_count((grn_model_folder_t *)controller->model);
}
