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

#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__ 1

#include <guarana.h>
#include <Evas.h>

struct ejy_playlist_model_child {
    grn_model_t base;
    struct {
        int id;
        const char *title;
        const char *path;
        const char *album;
        const char *artist;
        const char *genre;
        int trackno;
        int rating;
    } file;
};

struct ejy_playlist_model {
    grn_model_folder_t base;
    const char *query;
};

grn_model_t *ejy_playlist_model_new(const char *query);

grn_controller_t *ejy_playlist_controller_new(grn_model_t *model, grn_controller_t *parent);
void ejy_playlist_controller_selection_set(grn_controller_t *controller, int index, Evas_Bool value);
int ejy_playlist_controller_selection_get(grn_controller_t *controller);
int ejy_playlist_controller_selection_next(grn_controller_t *controller);
int ejy_playlist_controller_selection_previous(grn_controller_t *controller);
void ejy_playlist_controller_loaded_set(grn_controller_t *controller, int index, Evas_Bool value);
int ejy_playlist_controller_loaded_get(grn_controller_t *controller);
int ejy_playlist_controller_loaded_next(grn_controller_t *controller);
int ejy_playlist_controller_loaded_previous(grn_controller_t *controller);
struct ejy_playlist_model_child *ejy_playlist_controller_model_get_by_index(grn_controller_t *controller, int index);
void ejy_playlist_controller_change_view(grn_controller_t *controller, const char *emission);
int ejy_playlist_controller_model_count(grn_controller_t *controller);

Evas_Object *ejy_playlist_screen_list_add_to(Evas_Object *parent);
void ejy_playlist_screen_list_loaded_set(Evas_Object *o, int index, Evas_Bool value);
Evas_Object *ejy_playlist_screen_add(Evas *evas);
void ejy_playlist_screen_model_set(Evas_Object *o, grn_model_folder_t *model);
void ejy_playlist_screen_selection_set(Evas_Object *o, int index, Evas_Bool value);
int ejy_playlist_screen_selection_get(Evas_Object *o);
void ejy_playlist_screen_loaded_set(Evas_Object *o, int index, Evas_Bool value);
struct ejy_playlist_model_child *ejy_playlist_screen_model_get_by_index(Evas_Object *o, int index);
void ejy_playlist_screen_change_view(Evas_Object *o, const char *emission);

#endif /* __PLAYLIST_H__ */
