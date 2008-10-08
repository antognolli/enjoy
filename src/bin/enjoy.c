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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <enjoy_principal.h>
#include <guarana.h>
#include <guarana_widgets.h>
#include <guarana_utils.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Edje.h>
#include <Ecore_Evas.h>
#include <Ecore.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>
#include <fcntl.h>
#include <locale.h>
#include <sqlite3.h>

struct app {
    grn_manager_t *manager;
    grn_controller_t *controller;
    Ecore_Evas *ee;
    Evas *evas;
    int verbosity;
};

static const char short_options[] = "S:u:k:e:t:s:v:f:b:h";

static const struct option long_options[] = {
    {"system-conf", 1, NULL, 'S'},
    {"user-conf", 1, NULL, 'u'},
    {"key-prefix", 1, NULL, 'k'},
    {"engine", 1, NULL, 'e'},
    {"theme-file", 1, NULL, 't'},
    {"size", 1, NULL, 's'},
    {"verbose", 1, NULL, 'v'},
    {"scanpath", 1, NULL, 'f'},
    {"db", 1, NULL, 'b'},
    {"help", 0, NULL, 'h'},
    {NULL, 0, 0, 0}
};

static const char *help_texts[] = {
    "where system configuration (read-only) is",
    "where user configuration (read-write) is",
    "what EET key prefix to use",
    "engine to use (software_x11, directfb, ...)",
    "path to theme file to use",
    "window size",
    "verbosity level (-1:CRITICAL, 0:ERROR, 1:WARNING, 2:INFO, 3:DEBUG, ...)",
    "path to scan for media files",
    "database to store media info",
    "this message",
    NULL
};

static void
build_path_internal(char *str, int *str_len, const char *dir, const char *file)
{
    char *p;
    int dlen, flen;

    dlen = strlen(dir);
    flen = strlen(file);
    if (dlen + flen + 1 >= *str_len) {
        *str_len = 0;
        str[0] = '\0';
    }
    *str_len = dlen + flen + 1;

    memcpy(str, dir, dlen);
    p = str + dlen;
    p[0] = '/';
    p++;

    memcpy(p, file, flen);
    str[*str_len] = '\0';
}

static char tmppath[PATH_MAX];

static const char *
build_theme_path(const char *file)
{
    char buf[PATH_MAX];
    int len;
    char *p;

    p = getenv("TECSYS_THEMESDIR");
    if (!p)
        p = THEMESDIR;

    len = sizeof(tmppath);
    build_path_internal(tmppath, &len, p, file);
    if (access(tmppath, F_OK) == 0)
        return tmppath;

    len = sizeof(buf);
    build_path_internal(buf, &len, getenv("HOME"), ".enjoy/themes");
    len = sizeof(tmppath);
    build_path_internal(tmppath, &len, buf, file);
    if (access(tmppath, F_OK) == 0)
        return tmppath;

    len = sizeof(tmppath);
    build_path_internal(tmppath, &len, ".", file);
    if (access(tmppath, F_OK) == 0)
        return tmppath;

    fprintf(stderr, "ERROR: could not locate theme file '%s'.\n", file);
    return NULL;
}

static void
show_help(const char *prg_name)
{
    const struct option *lo;
    const char **help;
    int largest;

    fprintf(stderr,
            "Usage:\n"
            "\t%s [options]\n"
            "where options are:\n",
            prg_name);

    lo = long_options;

    largest = 0;
    for (; lo->name != NULL; lo++) {
        int len = strlen(lo->name) + 9;

        if (lo->has_arg)
            len += sizeof("=ARG") - 1;

        if (largest < len)
            largest = len;
    }

    lo = long_options;
    help = help_texts;
    for (; lo->name != NULL; lo++, help++) {
        int len, i;

        fprintf(stderr, "\t-%c, --%s", lo->val, lo->name);
        len = strlen(lo->name) + 7;
        if (lo->has_arg) {
            fputs("=ARG", stderr);
            len += sizeof("=ARG") - 1;
        }

        for (i = len; i < largest; i++)
            fputc(' ', stderr);

        fputs("   ", stderr);
        fputs(*help, stderr);
        fputc('\n', stderr);
    }
    fputc('\n', stderr);
}

