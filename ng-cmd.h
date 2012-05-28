/*
 * Command Header Declaration
 *
 * ng-cmd.h
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
 * along with <RhythmCat>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef HAVE_NG_COMMAND_H
#define HAVE_NG_COMMAND_H

#include <string.h>
#include <glib.h>
#include <loudmouth/loudmouth.h>
#include <glib/gprintf.h>

G_BEGIN_DECLS

gboolean ng_cmd_exec(const gchar *jid, const gchar *command);

G_END_DECLS

#endif


