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
#include <guarana.h>
#include <guarana_utils.h>

static int
_ejy_principal_model_load_int(Eet_File *ef, const char *eet_key_prefix, struct ejy_principal_model *m)
{
    char *eet_key;
    struct ejy_cfg *cfg;
    int r;

    r = asprintf(&eet_key, "%s/config", eet_key_prefix);
    if (r == -1)
        return 0;
    cfg = ejy_cfg_eet_descriptor_load(ef, eet_key);
    free(eet_key);
    if (cfg) {
        m->config = cfg;
        return 1;
    } else
        return 0;
}

static int
_ejy_principal_model_unload_int(Eet_File *ef, const char *eet_key_prefix, struct ejy_principal_model *m)
{
    char *eet_key;
    int r;

    r = asprintf(&eet_key, "%s/config", eet_key_prefix);
    if (r == -1)
        return 0;

    r = ejy_cfg_eet_descriptor_save(ef, eet_key, m->config);
    free(eet_key);

    return r;
}

static int
_ejy_principal_model_unload(grn_model_t *model)
{
    struct ejy_principal_model *m;
    grn_manager_t *manager;
    const char *file_rw, *eet_key_prefix;
    Eet_File *ef;
    int r;

    m = (struct ejy_principal_model *)model;
    manager = grn_manager_global_default_get();

    grn_manager_eet_info_get(manager, NULL, &file_rw, &eet_key_prefix);

    if (!file_rw || !eet_key_prefix)
        r = 0;

    ef = eet_open(file_rw, EET_FILE_MODE_READ_WRITE);
    if (!ef)
        ef = eet_open(file_rw, EET_FILE_MODE_WRITE);
    if (!ef)
        r = 0;
    else {
        r = _ejy_principal_model_unload_int(ef, eet_key_prefix, m);
        eet_close(ef);
    }

    free(m->config);
    return r;
}

static int
_ejy_principal_model_load(grn_model_t *model)
{
    struct ejy_principal_model *m;
    const char *file_rw, *eet_key_prefix;
    Eet_File *ef;
    int r;

    m = (struct ejy_principal_model *)model;

    grn_manager_global_default_eet_info_get(NULL, &file_rw, &eet_key_prefix);
    if (!file_rw || !eet_key_prefix)
        return 0;

    ef = eet_open(file_rw, EET_FILE_MODE_READ);
    if (ef) {
        r = _ejy_principal_model_load_int(ef, eet_key_prefix, m);
        eet_close(ef);
    } else
        r = 0;

    if (!r) {
        m->config = (struct ejy_cfg *)malloc(sizeof(struct ejy_cfg));
        m->config->player.volume = 0.5;
        m->config->screen.fullscreen = 0;
        m->config->playlist.sel = -1;
        m->config->playlist.play = -1;
        m->config->playlist.random = 0;
        m->config->playlist.repeat = 0;
    }

    return 1;
}

grn_model_t *
ejy_principal_model_new(void)
{
    grn_model_t *model;

    static struct grn_model_api api = {
        "Model/Principal",
        sizeof(struct ejy_principal_model),
        {0, NULL}, NULL
    };

    if (!api.del) {
        grn_model_api_default_set(&api);
        api.load = _ejy_principal_model_load;
        api.unload = _ejy_principal_model_unload;
    }

    model = grn_model_new(&api);
    grn_model_name_set(model, "Principal");

    return model;
}