static int
parse_size(const char *text, int *w, int *h)
{
    const char *sep;
    char *p;

    sep = strchr(text, 'x');
    if (!sep) {
        fprintf(stderr,
                "ERROR: invalid size format, must be WIDTHxHEIGHT, got '%s'\n",
                text);
        return -1;
    }
    sep++;

    *w = strtol(text, &p, 10);
    if (text == p) {
        fprintf(stderr, "ERROR: could not parse size width '%s'\n", text);
        return -1;
    }

    *h = strtol(sep, &p, 10);
    if (sep == p) {
        fprintf(stderr, "ERROR: could not parse size height '%s'\n", text);
        return -1;
    }

    return 0;
}

static int
create_ecore_evas(struct app *app, int argc, char *argv[])
{
    int opt_index;
    const char *engine, *size;
    int w, h;

    engine = NULL;
    size = NULL;

    opt_index = 0;
    while (1) {
        int c;

        c = getopt_long(argc, argv, short_options, long_options, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'e':
            engine = optarg;
            break;
        case 's':
            size = optarg;
            break;
        case 'h':
            show_help(argv[0]);
            return 0;
        case 'v':
            app->verbosity = atoi(optarg);
            break;
        default:
            break;
        }
    }

    grn_log_level_set(app->verbosity);

    w = 800;
    h = 450;

    if (size)
        parse_size(size, &w, &h);

    app->ee = ecore_evas_new(engine, 0, 0, w, h, NULL);
    if (!app->ee) {
        fprintf(stderr, "ERROR: can't create ecore_evas for %s engine.\n",
                engine);
        return 0;
    }

    app->evas = ecore_evas_get(app->ee);
    ecore_evas_name_class_set(app->ee, "Enjoy", "Enjoy");
    ecore_evas_title_set(app->ee, "Enjoy Media Player");

    return 1;
}

static int
mkdir_p(char *dir, int mode)
{
    struct stat st;
    int i, len;

    len = strlen(dir);

    for (i = 1; i < len; i++) {
        if (dir[i] == '/') {
            dir[i] = '\0';

            if (stat(dir, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    dir[i] = '/';
                    continue;
                } else {
                    fprintf(stderr,
                            "ERROR: path exist but it's not a directory: %s\n",
                            dir);
                    return -1;
                }
            } else {
                if (mkdir(dir, mode) == 0) {
                    dir[i] = '/';
                    continue;
                } else {
                    fprintf(stderr,
                            "ERROR: could not create directory: %s\n", dir);
                    return -1;
                }
            }
        }
    }

    if (mkdir(dir, mode) == 0)
        return 0;
    else {
        fprintf(stderr,
                "ERROR: could not create directory: %s\n", dir);
        return -1;
    }
}

static int
fix_user_conf(const char *path)
{
    struct stat st;
    char *dir;
    int ret;

    dir = strdup(path);
    if (!dir)
        return 0;

    dir = dirname(dir);
    if (stat(dir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "ERROR: path exist but it's not a directory: %s\n",
                    dir);
            ret = 0;
            goto free_and_exit;
        } else {
            int fd;

            fd = open(path, O_RDWR | O_CREAT);
            if (fd < 0) {
                fprintf(stderr, "ERROR: could not create file: %s\n", path);
                ret = 0;
                goto free_and_exit;
            } else {
                close(fd);
                unlink(path);
                ret = 1;
                goto free_and_exit;
            }
        }
    } else
        ret = mkdir_p(dir, 0700) == 0;

free_and_exit:
    free(dir);
    return ret;
}

