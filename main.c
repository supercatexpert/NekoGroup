/*
 * Main Declaration
 *
 * main.c
 * This file is part of <NekoGroup>
 *
 * Copyright (C) 2011 - SuperCat, license: GPL v3
 *
 * <NekoGroup> is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * <NekoGroup> is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with <NekoGroup>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <purple.h>
#include <glib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <glib/gprintf.h>
#include "core.h"
#include "log.h"

static GMainLoop *ng_main_loop;

static gboolean ng_main_working_timeout_cb(gpointer data)
{
    purple_idle_touch();
    return TRUE;
}

int main(int argc, char *argv[])
{
    ng_main_loop = g_main_loop_new(NULL, FALSE);
    signal(SIGCHLD, SIG_IGN);
    ng_log_init();
    if(!ng_core_init(&argc, &argv)) abort();
    g_timeout_add(15000, (GSourceFunc)(ng_main_working_timeout_cb), NULL);
    ng_core_account_init();
    g_main_loop_run(ng_main_loop);
    return 0;
}

