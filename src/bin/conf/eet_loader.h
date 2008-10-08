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

#ifndef __EET_LOADER_H__
#define __EET_LOADER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <enjoy_principal.h>
#include <Evas.h>
#include <Eet.h>

/* Global config and status */


struct ejy_cfg *ejy_cfg_eet_descriptor_load(Eet_File *ef, const char *key);
Evas_Bool ejy_cfg_eet_descriptor_save(Eet_File *ef, const char *key, const struct ejy_cfg *cfg);

/* Global config and status - end
 * everything else will be removed. */

struct ejy_stringitem {
    const char *value;
};

struct ejy_stringlist {
    Evas_List *list;
};

struct ejy_stringlist *ejy_stringlist_eet_descriptor_load(Eet_File *ef, const char *key);
Evas_Bool ejy_stringlist_eet_descriptor_save(Eet_File *ef, const char *key, const struct ejy_stringlist *tuple);
struct ejy_stringlist *ejy_stringlist_key_read(const char *eet_file, const char *key);
struct ejy_stringlist *ejy_stringlist_config_read(const char *key);

struct ejy_stringitem *ejy_stringitem_eet_descriptor_load(Eet_File *ef, const char *key);
Evas_Bool ejy_stringitem_eet_descriptor_save(Eet_File *ef, const char *key, const struct ejy_stringitem *tuple);
struct ejy_stringitem *ejy_stringitem_key_read(const char *eet_file, const char *key);
struct ejy_stringitem *ejy_stringitem_config_read(const char *key);

#endif