static int
create_manager(struct app *app, int argc, char *argv[])
{
    int opt_index, len;
    const char *file_ro, *file_rw, *key, *scanpath, *dbpath, *tmp;

    len = sizeof(tmppath);
    file_ro = SYSCONFDIR"/enjoy.conf";
    build_path_internal(tmppath, &len, getenv("HOME"), ".enjoy/config");
    file_rw = evas_stringshare_add(tmppath);
    build_path_internal(tmppath, &len, getenv("HOME"), ".enjoy/media.db");
    dbpath = evas_stringshare_add(tmppath);
    key = "/enjoy";
    scanpath = NULL;

    optind = 0;
    opterr = 0;
    opt_index = 0;
    while (1) {
        int c;

        c = getopt_long(argc, argv, short_options, long_options, &opt_index);
        if (c == -1)
            break;

        switch (c) {
        case 'S':
            file_ro = optarg;
            break;
        case 'u':
            tmp = evas_stringshare_add(optarg);
            evas_stringshare_del(file_rw);
            file_rw = tmp;
            break;
        case 'k':
            key = optarg;
            break;
        case 'f':
            scanpath = evas_stringshare_add(optarg);
            break;
        case 'b':
            tmp = evas_stringshare_add(optarg);
            evas_stringshare_del(dbpath);
            dbpath = tmp;
        default:
            break;
        }
    }

    if (access(file_ro, R_OK) != 0) {
        fprintf(stderr, "CRITICAL: system-conf (%s) is not readable: %s\n",
                file_ro, strerror(errno));
        return 0;
    }

    if (access(file_rw, R_OK | W_OK) != 0)
        if (!fix_user_conf(file_rw))
            fprintf(stderr, "ERROR: unable to save user configuration at: %s\n",
                    file_rw);

    if (access(dbpath, R_OK | W_OK) != 0)
        if (!fix_user_conf(dbpath)) {
            fprintf(stderr, "ERROR: unable to access media database \"%s\"\n",
                    dbpath);
            evas_stringshare_del(dbpath);
            dbpath = NULL;
        }

    app->manager = grn_manager_new(file_ro, file_rw, key);
    if (!app->manager) {
        fprintf(stderr,
                "ERROR: could not create manager: user-conf=%s,"
                " key-prefix=%s.\n",
                file_rw, key);
        return 0;
    }

    evas_stringshare_del(file_rw);

    grn_manager_global_default_set(app->manager);

    grn_manager_data_set(app->manager, "ecore_evas", app->ee, NULL);
    grn_manager_data_set(app->manager, "evas", app->evas, NULL);
    if (dbpath) {
        sqlite3 *db;
        grn_manager_data_set(app->manager, "dbpath", dbpath,
                             (grn_callback_t)evas_stringshare_del);
        if (sqlite3_open(dbpath, &db) == SQLITE_OK)
            grn_manager_data_set(app->manager, "db", db,
                                 (grn_callback_t)sqlite3_close);
    }
    if (scanpath)
        grn_manager_data_set(app->manager, "scanpath", scanpath,
                             (grn_callback_t)evas_stringshare_del);

    return 1;
}

static void
set_default_theme(struct app *app, int argc, char *argv[])
{
    int opt_index;

    optind = 0;
    opterr = 0;
    opt_index = 0;
    const char *theme_path;
    const char *abs_theme_path;

    theme_path = "default.edj";

    while (1) {
        int c;
        c = getopt_long(argc, argv, short_options, long_options, &opt_index);
        if (c == -1)
            break;
        if (c == 't') {
            theme_path = optarg;
            break;
        }
    }

    if (theme_path[0] == '/' || (theme_path[0] == '.' && theme_path[1] == '/'))
        abs_theme_path = theme_path;
    else
        abs_theme_path = build_theme_path(theme_path);

    grn_edje_widget_global_default_theme_set(abs_theme_path);
}

static void
on_ee_resize(Ecore_Evas *ee)
{
    struct app *app;
    int w, h;

    app = ecore_evas_data_get(ee, "app");
    if (!app)
        return;
    ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);
    evas_object_resize(app->controller->view, w, h);
}

