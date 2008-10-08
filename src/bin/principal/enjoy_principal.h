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

#ifndef __PRINCIPAL_H__
#define __PRINCIPAL_H__ 1

#include <guarana.h>
#include <Evas.h>

struct ejy_cfg {
    struct {
        int fullscreen;
    } screen;
    struct {
        double volume;
    } player;
    struct {
        int sel;
        int play;
        int random;
        int repeat;
    } playlist;
};

struct ejy_principal_model {
    grn_model_t base;
    struct ejy_cfg *config;
};

grn_model_t *ejy_principal_model_new(void);

grn_controller_t *ejy_principal_controller_new(grn_model_t *model, grn_controller_t *parent);
void ejy_principal_controller_file_set(grn_controller_t *controller);
double ejy_principal_controller_player_length_get(grn_controller_t *controller);
double ejy_principal_controller_player_volume_get(grn_controller_t *controller);
void ejy_principal_controller_player_play(grn_controller_t *controller);
void ejy_principal_controller_player_pause(grn_controller_t *controller);
void ejy_principal_controller_player_stop(grn_controller_t *controller);
void ejy_principal_controller_player_previous(grn_controller_t *controller);
void ejy_principal_controller_player_next(grn_controller_t *controller);
void ejy_principal_controller_player_progress_set(grn_controller_t *controller, double position);
void ejy_principal_controller_player_volume_set(grn_controller_t *controller, double volume);
void ejy_principal_controller_change_view(grn_controller_t *controller, const char *emission);
void ejy_principal_controller_fullscreen_set(grn_controller_t *controller, int val);
void ejy_principal_controller_playlist_select(grn_controller_t *controller, int index);
void ejy_principal_controller_playlist_load(grn_controller_t *controller, int index);
void ejy_principal_controller_playlist_updated(grn_controller_t *controller);


Evas_Object *ejy_principal_screen_add(Evas *evas);
Evas_Bool ejy_principal_screen_size_min_get(Evas_Object *o, int *w, int *h);
Evas_Bool ejy_principal_screen_size_max_get(Evas_Object *o, int *w, int *h);
Evas_Bool ejy_principal_screen_size_hint_get(Evas_Object *o, int *w, int *h);
Evas_Bool ejy_principal_screen_size_step_get(Evas_Object *o, int *w, int *h);
Evas_Bool ejy_principal_screen_borderless_get(Evas_Object *o);
void ejy_principal_screen_panel_set(Evas_Object *o, Evas_Object *panel);
void ejy_principal_screen_playlist_set(Evas_Object *o, Evas_Object *playlist);
void ejy_principal_screen_change_view(Evas_Object *o, const char *emission);

#endif /* __PRINCIPAL_H__ */
