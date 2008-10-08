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

#include "enjoy_playlist.h"
#include <guarana_utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static int
_ejy_playlist_model_child_del(grn_model_t *model)
{
    struct ejy_playlist_model_child *m;
    m = (struct ejy_playlist_model_child *)model;

    evas_stringshare_del(m->file.title);
    evas_stringshare_del(m->file.path);
    evas_stringshare_del(m->file.artist);
    evas_stringshare_del(m->file.album);
    evas_stringshare_del(m->file.genre);
    return 1;
}

static struct ejy_playlist_model_child *
_ejy_playlist_model_child_new(void)
{
    static struct grn_model_api api = {
        "Model/Playlist/Child",
        sizeof(struct ejy_playlist_model_child),
        {0, NULL}, NULL
    };

    if (!api.del) {
        grn_model_api_evas_set(&api);
        api.del = _ejy_playlist_model_child_del;
    }

    return (struct ejy_playlist_model_child *)grn_model_new(&api);
}

static int
_ejy_playlist_model_query_callback(void *data, int n_cols, char **values, char **col_names)
{
    struct ejy_playlist_model_child *child;
    struct ejy_playlist_model *m = data;

    child = _ejy_playlist_model_child_new();
    if (!child) {
        fputs("ERROR: _ejy_playlist_model_child_new: malloc", stderr);
        return 0;
    }
    child->file.id = atoi(values[0]);
    child->file.path = evas_stringshare_add(values[1]);
    child->file.title = evas_stringshare_add(values[2]);
    child->file.trackno = atoi(values[3]);
    child->file.rating = atoi(values[4]);
    child->file.album = evas_stringshare_add(values[6]);
    child->file.genre = evas_stringshare_add(values[7]);
    child->file.artist = evas_stringshare_add(values[8]);

    grn_model_folder_append(&m->base, &child->base);
    grn_model_unref(&child->base);

    return 0;
}

static int
_ejy_playlist_model_load(grn_model_t *model)
{
    grn_model_folder_t *folder;
    struct ejy_playlist_model *m;
    sqlite3 *db;

    folder = (grn_model_folder_t *)model;

    m = (struct ejy_playlist_model *)model;
    db = grn_manager_global_default_data_get("db");
    if (!db) {
        fputs("can't load music database.\n", stderr);
        return 0;
    }

    fprintf(stderr, "Executing query:\n%s\n", m->query);
    sqlite3_exec(db, m->query, _ejy_playlist_model_query_callback, m, NULL);

    return 1;
}

static int
_ejy_playlist_model_del(grn_model_t *model)
{
    struct ejy_playlist_model *m;

    m = (struct ejy_playlist_model *)model;
    evas_stringshare_del(m->query);

    return 1;
}

static int
_ejy_playlist_model_unload(grn_model_t *model)
{
    grn_model_folder_t *folder = (grn_model_folder_t *)model;

    grn_model_folder_clear(folder);
    return 1;
}

grn_model_t *
ejy_playlist_model_new(const char *query)
{
    static struct grn_model_folder_api api = {
        {"Model/Playlist",
         sizeof(struct ejy_playlist_model),
         {0, NULL}, NULL}
    };
    grn_model_t *model;
    struct ejy_playlist_model *m;

    if (!api.model.del) {
        grn_model_api_evas_set(&api.model);
        grn_model_folder_api_evas_list_set(&api);

        api.model.load = _ejy_playlist_model_load;
        api.model.unload = _ejy_playlist_model_unload;
        api.model.del = _ejy_playlist_model_del;
    }

    model = grn_model_new(&api.model);
    grn_model_name_set(model, "Playlist");

    m = (struct ejy_playlist_model *)model;
    m->query = evas_stringshare_add(query);

    return model;
}
