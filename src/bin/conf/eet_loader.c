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
#include "eet_loader.h"
#include <enjoy_principal.h>
#include <guarana_utils.h>
#include <Eet.h>
#include <Evas.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void
_ejy_eet_descriptor_class_set(Eet_Data_Descriptor_Class *eddc, const char *name, int size)
{
    memset(eddc, 0, sizeof(*eddc));
    eddc->version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
    eddc->name = name;
    eddc->size = size;
    eddc->func.str_alloc = (char *(*)(const char *))evas_stringshare_add;
    eddc->func.str_free = (void (*)(const char *))evas_stringshare_add;
    eddc->func.list_next = (void *(*)(void *))evas_list_next;
    eddc->func.list_append = (void *(*)(void *l, void *d))evas_list_append;
    eddc->func.list_data = (void *(*)(void *))evas_list_data;
    eddc->func.list_free = (void *(*)(void *))evas_list_free;
}

static Evas_Bool
_ejy_cfg_eet_descriptor_new(Eet_Data_Descriptor **edd_cfg)
{
    Eet_Data_Descriptor_Class eddc;

    _ejy_eet_descriptor_class_set(
        &eddc, "Enjoy_Config",
        sizeof(struct ejy_cfg));

    *edd_cfg = eet_data_descriptor2_new(&eddc);
    if (!edd_cfg)
        return 0;

    EET_DATA_DESCRIPTOR_ADD_BASIC(*edd_cfg, struct ejy_cfg,
        "screen.fullscreen", screen.fullscreen, EET_T_INT);

    EET_DATA_DESCRIPTOR_ADD_BASIC(*edd_cfg, struct ejy_cfg,
        "player.volume", player.volume, EET_T_DOUBLE);

    EET_DATA_DESCRIPTOR_ADD_BASIC(*edd_cfg, struct ejy_cfg,
        "playlist.sel", playlist.sel, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC(*edd_cfg, struct ejy_cfg,
        "playlist.play", playlist.play, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC(*edd_cfg, struct ejy_cfg,
        "playlist.random", playlist.random, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC(*edd_cfg, struct ejy_cfg,
        "playlist.repeat", playlist.repeat, EET_T_INT);

    return 1;
}

static Evas_Bool
_ejy_stringlist_eet_descriptor_new(Eet_Data_Descriptor **stringlist, Eet_Data_Descriptor **string)
{
    Eet_Data_Descriptor_Class eddc_stringlist, eddc_string;

    *stringlist = NULL;
    *string = NULL;

    _ejy_eet_descriptor_class_set(
        &eddc_string, "String_Descriptor",
        sizeof(struct ejy_stringitem));

    _ejy_eet_descriptor_class_set(
        &eddc_stringlist, "StringList_Descriptor",
        sizeof(struct ejy_stringlist));

    *string = eet_data_descriptor2_new(&eddc_string);
    if (!*string)
        return 0;

    *stringlist = eet_data_descriptor2_new(&eddc_stringlist);
    if (!*stringlist)
        return 0;

    EET_DATA_DESCRIPTOR_ADD_BASIC(
        *string, struct ejy_stringitem,
        "value", value, EET_T_STRING);

    EET_DATA_DESCRIPTOR_ADD_LIST(
        *stringlist, struct ejy_stringlist,
        "list", list, *string);

    return 1;
}

struct ejy_cfg *
ejy_cfg_eet_descriptor_load(Eet_File *ef, const char *key)
{
    int r;
    struct ejy_cfg *cfg;
    Eet_Data_Descriptor *edd_cfg;

    r = _ejy_cfg_eet_descriptor_new(&edd_cfg);
    if (!r)
        return NULL;

    cfg = eet_data_read(ef, edd_cfg, key);
    eet_data_descriptor_free(edd_cfg);

    return cfg;
}

Evas_Bool
ejy_cfg_eet_descriptor_save(Eet_File *ef, const char *key, const struct ejy_cfg *cfg)
{
    int r;

    Eet_Data_Descriptor *edd_cfg;

    r = _ejy_cfg_eet_descriptor_new(&edd_cfg);
    if (!r)
        return 0;

    r = eet_data_write(ef, edd_cfg, key, cfg, 0);
    eet_data_descriptor_free(edd_cfg);

    return 1;
}

/* stringlist functions */

struct ejy_stringlist *
ejy_stringlist_eet_descriptor_load(Eet_File *ef, const char *key)
{
    int r;
    struct ejy_stringlist *tuple;
    Eet_Data_Descriptor *edd_string, *edd_stringlist;

    r = _ejy_stringlist_eet_descriptor_new(&edd_stringlist, &edd_string);
    if (!r)
        return NULL;

    tuple = eet_data_read(ef, edd_stringlist, key);
    eet_data_descriptor_free(edd_stringlist);
    eet_data_descriptor_free(edd_string);

    return tuple;
}

Evas_Bool
ejy_stringlist_eet_descriptor_save(Eet_File *ef, const char *key, const struct ejy_stringlist *tuple)
{
    int r;

    Eet_Data_Descriptor *edd_string, *edd_stringlist;

    r = _ejy_stringlist_eet_descriptor_new(&edd_stringlist, &edd_string);
    if (!r)
        return r;

    r = eet_data_write(ef, edd_stringlist, key, tuple, 0);
    eet_data_descriptor_free(edd_stringlist);
    eet_data_descriptor_free(edd_string);
    return r;
}

struct ejy_stringlist *
ejy_stringlist_key_read(const char *eet_file, const char *key)
{
    Eet_File *ef;

    ef = eet_open(eet_file, EET_FILE_MODE_READ);
    if (!ef) {
        fprintf(stderr, "ERROR: file %s not found!\n", eet_file);
        return NULL;
    } else {
        struct ejy_stringlist *list;
        list = ejy_stringlist_eet_descriptor_load(ef, key);
        eet_close(ef);
        return list;
    }
}

struct ejy_stringlist *
ejy_stringlist_config_read(const char *key)
{
    struct ejy_stringlist *stringlist;
    const char *file_ro, *file_rw, *key_prefix;
    char *eet_key_result;
    int r;

    grn_manager_global_default_eet_info_get(&file_ro, &file_rw, &key_prefix);
    r = asprintf(&eet_key_result, "%s%s", key_prefix, key);
    if (r == -1)
        return 0;

    stringlist = ejy_stringlist_key_read(file_ro, eet_key_result);
    free(eet_key_result);

    return stringlist;
}

/* stringitem functions */

struct ejy_stringitem *
ejy_stringitem_eet_descriptor_load(Eet_File *ef, const char *key)
{
    int r;
    struct ejy_stringitem *tuple;
    Eet_Data_Descriptor *edd_stringitem, *edd_stringlist;

    r = _ejy_stringlist_eet_descriptor_new(&edd_stringlist, &edd_stringitem);
    if (!r)
        return NULL;

    tuple = eet_data_read(ef, edd_stringitem, key);
    eet_data_descriptor_free(edd_stringlist);
    eet_data_descriptor_free(edd_stringitem);

    return tuple;
}

Evas_Bool
ejy_stringitem_eet_descriptor_save(Eet_File *ef, const char *key, const struct ejy_stringitem *tuple)
{
    int r;

    Eet_Data_Descriptor *edd_stringitem, *edd_stringlist;

    r = _ejy_stringlist_eet_descriptor_new(&edd_stringlist, &edd_stringitem);
    if (!r)
        return r;

    r = eet_data_write(ef, edd_stringitem, key, tuple, 0);
    eet_data_descriptor_free(edd_stringlist);
    eet_data_descriptor_free(edd_stringitem);
    return r;
}

struct ejy_stringitem *
ejy_stringitem_key_read(const char *eet_file, const char *key)
{
    Eet_File *ef;

    ef = eet_open(eet_file, EET_FILE_MODE_READ);
    if (!ef) {
        fprintf(stderr, "ERROR: file %s not found!\n", eet_file);
        return NULL;
    } else {
        struct ejy_stringitem *item;
        item = ejy_stringitem_eet_descriptor_load(ef, key);
        eet_close(ef);
        return item;
    }
}

struct ejy_stringitem *
ejy_stringitem_config_read(const char *key)
{
    struct ejy_stringitem *stringitem;
    const char *file_ro, *file_rw, *key_prefix;
    char *eet_key_result;
    int r;

    grn_manager_global_default_eet_info_get(&file_ro, &file_rw, &key_prefix);
    r = asprintf(&eet_key_result, "%s%s", key_prefix, key);
    if (r == -1)
        return 0;

    stringitem = ejy_stringitem_key_read(file_ro, eet_key_result);
    free(eet_key_result);

    return stringitem;
}
