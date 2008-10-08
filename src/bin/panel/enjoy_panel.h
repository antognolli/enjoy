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

#ifndef __PANEL_H__
#define __PANEL_H__ 1

#include <enjoy_playlist.h>
#include <guarana.h>
#include <Evas.h>

grn_controller_t *ejy_panel_controller_new(grn_model_t *model, grn_controller_t *parent);
void ejy_panel_controller_file_set(grn_controller_t *controller, struct ejy_playlist_model_child *child);
int ejy_panel_controller_update_info(grn_controller_t *controller, double elapsed);
void ejy_panel_controller_play_set(grn_controller_t *controller, Evas_Bool value);

Evas_Object *ejy_panel_screen_add(Evas *evas);
void ejy_panel_screen_title_set(Evas_Object *o, const char *title);
void ejy_panel_screen_artist_set(Evas_Object *o, const char *artist);
void ejy_panel_screen_album_set(Evas_Object *o, const char *album);
void ejy_panel_screen_filename_set(Evas_Object *o, const char *filename);
void ejy_panel_screen_time_total_set(Evas_Object *o, const char *time_total);
void ejy_panel_screen_time_elapsed_set(Evas_Object *o, const char *time_elapsed);
void ejy_panel_screen_time_remain_set(Evas_Object *o, const char *time_remain);
void ejy_panel_screen_play_set(Evas_Object *o, Evas_Bool v);
void ejy_panel_screen_progress_update(Evas_Object *o, double progress, double total);
void ejy_panel_screen_volume_set(Evas_Object *o, double volume);

#endif /* __PANEL_H__ */
