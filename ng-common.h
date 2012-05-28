/*
 * NekoGroup Common Header Declaration
 *
 * ng-common.h
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

#ifndef HAVE_NG_COMMON_H
#define HAVE_NG_COMMON_H

#include <glib.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif

#define G_LOG_DOMAIN "NekoGroup"
#define GETTEXT_PACKAGE "NekoGroup"
#define PACKAGE "NekoGroup"

G_END_DECLS

#endif

