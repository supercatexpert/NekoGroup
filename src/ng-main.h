/*
 * NekoGroup Main Header Declaration
 *
 * ng-main.h
 * This file is part of NekoGroup
 *
 * Copyright (C) 2012 - SuperCat, license: GPL v3
 *
 * NekoGroup is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * NekoGroup is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NekoGroup; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
#ifndef HAVE_NG_MAIN_H
#define HAVE_NG_MAIN_H

#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>

G_BEGIN_DECLS

gboolean ng_main_run(gint *argc, gchar **argv[]);
void ng_main_exit();
void ng_main_quit();

G_END_DECLS

#endif

