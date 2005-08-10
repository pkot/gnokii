/*

  $Id$
  
  X G N O K I I

  A Linux/Unix GUI for the mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999 Pavel Janík ml., Hugh Blemings
  & 1999-2005 Jan Derfinak.

*/

#ifndef XGNOKII_LOGOS_H
#define XGNOKII_LOGOS_H

/* drawable point size - depends on .xpm file */
#define POINTSIZE 5

/* maximal size for drawing area */
#define MAXWIDTH 82
#define MAXHEIGHT 48

/* where to draw preview logos in previewPixmap */
#define PREVIEWSTARTX 28
#define PREVIEWSTARTY 160

#define PREVIEWWIDTH 138
#define PREVIEWHEIGHT 289

/* relative movement caller & operator logo from startuplogo */
#define PREVIEWJUMPX 6
#define PREVIEWJUMPY 6

#define TOOL_BRUSH 0
#define TOOL_LINE 1
#define TOOL_RECTANGLE 2
#define TOOL_FILLED_RECTANGLE 3
#define TOOL_CIRCLE 4
#define TOOL_FILLED_CIRCLE 5
#define TOOL_ELIPSE 6
#define TOOL_FILLED_ELIPSE 7
#define TOOL_TEXT 8

extern void GUI_ShowLogosWindow(void);
extern void GUI_CreateLogosWindow(void);

/* this is called from optionsApplyCallback when some changes
 * caller groups names */
extern void GUI_RefreshLogosGroupsCombo(void);

#endif