static int
create_principal_controller(struct app *app, int argc, char *argv[])
{
    Evas_Object *view;
    int w, h;
    grn_model_t *model;

    model = ejy_principal_model_new();
    app->controller = ejy_principal_controller_new(model, NULL);
    if (!app->controller) {
        fputs("ERROR: could not create principal controller.\n", stderr);
        return 0;
    }

    if (!grn_controller_resume(app->controller)) {
        fputs("ERROR: principal controller failed to resume.\n", stderr);
        grn_controller_unref(app->controller);
        app->controller = NULL;
        return 0;
    }

    view = app->controller->view;

    if (ejy_principal_screen_size_min_get(view, &w, &h))
        ecore_evas_size_min_set(app->ee, w, h);
    if (ejy_principal_screen_size_max_get(view, &w, &h))
        if (w > 0 && h > 0)
            ecore_evas_size_max_set(app->ee, w, h);

    if (ejy_principal_screen_size_hint_get(view, &w, &h)) {
        ecore_evas_resize(app->ee, w, h);
        ecore_evas_geometry_get(app->ee, NULL, NULL, &w, &h);
        evas_object_resize(view, w, h);
    }

    ecore_evas_data_set(app->ee, "app", app);
    ecore_evas_callback_resize_set(app->ee, on_ee_resize);

    ecore_evas_borderless_set(
        app->ee, ejy_principal_screen_borderless_get(view));
    if (ejy_principal_screen_size_step_get(view, &w, &h))
        ecore_evas_size_step_set(app->ee, w, h);

    evas_object_show(view);
    evas_object_focus_set(view, 1);

    return 1;
}

static int
exit_signal(void *data, int type, void *e)
{
    Ecore_Event_Signal_Exit *event = e;

    fprintf(stderr, "Got exit signal [interrupt=%u, quit=%u, terminate=%u]\n",
            event->interrupt, event->quit, event->terminate);

    ecore_main_loop_quit();
    return 1;
}

static int
user_signal(void *data, int type, void *e)
{
    Ecore_Event_Signal_User *event = e;

    fprintf(stderr, "Got SIGUSR%d\n", event->number);

    if (event->number == 2) {
        int old_grn, new;

        old_grn = grn_log_level_get();
        new = (old_grn + 1) % 5;

        fprintf(stderr,
                "INFO: changing guarana log level from %d to %d\n",
                old_grn, new);

        grn_log_level_set(new);
    }

    return 1;
}

/* auxdbg.c */
extern void set_death_handlers(void);

int
main(int argc, char *argv[])
{
    struct app app;

    set_death_handlers();

    memset(&app, 0, sizeof(app));
    app.verbosity = 1; /* show warnings */

    ecore_evas_init();
    grn_widgets_init();

    if (!create_ecore_evas(&app, argc, argv))
        return -1;

    if (!create_manager(&app, argc, argv))
        return -2;

    set_default_theme(&app, argc, argv);
    if (!create_principal_controller(&app, argc, argv))
        return -3;

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_signal, &app);
    ecore_event_handler_add(ECORE_EVENT_SIGNAL_USER, user_signal, &app);

    ecore_evas_show(app.ee);
    ecore_main_loop_begin();

    fputs("INFO: main loop ended.\n", stderr);

    grn_controller_suspend(app.controller);
    if (grn_controller_ref_count(app.controller) != 1)
        fprintf(stderr, "principal controller have invalid ref_count: %u\n",
                grn_controller_ref_count(app.controller));

    grn_controller_unref(app.controller);
    app.controller = NULL;

    ecore_evas_free(app.ee);
    app.ee = NULL;

    grn_manager_del(app.manager);
    grn_manager_global_default_set(NULL);
    app.manager = NULL;

    grn_edje_widget_global_default_theme_set(NULL);

    grn_widgets_shutdown();
    ecore_evas_shutdown();

    return 0;
}
