/*
 * NekoGroup Utility API Header Declaration
 *
 * ng-utils.h
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

#ifndef HAVE_NG_UTILS_H
#define HAVE_NG_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <glib.h>

G_BEGIN_DECLS

gchar *ng_utils_get_jid_from_address(const gchar *address);
gchar *ng_utils_get_resource_from_address(const gchar *address);
gchar *ng_utils_get_shortname(const gchar *jid);
gchar *ng_utils_generate_new_nick(const gchar *jid);
gint64 ng_uitls_get_real_time();

G_END_DECLS

#endif

