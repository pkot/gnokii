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

  Copyright (C) 1999      Pavel Janik ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak
  Copyright (C) 2000-2001 Marcin Wiacek, Chris Kemp
  Copyright (C) 2002-2006 Pawel Kot
  Copyright (C) 2002      Markus Plail

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>

#include <gtk/gtk.h>

#include "misc.h"
#include "gnokii.h"

#include "xgnokii_logos.h"
#include "xgnokii_common.h"
#include "xgnokii_lowlevel.h"
#include "xgnokii.h"

#include "xpm/Operator_logo.xpm"
#include "xpm/Startup_logo.xpm"
#include "xpm/Caller_logo.xpm"

#include "xpm/Black_point.xpm"
#include "xpm/Green_point.xpm"
#include "xpm/Green_pixel.xpm"

#include "xpm/New.xpm"
#include "xpm/Send.xpm"
#include "xpm/Read.xpm"
#include "xpm/Open.xpm"
#include "xpm/Save.xpm"

#include "xpm/Edit_invert.xpm"
#include "xpm/Edit_flip_horizontal.xpm"
#include "xpm/Edit_flip_vertical.xpm"

#include "xpm/Tool_brush.xpm"
#include "xpm/Tool_line.xpm"
#include "xpm/Tool_rectangle.xpm"
#include "xpm/Tool_filled_rectangle.xpm"

GtkWidget *GUI_LogosWindow;

ErrorDialog errorDialog = { NULL, NULL };
InfoDialog infoDialog = { NULL, NULL };

/* stuff for drawingArea */
GtkWidget *drawingArea = NULL;
GdkPixmap *drawingPixmap = NULL;
GdkPixmap *greenPointPixmap, *blackPointPixmap;
int drawingAreaWidth, drawingAreaHeight;	/* in pixels */
int mouseButtonPushed = 0;

/* stuff for previewArea */
#define MAX_PIXMAPS 100
GtkWidget *previewArea = NULL;
GdkPixmap *previewPixmap = NULL;
GdkPixmap *greenPixelPixmap;
int previewPixmapWidth, previewPixmapHeight;
int previewAvailable = 1, showPreviewErrorDialog = 1;
int previewPixmapNumber = 0, pixmapFiles = 0;
int pixmapsInitialized = FALSE, pixmapDefaultId = -1;
gchar *pixmapNames[MAX_PIXMAPS];

gn_bmp bitmap, oldBitmap;
gn_network_info networkInfo;

/* widgets for toolbar - some, need global variables */
GtkWidget *buttonStartup, *buttonOperator, *buttonCaller;
GtkWidget *networkCombo, *callerCombo;

int activeTool = TOOL_BRUSH;
int toolStartX, toolStartY, toolLastX, toolLastY;

/* tools for drawing */
static GtkWidget *buttonBrush, *buttonLine, *buttonRectangle;
static GtkWidget *buttonFilledRectangle;

/* Contains fileName for Export dialog. */
typedef struct {
	gchar *fileName;
} ExportDialogData;

static ExportDialogData exportDialogData = { NULL };

GtkWidget *FileSelection;

static int callersGroupsInitialized = 0;

/* returns lowest number from three numbers */
int GetMinFrom3(int a, int b, int c)
{
	if (a > b) {
		if (b > c)
			return c;
		else
			return b;
	} else {
		if (a > c)
			return c;
		else
			return a;
	}
}

/* returns highest number from three numbers */
int GetMaxFrom3(int a, int b, int c)
{
	if (a > b) {
		if (c > a)
			return c;
		else
			return a;
	} else {
		if (c > b)
			return c;
		else
			return b;
	}
}

/* Read all Preview* files from the given path and put them into pixmapNames
 * array. We support no more than MAX_PIXMAP files. */
static void GetPixmaps(gchar *path)
{
	DIR *dir;
	struct dirent *de;

	dir = opendir(path);
	while (dir && (de = readdir(dir))) {
		if (pixmapFiles == MAX_PIXMAPS)
			break;
		if (!strncmp(de->d_name, "Preview_", 8)) {
			pixmapNames[pixmapFiles] = g_strdup(de->d_name);
			if (!strcmp(de->d_name, "Preview_default.xpm"))
				pixmapDefaultId = pixmapFiles;
			pixmapFiles++;
		}
	}
	pixmapsInitialized = TRUE;
}

/* Get the pixmap that filename contains model string. If there's none, get
 * default one. */
static int GetPixmapId(gchar *model)
{
	int i, pos = pixmapDefaultId;

	for (i = 0; i < pixmapFiles; i++) {
		if (strstr(pixmapNames[i], model)) {
			pos = i;
			break;
		}
	}
	return pos;
}

/* Load preview pixmap from file. On the first run do the initialization.
 * Next runs are after clicking on the preview area that should switch to the
 * next pixmap. In case none is available NULL is returned. */
GdkPixmap *GetPreviewPixmap(GtkWidget *widget)
{
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask;
	gchar *file, *dirname;

	dirname = g_strdup_printf("%s/xpm/", xgnokiiConfig.xgnokiidir);
	if (!pixmapsInitialized) {
		GetPixmaps(dirname);
		previewPixmapNumber = GetPixmapId(xgnokiiConfig.model);
	}

	if (previewPixmapNumber >= 0) {
		file = g_strdup_printf("%s%s", dirname,
					pixmapNames[previewPixmapNumber]);
		pixmap = gdk_pixmap_create_from_xpm(widget->window, &mask,
					    &widget->style->bg[GTK_STATE_NORMAL], file);
		g_free(file);
	}
	g_free(dirname);
	return pixmap;
}

/* ********************************************************
 * ** SET/CLEAR POINTS ************************************
 * ********************************************************
 */
void SetPreviewPoint(GtkWidget * widget, int x, int y, int update)
{
	if (!previewAvailable)
		return;

	/* there is difference between positiong of startupLogo and others */
	if (bitmap.type != GN_BMP_StartupLogo) {
		x += PREVIEWJUMPX;
		y += PREVIEWJUMPY;
	}

	/* draw point to pixmap */
	if (previewPixmap)
		gdk_draw_point(previewPixmap, widget->style->black_gc,
			       x + PREVIEWSTARTX, y + PREVIEWSTARTY);

	if (update) {
		GdkRectangle updateRect;

		/* update point on screen */
		updateRect.width = 1;
		updateRect.height = 1;
		updateRect.x = PREVIEWSTARTX + x;
		updateRect.y = PREVIEWSTARTY + y;

		gtk_widget_draw(previewArea, &updateRect);
	}
}

void ClearPreviewPoint(GtkWidget * widget, int x, int y, int update)
{
	if (!previewAvailable)
		return;

	/* there is difference between positiong of startupLogo and others */
	if (bitmap.type != GN_BMP_StartupLogo) {
		x += PREVIEWJUMPX;
		y += PREVIEWJUMPY;
	}

	/* clean point from pixmap - any idea how to draw green point without pixmap? */
	if (previewPixmap)
		gdk_draw_pixmap(previewPixmap,
				widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
				greenPixelPixmap, 0, 0, x + PREVIEWSTARTX, y + PREVIEWSTARTY, 1, 1);
	if (update) {
		GdkRectangle updateRect;

		/* clean from screen too */
		updateRect.width = 1;
		updateRect.height = 1;
		updateRect.x = PREVIEWSTARTX + x;
		updateRect.y = PREVIEWSTARTY + y;

		gtk_widget_draw(previewArea, &updateRect);
	}
}

int IsPoint(int x, int y)
{
	return gn_bmp_point(&bitmap, x, y);
}

void SetPoint(GtkWidget * widget, int x, int y, int update)
{
	/* difference between settings points in startupLogo and others */
	gn_bmp_point_set(&bitmap, x, y);

	/* draw point to pixmap */
	gdk_draw_pixmap(drawingPixmap, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			blackPointPixmap, 0, 0, x * (POINTSIZE + 1), y * (POINTSIZE + 1), -1, -1);

	if (update) {
		GdkRectangle updateRect;

		/* calculate update rectangle */
		updateRect.width = POINTSIZE + 2;
		updateRect.height = POINTSIZE + 2;
		updateRect.x = x * (POINTSIZE + 1);
		updateRect.y = y * (POINTSIZE + 1);

		/* update on screen */
		gtk_widget_draw(drawingArea, &updateRect);
	}

	/* draw preview point too */
	if (previewAvailable)
		SetPreviewPoint(widget, x, y, update);
}

void ClearPoint(GtkWidget * widget, int x, int y, int update)
{
	/* difference between settings points in startupLogo and others */
	gn_bmp_point_clear(&bitmap, x, y);

	/* clear point from pixmap */
	gdk_draw_pixmap(drawingPixmap, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			greenPointPixmap, 0, 0, x * (POINTSIZE + 1), y * (POINTSIZE + 1), -1, -1);

	if (update) {
		GdkRectangle updateRect;

		/* calculate update rectangle */
		updateRect.width = POINTSIZE + 2;
		updateRect.height = POINTSIZE + 2;
		updateRect.x = x * (POINTSIZE + 1);
		updateRect.y = y * (POINTSIZE + 1);

		/* update on screen */
		gtk_widget_draw(drawingArea, &updateRect);
	}

	/* clear point from previewArea too */
	if (previewAvailable)
		ClearPreviewPoint(widget, x, y, update);
}

/* ****************************************************
 * *** UPDATES - PREVIEW & DRAWING AREAS **************
 * ****************************************************
 */

/* this redraw all logo points - preview & drawing area */
void UpdatePointsRectangle(GtkWidget * widget, int x1, int y1, int x2, int y2)
{
	GdkRectangle updateRect;
	int x, y, dx = 0, dy = 0;

	if (bitmap.type != GN_BMP_StartupLogo) {
		dx = PREVIEWJUMPX;
		dy = PREVIEWJUMPY;
	}

	if (x1 > x2) {
		x = x1;
		x1 = x2;
		x2 = x;
	}

	if (y1 > y2) {
		y = y1;
		y1 = y2;
		y2 = y;
	}

	for (y = y1; y <= y2; y++)
		for (x = x1; x <= x2; x++) {
			if (IsPoint(x, y)) {
				/* set on drawing area */
				gdk_draw_pixmap(drawingPixmap,
						drawingArea->style->
						fg_gc[GTK_WIDGET_STATE(drawingArea)],
						blackPointPixmap, 0, 0, x * (POINTSIZE + 1),
						y * (POINTSIZE + 1), -1, -1);

				/* set on preview */
				if (previewAvailable && previewPixmap)
					gdk_draw_point(previewPixmap, previewArea->style->black_gc,
						       x + PREVIEWSTARTX + dx,
						       y + PREVIEWSTARTY + dy);
			} else {
				/* clear from drawing */
				gdk_draw_pixmap(drawingPixmap,
						drawingArea->style->
						fg_gc[GTK_WIDGET_STATE(drawingArea)],
						greenPointPixmap, 0, 0, x * (POINTSIZE + 1),
						y * (POINTSIZE + 1), -1, -1);

				/* clear from preview */
				if (previewAvailable && previewPixmap)
					gdk_draw_pixmap(previewPixmap,
							previewArea->style->
							fg_gc[GTK_WIDGET_STATE(previewArea)],
							greenPixelPixmap, 0, 0,
							x + PREVIEWSTARTX + dx,
							y + PREVIEWSTARTY + dy, 1, 1);
			}
		}

	if (previewAvailable) {
		updateRect.x = PREVIEWSTARTX + dx + x1;
		updateRect.y = PREVIEWSTARTY + dy + y1;
		updateRect.width = x2 - x1 + 1;
		updateRect.height = y2 - y1 + 1;
		gtk_widget_draw(previewArea, &updateRect);
	}

	updateRect.x = x1 * (POINTSIZE + 1);
	updateRect.y = y1 * (POINTSIZE + 1);
	updateRect.width = (x2 - x1 + 1) * (POINTSIZE + 1) + 1;
	updateRect.height = (y2 - y1 + 1) * (POINTSIZE + 1) + 1;
	gtk_widget_draw(drawingArea, &updateRect);
}

void UpdatePoints(GtkWidget * widget)
{
	UpdatePointsRectangle(widget, 0, 0, bitmap.width - 1, bitmap.height - 1);
}

/* this redraw all logo points in previewArea, NO DRAWING AREA */
void UpdatePreviewPoints(void)
{
	GdkRectangle updateRect;
	int x, y, dx = 0, dy = 0;

	if (!previewPixmap || !previewAvailable)
		return;

	if (bitmap.type != GN_BMP_StartupLogo) {
		dx = PREVIEWJUMPX;
		dy = PREVIEWJUMPY;
	}

	for (y = 0; y < bitmap.height; y++)
		for (x = 0; x < bitmap.width; x++) {
			if (IsPoint(x, y)) {
				gdk_draw_point(previewPixmap, previewArea->style->black_gc,
					       x + PREVIEWSTARTX + dx, y + PREVIEWSTARTY + dy);
			} else {
				gdk_draw_pixmap(previewPixmap,
						previewArea->style->
						fg_gc[GTK_WIDGET_STATE(previewArea)],
						greenPixelPixmap, 0, 0, x + PREVIEWSTARTX + dx,
						y + PREVIEWSTARTY + dy, 1, 1);
			}
		}

	updateRect.x = dx;
	updateRect.y = dy;
	updateRect.width = bitmap.width;
	updateRect.height = bitmap.height;
	gtk_widget_draw(previewArea, &updateRect);
}

/* ******************************************************
 * **** DRAWING TOOLS ***********************************
 * ******************************************************
 */

/* TOOL - BRUSH */
void ToolBrush(GtkWidget * widget, int column, int row, int button)
{
	/* only this tool directly update bitmap & screen */
	if (button > 1)
		ClearPoint(widget, column, row, 1);
	else
		SetPoint(widget, column, row, 1);
}

/* TOOL - LINE */

/* this function clear or draw a line on the screen  USED BY TOOLLINEUPDATE */
void ToolLine(GtkWidget * widget, int x1, int y1, int x2, int y2, int draw)
{
	int udx, udy, dx, dy, error, loop, xadd, yadd;

	dx = x2 - x1;		/* x delta */
	dy = y2 - y1;		/* y delta */

	udx = abs(dx);		/* unsigned x delta */
	udy = abs(dy);		/* unsigned y delta */

	if (dx < 0) {
		xadd = -1;
	} else {
		xadd = 1;
	}			/* set directions */
	if (dy < 0) {
		yadd = -1;
	} else {
		yadd = 1;
	}

	error = 0;
	loop = 0;
	if (udx > udy) {	/* delta X > delta Y */
		do {
			error += udy;

			if (error >= udx) {	/* is time to move up or down? */
				error -= udx;
				y1 += yadd;
			}
			loop++;
			if (draw == 1) {
				SetPoint(widget, x1, y1, 0);
			} else {
				/* now clearing line before drawing new one, we must check */
				/* if there is a point in oldBitmap which saves bitmap before */
				/* we starting drawing new line */
				if (!gn_bmp_point(&oldBitmap, x1, y1)) {
					ClearPoint(widget, x1, y1, 0);
				}
			}
			x1 += xadd;	/* move horizontally */
		} while (loop < udx);	/* repeat for x length */
	} else {
		do {
			error += udx;
			if (error >= udy) {	/* is time to move left or right? */
				error -= udy;
				x1 += xadd;
			}
			loop++;
			if (draw == 1) {
				SetPoint(widget, x1, y1, 0);
			} else {
				/* check comment in delta X > delta Y */
				if (!gn_bmp_point(&oldBitmap, x1, y1)) {
					ClearPoint(widget, x1, y1, 0);
				}
			}
			y1 += yadd;	/* move vertically */
		} while (loop < udy);	/* repeat for y length */
	}
}

/* going to rewrite to Bresenham algorithm */
void ToolLineUpdate(GtkWidget * widget, int column, int row)
{
	/* clear old line */
	ToolLine(widget, toolStartX, toolStartY, toolLastX, toolLastY, 0);
	/* draw new one */
	ToolLine(widget, toolStartX, toolStartY, column, row, 1);
}

/* TOOL - FILLED RECT */

/* FIXME - going to rewrite for optimalized version, clearing and */
/*         drawing new parts only before clearing and drawing whole */
/*         filled rectangle - it's too slow on diskless terminal ;(( */
void ToolFilledRectangleUpdate(GtkWidget * widget, int column, int row)
{
	int i, j, x1, y1, x2, y2;

	/* swap Xs to x1 < x2 */
	if (toolStartX > toolLastX) {
		x1 = toolLastX;
		x2 = toolStartX;
	} else {
		x1 = toolStartX;
		x2 = toolLastX;
	}

	/* swap Ys to y1 < y2 */
	if (toolStartY > toolLastY) {
		y1 = toolLastY;
		y2 = toolStartY;
	} else {
		y1 = toolStartY;
		y2 = toolLastY;
	}

	/* clear one now */
	for (j = y1; j <= y2; j++)
		for (i = x1; i <= x2; i++)
			if (!gn_bmp_point(&oldBitmap, i, j))
				ClearPoint(widget, i, j, 0);

	/* swap Xs to x1 < x2 */
	if (toolStartX > column) {
		x1 = column;
		x2 = toolStartX;
	} else {
		x1 = toolStartX;
		x2 = column;
	}

	/* swap Ys to y1 < y2 */
	if (toolStartY > row) {
		y1 = row;
		y2 = toolStartY;
	} else {
		y1 = toolStartY;
		y2 = row;
	}

	/* draw new one */
	for (j = y1; j <= y2; j++)
		for (i = x1; i <= x2; i++)
			SetPoint(widget, i, j, 0);
}

/* TOOL - RECTANGLE */
void ToolRectangleUpdate(GtkWidget * widget, int column, int row)
{
	int i, j, x1, y1, x2, y2;

	/* clear old rectangle */
	/* swap Xs to x1 < x2 */
	if (toolStartX > toolLastX) {
		x1 = toolLastX;
		x2 = toolStartX;
	} else {
		x1 = toolStartX;
		x2 = toolLastX;
	}

	/* swap Ys to y1 < y2 */
	if (toolStartY > toolLastY) {
		y1 = toolLastY;
		y2 = toolStartY;
	} else {
		y1 = toolStartY;
		y2 = toolLastY;
	}

	/* clear old one */
	for (i = x1; i <= x2; i++) {
		if (!gn_bmp_point(&oldBitmap, i, y1))
			ClearPoint(widget, i, y1, 0);
		if (!gn_bmp_point(&oldBitmap, i, y2))
			ClearPoint(widget, i, y2, 0);
	}

	for (j = y1; j <= y2; j++) {
		if (!gn_bmp_point(&oldBitmap, x1, j))
			ClearPoint(widget, x1, j, 0);
		if (!gn_bmp_point(&oldBitmap, x2, j))
			ClearPoint(widget, x2, j, 0);
	}

	/* draw new rectangle */
	/* swap Xs to x1 < x2 */
	if (toolStartX > column) {
		x1 = column;
		x2 = toolStartX;
	} else {
		x1 = toolStartX;
		x2 = column;
	}

	/* swap Ys to y1 < y2 */
	if (toolStartY > row) {
		y1 = row;
		y2 = toolStartY;
	} else {
		y1 = toolStartY;
		y2 = row;
	}

	/* draw new one */
	for (i = x1; i <= x2; i++) {
		if (!IsPoint(i, y1))
			SetPoint(widget, i, y1, 0);
		if (!IsPoint(i, y2))
			SetPoint(widget, i, y2, 0);
	}

	for (j = y1; j <= y2; j++) {
		if (!IsPoint(x1, j))
			SetPoint(widget, x1, j, 0);
		if (!IsPoint(x2, j))
			SetPoint(widget, x2, j, 0);
	}
}

/* this update tools actions on the screen - this is for optimalization */
/* eg. for faster redrawing tools actions - we do not need redraw pixel */
/* by pixel. Faster is redraw whole rectangle which contains all changes */
void UpdateToolScreen(GtkWidget * widget, int x1, int y1, int x2, int y2)
{
	GdkRectangle updateRect;

	/* update preview area */
	if (previewAvailable) {
		updateRect.x = PREVIEWSTARTX + x1;
		updateRect.y = PREVIEWSTARTY + y1;
		if (bitmap.type != GN_BMP_StartupLogo) {
			updateRect.x += PREVIEWJUMPX;
			updateRect.y += PREVIEWJUMPY;
		}
		updateRect.width = x2 - x1 + 1;
		updateRect.height = y2 - y1 + 1;
		gtk_widget_draw(previewArea, &updateRect);
	}

	/* update drawing area */
	updateRect.x = x1 * (POINTSIZE + 1);
	updateRect.y = y1 * (POINTSIZE + 1);
	updateRect.width = (x2 - x1 + 1) * (POINTSIZE + 2);
	updateRect.height = (y2 - y1 + 1) * (POINTSIZE + 2);
	gtk_widget_draw(drawingArea, &updateRect);
}

/* *************************************
 * ** PREVIEW AREA EVENTS **************
 * *************************************
 */

gint PreviewAreaButtonPressEvent(GtkWidget * widget, GdkEventButton * event)
{
	previewPixmapNumber = ((previewPixmapNumber + 1) % pixmapFiles);

	gtk_drawing_area_size(GTK_DRAWING_AREA(previewArea),
			      previewPixmapWidth, previewPixmapHeight);

	return TRUE;
}

gint PreviewAreaConfigureEvent(GtkWidget * widget, GdkEventConfigure * event)
{
	if (previewPixmap)
		gdk_pixmap_unref(previewPixmap);
	previewPixmap = GetPreviewPixmap(widget);

	UpdatePreviewPoints();

	return TRUE;
}

gint PreviewAreaExposeEvent(GtkWidget * widget, GdkEventExpose * event)
{
	/* got previewPixmap? */
	if (previewPixmap)
		/* yes - simply redraw some rectangle */
		gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
				previewPixmap, event->area.x, event->area.y, event->area.x,
				event->area.y, event->area.width, event->area.height);

	return FALSE;
}

/* ********************************
 * ** DRAWING AREA EVENTS *********
 * ********************************
 */

gint DrawingAreaButtonPressEvent(GtkWidget * widget, GdkEventButton * event)
{
	/* got drawingPixmap? */
	if (drawingPixmap == NULL)
		return TRUE;

	if (!mouseButtonPushed) {
		if ((event->button == 1 && activeTool != TOOL_BRUSH) || (activeTool == TOOL_BRUSH)) {
			/* position from we starting drawing */
			toolStartX = event->x / (POINTSIZE + 1);
			if (toolStartX < 0)
				toolStartX = 0;
			if (toolStartX > bitmap.width - 1)
				toolStartX = bitmap.width - 1;

			toolStartY = event->y / (POINTSIZE + 1);
			if (toolStartY < 0)
				toolStartY = 0;
			if (toolStartY > bitmap.height - 1)
				toolStartY = bitmap.height - 1;

			toolLastX = toolStartX;
			toolLastY = toolStartY;

			/* store old bitmap for drawing, resp. for moving, resizing primitive */
			memcpy(&oldBitmap, &bitmap, sizeof(oldBitmap));
		}

		if (event->button == 1)
			mouseButtonPushed = 1;

		switch (activeTool) {
		case TOOL_BRUSH:
			ToolBrush(widget, toolStartX, toolStartY, event->button);
			break;
		case TOOL_LINE:
		case TOOL_RECTANGLE:
			if (event->button == 1)
				ToolBrush(widget, toolStartX, toolStartY, event->button);
			break;
		}
	}

	/* user is drawing some tool other than TOOL_BRUSH and pushed mouse button
	 * another than first => cancel tool and redraw to oldBitmap (bitmap when
	 * user start drawing)
	 */
	if (mouseButtonPushed && activeTool != TOOL_BRUSH && event->button != 1) {
		int lowestX, lowestY, highestX, highestY;
		int i, j;

		lowestX = GetMinFrom3(toolStartX, toolLastX, toolLastX);
		lowestY = GetMinFrom3(toolStartY, toolLastY, toolLastY);
		highestX = GetMaxFrom3(toolStartX, toolLastX, toolLastX);
		highestY = GetMaxFrom3(toolStartY, toolLastY, toolLastY);

		for (j = lowestY; j <= highestY; j++)
			for (i = lowestX; i <= highestX; i++)
				if (gn_bmp_point(&oldBitmap, i, j))
					SetPoint(widget, i, j, 0);
				else
					ClearPoint(widget, i, j, 0);
		UpdateToolScreen(widget, lowestX, lowestY, highestX, highestY);

		mouseButtonPushed = 0;
	}

	return TRUE;
}

gint DrawingAreaButtonReleaseEvent(GtkWidget * widget, GdkEventButton * event)
{
	if (event->button == 1)
		mouseButtonPushed = 0;

	return TRUE;
}

gint DrawingAreaMotionNotifyEvent(GtkWidget * widget, GdkEventMotion * event)
{
	int x, y;
	GdkModifierType state;

	if (!mouseButtonPushed && activeTool != TOOL_BRUSH)
		return TRUE;

	if (event->is_hint)
		gdk_window_get_pointer(event->window, &x, &y, &state);
	else {
		x = event->x;
		y = event->y;
		state = event->state;
	}

	x = x / (POINTSIZE + 1);
	y = y / (POINTSIZE + 1);
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x > bitmap.width - 1)
		x = bitmap.width - 1;
	if (y > bitmap.height - 1)
		y = bitmap.height - 1;

	if (y == toolLastY && x == toolLastX)
		return TRUE;

	switch (activeTool) {
	case TOOL_BRUSH:
		if (state & GDK_BUTTON1_MASK && drawingPixmap != NULL)
			ToolBrush(widget, x, y, 1);
		if (state & GDK_BUTTON2_MASK && drawingPixmap != NULL)
			ToolBrush(widget, x, y, 2);
		if (state & GDK_BUTTON3_MASK && drawingPixmap != NULL)
			ToolBrush(widget, x, y, 3);
		break;
	case TOOL_RECTANGLE:
		if (drawingPixmap != NULL)
			ToolRectangleUpdate(widget, x, y);
		break;
	case TOOL_FILLED_RECTANGLE:
		if (drawingPixmap != NULL)
			ToolFilledRectangleUpdate(widget, x, y);
		break;
	case TOOL_LINE:
		if (drawingPixmap != NULL)
			ToolLineUpdate(widget, x, y);
		break;
	}

	/* what is this?
	 * it's simple, above tools updates only bitmap in memory and this
	 * function update from bitmap to screen, it's made as non-blinking
	 * drawing functions with this, simply draw everything we need and
	 * after that, redraw to screen rectangle in which we made changes 
	 * it's not redrawing pixel by pixel (blinking)
	 */
	if (activeTool != TOOL_BRUSH) {
		int lowestX, lowestY, highestX, highestY;

		lowestX = GetMinFrom3(toolStartX, toolLastX, x);
		lowestY = GetMinFrom3(toolStartY, toolLastY, y);
		highestX = GetMaxFrom3(toolStartX, toolLastX, x);
		highestY = GetMaxFrom3(toolStartY, toolLastY, y);

		UpdateToolScreen(widget, lowestX, lowestY, highestX, highestY);
	}

	toolLastX = x;
	toolLastY = y;
	return TRUE;
}

/* configureEvent? -> event when someone resize windows, ... */
gint DrawingAreaConfigureEvent(GtkWidget * widget, GdkEventConfigure * event)
{
	int x, y;
	/* got drawingPixmap? */
	if (drawingPixmap)
		gdk_pixmap_unref(drawingPixmap);	/* got, erase it */

	/* make a new pixmap */
	drawingPixmap = gdk_pixmap_new(widget->window, drawingAreaWidth, drawingAreaHeight, -1);

	/* draw grid into pixmap */
	for (y = 0; y < bitmap.height; y++)
		for (x = 0; x < bitmap.width; x++)
			if (IsPoint(x, y))
				gdk_draw_pixmap(drawingPixmap,
						widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
						blackPointPixmap, 0, 0, x * (POINTSIZE + 1),
						y * (POINTSIZE + 1), -1, -1);
			else
				gdk_draw_pixmap(drawingPixmap,
						widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
						greenPointPixmap, 0, 0, x * (POINTSIZE + 1),
						y * (POINTSIZE + 1), -1, -1);

	return TRUE;
}

gint DrawingAreaExposeEvent(GtkWidget * widget, GdkEventExpose * event)
{
	/* got drawingPixmap? */
	if (drawingPixmap)
		/* got - draw it */
		gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
				drawingPixmap,
				event->area.x, event->area.y, event->area.x, event->area.y,
				event->area.width, event->area.height);
	return FALSE;
}

/* *****************************************
 * ** TOOLBAR & MENU EVENTS ****************
 * *****************************************
 */

void GetNetworkInfoEvent(GtkWidget * widget)
{
	gn_error error;
	gchar *netcou;
	PhoneEvent *e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	D_NetworkInfo *data = (D_NetworkInfo *) g_malloc(sizeof(D_NetworkInfo));

	/* prepare data for event */
	data->info = &networkInfo;
	e->event = Event_GetNetworkInfo;
	e->data = data;

	/* launch event and wait for result */
	GUI_InsertEvent(e);
	pthread_mutex_lock(&getNetworkInfoMutex);
	pthread_cond_wait(&getNetworkInfoCond, &getNetworkInfoMutex);
	pthread_mutex_unlock(&getNetworkInfoMutex);
	error = data->status;
	g_free(data);

	/* watch for errors */
	if (error != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error getting network info\n%s"), gn_error_print(error));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
		/* after the error network_code is undefined but later code won't know */
		networkInfo.network_code[0] = '\0';
	}

	/* set new operator name to combo */
	netcou = g_strdup_printf("%s (%s)", gn_network_name_get(networkInfo.network_code),
					    gn_network2country(networkInfo.network_code));
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(networkCombo)->entry), netcou);
}

void GetLogoEvent(GtkWidget * widget)
{
	gn_error error;
	int i;
	PhoneEvent *e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	D_Bitmap *data = (D_Bitmap *) g_malloc(sizeof(D_Bitmap));
	gchar *netcou = (gchar *) gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(networkCombo)->entry));
	gchar network[64], country[24];

	/* prepare data for event */
	sscanf(netcou, "%s (%[^)])", network, country);
	strncpy(bitmap.netcode, gn_network_code_find(network, country), sizeof(bitmap.netcode));
	data->bitmap = &bitmap;
	e->event = Event_GetBitmap;
	e->data = data;
	if (phoneMonitor.supported & PM_CALLERGROUP) {
		for (i = 0; i < GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER; i++)
			if (strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(callerCombo)->entry)),
				   xgnokiiConfig.callerGroups[i]) == 0)
				bitmap.number = i;
	}

	/* launch event and wait for result */
	GUI_InsertEvent(e);
	pthread_mutex_lock(&getBitmapMutex);
	pthread_cond_wait(&getBitmapCond, &getBitmapMutex);
	pthread_mutex_unlock(&getBitmapMutex);
	error = data->status;
	g_free(data);

	/* watch for errors */
	if (error != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error getting bitmap\n%s"), gn_error_print(error));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
	} else {
		/* no error, draw logo from phone */
		UpdatePoints(drawingArea);
	}
}

void SetLogoEvent(GtkWidget * widget)
{
	gn_error error;
	PhoneEvent *e = (PhoneEvent *) g_malloc(sizeof(PhoneEvent));
	D_Bitmap *data = (D_Bitmap *) g_malloc(sizeof(D_Bitmap));
	gchar *netcou = (gchar *) gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(networkCombo)->entry));
	gchar network[64], country[24];
	int i;

	/* prepare data */
	sscanf(netcou, "%s (%[^)])", network, country);
	strncpy(bitmap.netcode, gn_network_code_find(network, country), sizeof(bitmap.netcode));

	if (bitmap.type == GN_BMP_CallerLogo) {
		/* above condition must be there, because if you launch logos before
		 * callerGroups are available, you will see segfault - callerGroups not initialized 
		 */
		if (phoneMonitor.supported & PM_CALLERGROUP) {
			for (i = 0; i < GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER; i++)
				if (strcmp
				    (gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(callerCombo)->entry)),
				     xgnokiiConfig.callerGroups[i]) == 0)
					bitmap.number = i;
		}
	}

	data->bitmap = &bitmap;
	e->event = Event_SetBitmap;
	e->data = data;

	/* launch event and wait for result */
	GUI_InsertEvent(e);
	pthread_mutex_lock(&setBitmapMutex);
	pthread_cond_wait(&setBitmapCond, &setBitmapMutex);
	pthread_mutex_unlock(&setBitmapMutex);
	error = data->status;
	g_free(data);

	/* watch for errors */
	if (error != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error setting bitmap\n%s"), gn_error_print(error));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
	}
}

static void ClearLogoEvent(GtkWidget * widget)
{
	/*
	bitmap.size=bitmap.width*bitmap.height/8;
	*/
	gn_bmp_clear(&bitmap);

	UpdatePoints(widget);
}

void InvertLogoEvent(GtkWidget * widget)
{
	int column, row;

	for (column = 0; column < bitmap.width; column++)
		for (row = 0; row < bitmap.height; row++)
			if (IsPoint(column, row))
				gn_bmp_point_clear(&bitmap, column, row);
			else
				gn_bmp_point_set(&bitmap, column, row);

	UpdatePoints(widget);
}

void UpLogoEvent(GtkWidget * widget)
{
	int column, row;

	gn_bmp tbitmap;

	memcpy(&tbitmap, &bitmap, sizeof(gn_bmp));

	for (row = 0; row < bitmap.height - 1; row++)
		for (column = 0; column < bitmap.width; column++)
			if (IsPoint(column, row + 1))
				gn_bmp_point_set(&bitmap, column, row);
			else
				gn_bmp_point_clear(&bitmap, column, row);

	for (column = 0; column < bitmap.width; column++)
		if (gn_bmp_point(&tbitmap, column, 0))
			gn_bmp_point_set(&bitmap, column, bitmap.height - 1);
		else
			gn_bmp_point_clear(&bitmap, column, bitmap.height - 1);

	UpdatePoints(widget);
}

void DownLogoEvent(GtkWidget * widget)
{
	int column, row;

	gn_bmp tbitmap;

	memcpy(&tbitmap, &bitmap, sizeof(gn_bmp));

	for (row = bitmap.height - 1; row > 0; row--)
		for (column = 0; column < bitmap.width; column++)
			if (IsPoint(column, row - 1))
				gn_bmp_point_set(&bitmap, column, row);
			else
				gn_bmp_point_clear(&bitmap, column, row);

	for (column = 0; column < bitmap.width; column++)
		if (gn_bmp_point(&tbitmap, column, bitmap.height - 1))
			gn_bmp_point_set(&bitmap, column, 0);
		else
			gn_bmp_point_clear(&bitmap, column, 0);

	UpdatePoints(widget);
}

void LeftLogoEvent(GtkWidget * widget)
{
	int column, row;

	gn_bmp tbitmap;

	memcpy(&tbitmap, &bitmap, sizeof(gn_bmp));

	for (column = 0; column < bitmap.width - 1; column++)
		for (row = 0; row < bitmap.height; row++)
			if (IsPoint(column + 1, row))
				gn_bmp_point_set(&bitmap, column, row);
			else
				gn_bmp_point_clear(&bitmap, column, row);

	for (row = 0; row < bitmap.height; row++)
		if (gn_bmp_point(&tbitmap, 0, row))
			gn_bmp_point_set(&bitmap, bitmap.width - 1, row);
		else
			gn_bmp_point_clear(&bitmap, bitmap.width - 1, row);

	UpdatePoints(widget);
}

void RightLogoEvent(GtkWidget * widget)
{
	int column, row;

	gn_bmp tbitmap;

	memcpy(&tbitmap, &bitmap, sizeof(gn_bmp));

	for (column = bitmap.width - 1; column > 0; column--)
		for (row = 0; row < bitmap.height; row++)
			if (IsPoint(column - 1, row))
				gn_bmp_point_set(&bitmap, column, row);
			else
				gn_bmp_point_clear(&bitmap, column, row);

	for (row = 0; row < bitmap.height; row++)
		if (gn_bmp_point(&tbitmap, bitmap.width - 1, row))
			gn_bmp_point_set(&bitmap, 0, row);
		else
			gn_bmp_point_clear(&bitmap, 0, row);

	UpdatePoints(widget);
}

void FlipVerticalLogoEvent(GtkWidget * widget)
{
	int row, column, temp;

	for (row = 0; row < (bitmap.height / 2); row++)
		for (column = 0; column < bitmap.width; column++) {
			temp = IsPoint(column, row);
			if (IsPoint(column, bitmap.height - 1 - row))
				gn_bmp_point_set(&bitmap, column, row);
			else
				gn_bmp_point_clear(&bitmap, column, row);

			if (temp)
				gn_bmp_point_set(&bitmap, column, bitmap.height - 1 - row);
			else
				gn_bmp_point_clear(&bitmap, column, bitmap.height - 1 - row);
		}

	UpdatePoints(widget);
}

void FlipHorizontalLogoEvent(GtkWidget * widget)
{
	int row, column, temp;

	for (row = 0; row < bitmap.height; row++)
		for (column = 0; column < (bitmap.width / 2); column++) {
			temp = IsPoint(column, row);

			if (IsPoint(bitmap.width - 1 - column, row))
				gn_bmp_point_set(&bitmap, column, row);
			else
				gn_bmp_point_clear(&bitmap, column, row);

			if (temp)
				gn_bmp_point_set(&bitmap, bitmap.width - 1 - column, row);
			else
				gn_bmp_point_clear(&bitmap, bitmap.width - 1 - column, row);
		}

	UpdatePoints(widget);
}

/* this is launched when tool was changed */
gint ToolTypeEvent(GtkWidget * widget)
{
	if (GTK_TOGGLE_BUTTON(buttonBrush)->active)
		activeTool = TOOL_BRUSH;
	else if (GTK_TOGGLE_BUTTON(buttonLine)->active)
		activeTool = TOOL_LINE;
	else if (GTK_TOGGLE_BUTTON(buttonRectangle)->active)
		activeTool = TOOL_RECTANGLE;
	else if (GTK_TOGGLE_BUTTON(buttonFilledRectangle)->active)
		activeTool = TOOL_FILLED_RECTANGLE;

	return 0;
}

/* this is launched when logo type was changed by buttons on toolbar */
gint LogoTypeEvent(GtkWidget * widget)
{
	int clear = 0;
	gn_log_xdebug("LogoTypeEvent called!\n");
	gn_log_xdebug("width: %i, height: %i\n", statemachine->driver.phone.startup_logo_width,
		statemachine->driver.phone.startup_logo_height);
	/* is startupLogo? */
	/* Resize and clear anyway - CK */
	if (GTK_TOGGLE_BUTTON(buttonStartup)->active) {
		/* look for old bitmap type, clean if another */
		clear = 1;
		gn_bmp_resize(&bitmap, GN_BMP_StartupLogo, &statemachine->driver.phone);
	}

	/* has phone support for callerGroups? */
	if (phoneMonitor.supported & PM_CALLERGROUP) {
		if (GTK_TOGGLE_BUTTON(buttonCaller)->active && bitmap.type != GN_BMP_CallerLogo) {
			/* previous was startup? clear and draw batteries, signal, ... */
			/* Clear anyway for 7110...CK */
			clear = 1;
			gn_bmp_resize(&bitmap, GN_BMP_CallerLogo, &statemachine->driver.phone);
		}
	}

	/* is new type operatorLogo? */
	if ((GTK_TOGGLE_BUTTON(buttonOperator)->active && bitmap.type != GN_BMP_OperatorLogo) ||
	    (GTK_TOGGLE_BUTTON(buttonOperator)->active && bitmap.type != GN_BMP_NewOperatorLogo)) {
		/* previous startup? clear and draw batteries, signal, ... */
		/* Clear anyway for 7110..CK */
		clear = 1;
		if (!strncmp(statemachine->driver.phone.models, "6510", 4))
			gn_bmp_resize(&bitmap, GN_BMP_NewOperatorLogo, &statemachine->driver.phone);
		else
			gn_bmp_resize(&bitmap, GN_BMP_OperatorLogo, &statemachine->driver.phone);
	}

	/* must clear? */
	if (clear) {
		if (previewAvailable) {
			/* configure event reload pixmap from disk and redraws */
			gtk_drawing_area_size(GTK_DRAWING_AREA(previewArea),
					      previewPixmapWidth, previewPixmapHeight);
		}
		/* change new drawingArea size */
		drawingAreaWidth = bitmap.width * (POINTSIZE + 1) + 1;
		drawingAreaHeight = bitmap.height * (POINTSIZE + 1) + 1;

		gtk_drawing_area_size(GTK_DRAWING_AREA(drawingArea),
				      drawingAreaWidth, drawingAreaHeight);
	}
	return 0;
}

inline void CloseLogosWindow(void)
{
	gtk_widget_hide(GUI_LogosWindow);
}

void ExportLogoFileMain(gchar * name)
{
	gn_bmp tbitmap;
	gn_error error;

	tbitmap = bitmap;

	strncpy(tbitmap.netcode, gn_network_code_get(networkInfo.network_code), sizeof(tbitmap.netcode));

	error = gn_file_bitmap_save(name, &tbitmap, &statemachine->driver.phone);
	if (error != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error writing file\n%s"), gn_error_print(error));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
	}
}

static void YesLogoFileExportDialog(GtkWidget * w, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(data));
	ExportLogoFileMain(exportDialogData.fileName);
}

static void ExportFileSelected(GtkWidget * w, GtkFileSelection * fs)
{
	static YesNoDialog dialog = { NULL, NULL };
	FILE *f;
	gchar err[255];

	exportDialogData.fileName = (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	gtk_widget_hide(GTK_WIDGET(fs));

	if ((f = fopen(exportDialogData.fileName, "r")) != NULL) {
		fclose(f);
		if (dialog.dialog == NULL) {
			CreateYesNoDialog(&dialog, (GtkSignalFunc) YesLogoFileExportDialog, (GtkSignalFunc) CancelDialog,
					  GUI_LogosWindow);
			gtk_window_set_title(GTK_WINDOW(dialog.dialog), _("Overwrite file?"));
			g_snprintf(err, 255, _("File %s already exists.\nOverwrite?"),
				   exportDialogData.fileName);
			gtk_label_set_text(GTK_LABEL(dialog.text), err);
		}
		gtk_widget_show(dialog.dialog);
	} else
		ExportLogoFileMain(exportDialogData.fileName);
}

void ImportFileSelected(GtkWidget * w, GtkFileSelection * fs)
{
	gn_bmp tbitmap;
	gn_error error = 0;

	gchar *fileName;
	FILE *f;

	fileName = (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
	gtk_widget_hide(GTK_WIDGET(fs));

	if ((f = fopen(fileName, "r")) == NULL) {
		gchar *buf = g_strdup_printf(_("Can't open file %s for reading!\n"), fileName);
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
		return;
	} else
		fclose(f);

	error = gn_file_bitmap_read(fileName, &tbitmap, &statemachine->driver.phone);
	if (error != GN_ERR_NONE) {
		gchar *buf = g_strdup_printf(_("Error reading file\n%s"), gn_error_print(error));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);
		return;
	}

	exportDialogData.fileName = fileName;

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(buttonStartup), false);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(buttonOperator), false);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(buttonCaller), false);

	if (tbitmap.type == GN_BMP_NewOperatorLogo || tbitmap.type == GN_BMP_OperatorLogo)
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(buttonOperator), true);
	if (tbitmap.type == GN_BMP_StartupLogo)
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(buttonStartup), true);
	if (tbitmap.type == GN_BMP_CallerLogo)
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(buttonCaller), true);

	memcpy(&bitmap, &tbitmap, sizeof(gn_bmp));

	UpdatePoints(drawingArea);
}

void SaveLogoAs(GtkWidget * widget)
{
	FileSelection = gtk_file_selection_new(_("Save logo as ..."));

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->ok_button),
			   "clicked", (GtkSignalFunc) ExportFileSelected, FileSelection);

	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->cancel_button),
				  "clicked", (GtkSignalFunc) gtk_widget_destroy,
				  GTK_OBJECT(FileSelection));

	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(FileSelection));

	gtk_widget_show(FileSelection);
}

void SaveLogo(GtkWidget * widget)
{
	if (exportDialogData.fileName == NULL) {
		SaveLogoAs(widget);
	} else {
		ExportLogoFileMain(exportDialogData.fileName);
	}
}

void OpenLogo(GtkWidget * widget)
{
	FileSelection = gtk_file_selection_new(_("Open logo..."));

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->ok_button),
			   "clicked", (GtkSignalFunc) ImportFileSelected, FileSelection);

	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->cancel_button),
				  "clicked", (GtkSignalFunc) gtk_widget_destroy,
				  GTK_OBJECT(FileSelection));

	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(FileSelection));

	gtk_widget_show(FileSelection);
}

static GtkItemFactoryEntry logosMenuItems[] = {
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>O", OpenLogo, 0, NULL},
	{NULL, "<control>S", SaveLogo, 0, NULL},
	{NULL, NULL, SaveLogoAs, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>G", GetNetworkInfoEvent, 0, NULL},
	{NULL, NULL, GetLogoEvent, 0, NULL},
	{NULL, "<control>T", SetLogoEvent, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>W", CloseLogosWindow, 0, NULL},
	{NULL, NULL, NULL, 0, "<Branch>"},
	{NULL, "<control>C", ClearLogoEvent, 0, NULL},
	{NULL, "<control>I", InvertLogoEvent, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>U", UpLogoEvent, 0, NULL},
	{NULL, "<control>D", DownLogoEvent, 0, NULL},
	{NULL, "<control>L", LeftLogoEvent, 0, NULL},
	{NULL, "<control>R", RightLogoEvent, 0, NULL},
	{NULL, NULL, NULL, 0, "<Separator>"},
	{NULL, "<control>H", FlipHorizontalLogoEvent, 0, NULL},
	{NULL, "<control>V", FlipVerticalLogoEvent, 0, NULL},
};

void InitLogosMenu(void)
{
	logosMenuItems[0].path = _("/_File");
	logosMenuItems[1].path = _("/File/_Open");
	logosMenuItems[2].path = _("/File/_Save");
	logosMenuItems[3].path = _("/File/Save _as ...");
	logosMenuItems[4].path = _("/File/Sep1");
	logosMenuItems[5].path = _("/File/_Get operator");
	logosMenuItems[6].path = _("/File/Get _logo");
	logosMenuItems[7].path = _("/File/Se_t logo");
	logosMenuItems[8].path = _("/File/Sep2");
	logosMenuItems[9].path = _("/File/_Close");
	logosMenuItems[10].path = _("/_Edit");
	logosMenuItems[11].path = _("/Edit/_Clear");
	logosMenuItems[12].path = _("/Edit/_Invert");
	logosMenuItems[13].path = _("/Edit/Sep3");
	logosMenuItems[14].path = _("/Edit/_Up logo");
	logosMenuItems[15].path = _("/Edit/_Down logo");
	logosMenuItems[16].path = _("/Edit/_Left logo");
	logosMenuItems[17].path = _("/Edit/_Right logo");
	logosMenuItems[18].path = _("/Edit/Sep4");
	logosMenuItems[19].path = _("/Edit/Flip _horizontal");
	logosMenuItems[20].path = _("/Edit/Flip _vertical");
}

void GUI_CreateLogosWindow(void)
{

	int nMenuItems = sizeof(logosMenuItems) / sizeof(logosMenuItems[0]);
	GtkAccelGroup *accelGroup;
	GtkItemFactory *itemFactory;
	GtkWidget *menuBar;
	GtkWidget *toolBar, *vertToolBar;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *drawingBox;
	GtkWidget *separator;
	GdkBitmap *mask;

	GList *glistNetwork = NULL;

	int i = 0;
	gn_network network;

	previewPixmapWidth = PREVIEWWIDTH;
	previewPixmapHeight = PREVIEWHEIGHT;

	InitLogosMenu();

/* realize top level window for logos */
	GUI_LogosWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass(GTK_WINDOW(GUI_LogosWindow), "LogosWindow", "Xgnokii");
	gtk_window_set_policy(GTK_WINDOW(GUI_LogosWindow), 1, 1, 1);
	gtk_window_set_title(GTK_WINDOW(GUI_LogosWindow), _("Logos"));
	gtk_signal_connect(GTK_OBJECT(GUI_LogosWindow), "delete_event",
			   GTK_SIGNAL_FUNC(DeleteEvent), NULL);
	gtk_widget_realize(GUI_LogosWindow);

	CreateErrorDialog(&errorDialog, GUI_LogosWindow);
	CreateInfoDialog(&infoDialog, GUI_LogosWindow);

	accelGroup = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(GUI_LogosWindow), accelGroup);
	
/* create main vbox */
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(GUI_LogosWindow), vbox);
	gtk_widget_show(vbox);

	itemFactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accelGroup);
	gtk_item_factory_create_items(itemFactory, nMenuItems, logosMenuItems, NULL);
	menuBar = gtk_item_factory_get_widget(itemFactory, "<main>");

	gtk_box_pack_start(GTK_BOX(vbox), menuBar, FALSE, FALSE, 0);
	gtk_widget_show(menuBar);

/* toolbar */
	toolBar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolBar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolBar), GTK_ORIENTATION_HORIZONTAL);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolBar), NULL, _("Clear logo"), NULL,
				NewPixmap(New_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) ClearLogoEvent, toolBar);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolBar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolBar), NULL, _("Get logo"), NULL,
				NewPixmap(Read_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) GetLogoEvent, toolBar);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolBar), NULL, _("Set logo"), NULL,
				NewPixmap(Send_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) SetLogoEvent, toolBar);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolBar));

	gtk_toolbar_append_item(GTK_TOOLBAR(toolBar), NULL, _("Import from file"), NULL,
				NewPixmap(Open_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) OpenLogo, NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolBar), NULL, _("Export to file"), NULL,
				NewPixmap(Save_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) SaveLogo, NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolBar));

	buttonStartup = gtk_toolbar_append_element(GTK_TOOLBAR(toolBar),
						   GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL, NULL,
						   _("Startup logo"), "",
						   NewPixmap(Startup_logo_xpm,
							     GUI_LogosWindow->window,
							     &GUI_LogosWindow->style->
							     bg[GTK_STATE_NORMAL]),
						   GTK_SIGNAL_FUNC(LogoTypeEvent), NULL);

	buttonOperator = gtk_toolbar_append_element(GTK_TOOLBAR(toolBar),
						    GTK_TOOLBAR_CHILD_RADIOBUTTON, buttonStartup,
						    NULL, _("Operator logo"), "",
						    NewPixmap(Operator_logo_xpm,
							      GUI_LogosWindow->window,
							      &GUI_LogosWindow->style->
							      bg[GTK_STATE_NORMAL]),
						    GTK_SIGNAL_FUNC(LogoTypeEvent), NULL);

	buttonCaller = gtk_toolbar_append_element(GTK_TOOLBAR(toolBar),
						  GTK_TOOLBAR_CHILD_RADIOBUTTON,
						  buttonOperator,
						  NULL, _("Caller logo"),
						  "", NewPixmap(Caller_logo_xpm,
								GUI_LogosWindow->window,
								&GUI_LogosWindow->style->
								bg[GTK_STATE_NORMAL]),
						  GTK_SIGNAL_FUNC(LogoTypeEvent), NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolBar));

	networkCombo = gtk_combo_new();
	gtk_combo_set_use_arrows_always(GTK_COMBO(networkCombo), 1);

	while (gn_network_get(&network, i++)) {
		gchar *netcou;

		netcou = g_strdup_printf("%s (%s)", network.name, gn_network2country(network.code));
		glistNetwork = g_list_insert_sorted(glistNetwork,
						    netcou, (GCompareFunc) strcmp);
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(networkCombo), glistNetwork);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(networkCombo)->entry), FALSE);
	gtk_toolbar_append_widget(GTK_TOOLBAR(toolBar), networkCombo, "", "");
	gtk_widget_show(networkCombo);
	g_list_foreach(glistNetwork, (GFunc) g_free, NULL);
	g_list_free(glistNetwork);

	callerCombo = gtk_combo_new();
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(callerCombo)->entry), FALSE);
	gtk_toolbar_append_widget(GTK_TOOLBAR(toolBar), callerCombo, "", "");
	gtk_widget_show(callerCombo);

	gtk_box_pack_start(GTK_BOX(vbox), toolBar, FALSE, FALSE, 0);
	gtk_widget_show(toolBar);

	/* vertical separator */
	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(separator), FALSE, FALSE, 0);

	/* create horizontal box for preview and drawing areas */
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	/* set gn_bmp width,height needed for creating drawinArea
	 * we are starting, default is startupLogo
	 */
	bitmap.type = GN_BMP_StartupLogo;
	bitmap.height = 48;
	bitmap.width = 84;
	bitmap.size = bitmap.height * bitmap.width / 8;
	drawingAreaWidth = bitmap.width * (POINTSIZE + 1) + 1;
	drawingAreaHeight = bitmap.height * (POINTSIZE + 1) + 1;

	/* previewArea */
	previewPixmap = GetPreviewPixmap(GUI_LogosWindow);

	if (previewPixmap != NULL) {
		previewArea = gtk_drawing_area_new();
		gtk_drawing_area_size(GTK_DRAWING_AREA(previewArea),
				      previewPixmapWidth, previewPixmapHeight);

		greenPixelPixmap = gdk_pixmap_create_from_xpm_d(GUI_LogosWindow->window,
								&mask,
								&GUI_LogosWindow->style->
								bg[GTK_STATE_NORMAL],
								Green_pixel_xpm);

		gtk_signal_connect(GTK_OBJECT(previewArea), "expose_event",
				   (GtkSignalFunc) PreviewAreaExposeEvent, NULL);
		gtk_signal_connect(GTK_OBJECT(previewArea), "configure_event",
				   (GtkSignalFunc) PreviewAreaConfigureEvent, NULL);
		gtk_signal_connect(GTK_OBJECT(previewArea), "button_press_event",
				   (GtkSignalFunc) PreviewAreaButtonPressEvent, NULL);

		gtk_widget_set_events(previewArea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

		gtk_box_pack_start(GTK_BOX(hbox), previewArea, FALSE, FALSE, 0);
		gtk_widget_show(previewArea);

		/* clear battery, signal, menu & names from preview phone */
		UpdatePreviewPoints();

	} else
		previewAvailable = 0;

	/* drawingArea */
	greenPointPixmap = gdk_pixmap_create_from_xpm_d(GUI_LogosWindow->window,
							&mask,
							&GUI_LogosWindow->style->
							bg[GTK_STATE_NORMAL], Green_point_xpm);
	blackPointPixmap =
	    gdk_pixmap_create_from_xpm_d(GUI_LogosWindow->window, &mask,
					 &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL],
					 Black_point_xpm);

	drawingBox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), drawingBox, FALSE, FALSE, 0);
	gtk_widget_show(drawingBox);

	drawingArea = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(drawingArea), drawingAreaWidth, drawingAreaHeight);

	gtk_signal_connect(GTK_OBJECT(drawingArea), "configure_event",
			   (GtkSignalFunc) DrawingAreaConfigureEvent, NULL);
	gtk_signal_connect(GTK_OBJECT(drawingArea), "expose_event",
			   (GtkSignalFunc) DrawingAreaExposeEvent, NULL);
	gtk_signal_connect(GTK_OBJECT(drawingArea), "button_press_event",
			   (GtkSignalFunc) DrawingAreaButtonPressEvent, NULL);
	gtk_signal_connect(GTK_OBJECT(drawingArea), "button_release_event",
			   (GtkSignalFunc) DrawingAreaButtonReleaseEvent, NULL);
	gtk_signal_connect(GTK_OBJECT(drawingArea), "motion_notify_event",
			   (GtkSignalFunc) DrawingAreaMotionNotifyEvent, NULL);

	gtk_widget_set_events(drawingArea, GDK_EXPOSURE_MASK | GDK_LEAVE_NOTIFY_MASK |
			      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			      GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	gtk_box_pack_start(GTK_BOX(drawingBox), drawingArea, FALSE, FALSE, 0);
	gtk_widget_show(drawingArea);

	/* vertical tool bar */
	vertToolBar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(vertToolBar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(vertToolBar), GTK_ORIENTATION_VERTICAL);

	buttonBrush = gtk_toolbar_append_element(GTK_TOOLBAR(vertToolBar),
						 GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL, NULL,
						 _("Brush tool"), "", NewPixmap(Tool_brush_xpm,
										GUI_LogosWindow->
										window,
										&GUI_LogosWindow->
										style->
										bg
										[GTK_STATE_NORMAL]),
						 GTK_SIGNAL_FUNC(ToolTypeEvent), NULL);

	buttonLine = gtk_toolbar_append_element(GTK_TOOLBAR(vertToolBar),
						GTK_TOOLBAR_CHILD_RADIOBUTTON, buttonBrush, NULL,
						_("Line tool"), "", NewPixmap(Tool_line_xpm,
									      GUI_LogosWindow->
									      window,
									      &GUI_LogosWindow->
									      style->
									      bg[GTK_STATE_NORMAL]),
						GTK_SIGNAL_FUNC(ToolTypeEvent), NULL);

	buttonRectangle = gtk_toolbar_append_element(GTK_TOOLBAR(vertToolBar),
						     GTK_TOOLBAR_CHILD_RADIOBUTTON, buttonLine,
						     NULL, _("Rectangle tool"), "",
						     NewPixmap(Tool_rectangle_xpm,
							       GUI_LogosWindow->window,
							       &GUI_LogosWindow->style->
							       bg[GTK_STATE_NORMAL]),
						     GTK_SIGNAL_FUNC(ToolTypeEvent), NULL);

	buttonFilledRectangle = gtk_toolbar_append_element(GTK_TOOLBAR(vertToolBar),
							   GTK_TOOLBAR_CHILD_RADIOBUTTON,
							   buttonRectangle, NULL,
							   _("Filled rectangle tool"), "",
							   NewPixmap(Tool_filled_rectangle_xpm,
								     GUI_LogosWindow->window,
								     &GUI_LogosWindow->style->
								     bg[GTK_STATE_NORMAL]),
							   GTK_SIGNAL_FUNC(ToolTypeEvent), NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(vertToolBar));

	gtk_toolbar_append_item(GTK_TOOLBAR(vertToolBar), NULL, _("Invert logo"), NULL,
				NewPixmap(Edit_invert_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) InvertLogoEvent, vertToolBar);

	gtk_toolbar_append_item(GTK_TOOLBAR(vertToolBar), NULL, _("Horizontal flip"), NULL,
				NewPixmap(Edit_flip_horizontal_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) FlipHorizontalLogoEvent, vertToolBar);

	gtk_toolbar_append_item(GTK_TOOLBAR(vertToolBar), NULL, _("Vertical flip"), NULL,
				NewPixmap(Edit_flip_vertical_xpm, GUI_LogosWindow->window,
					  &GUI_LogosWindow->style->bg[GTK_STATE_NORMAL]),
				(GtkSignalFunc) FlipVerticalLogoEvent, vertToolBar);


	gtk_box_pack_start(GTK_BOX(hbox), vertToolBar, FALSE, FALSE, 0);
	gtk_widget_show(vertToolBar);

	GUIEventAdd(GUI_EVENT_CALLERS_GROUPS_CHANGED, &GUI_RefreshLogosGroupsCombo);
}

void GUI_RefreshLogosGroupsCombo(void)
{
	GList *callerList = NULL;
	int i;

	/* All groups + no group */
	for (i = 0; i < GN_PHONEBOOK_CALLER_GROUPS_MAX_NUMBER + 1; i++)
		callerList = g_list_insert(callerList, xgnokiiConfig.callerGroups[i], i);

	gtk_combo_set_popdown_strings(GTK_COMBO(callerCombo), callerList);
	g_list_free(callerList);

	if (!callersGroupsInitialized)
		callersGroupsInitialized = 1;
}

void GUI_ShowLogosWindow(void)
{
	/* Set network name taken from the phone */
	GetNetworkInfoEvent(NULL);
	/* if phone support caller groups, read callerGroups names */
	if (phoneMonitor.supported & PM_CALLERGROUP) {
		if (xgnokiiConfig.callerGroups[0] == NULL) {
			GUI_Refresh();
			GUI_InitCallerGroupsInf();
			GUI_RefreshLogosGroupsCombo();
		}
		if (!callersGroupsInitialized)
			GUI_RefreshLogosGroupsCombo();
		gtk_widget_show(buttonCaller);
		gtk_widget_show(callerCombo);
	} else {
		/* if not supported, hide widget for handling callerGroups */
		gtk_widget_hide(buttonCaller);
		gtk_widget_hide(callerCombo);
	}

	/* Call to reset Startup logo size */
	LogoTypeEvent(GUI_LogosWindow);
	gn_log_xdebug("width: %i, height: %i\n", bitmap.width, bitmap.height);
	gtk_window_present(GTK_WINDOW(GUI_LogosWindow));

	if (!previewAvailable && showPreviewErrorDialog) {
		gchar *buf = g_strdup(_("Load preview pixmap error, feature disabled."));
		gtk_label_set_text(GTK_LABEL(errorDialog.text), buf);
		gtk_widget_show(errorDialog.dialog);
		g_free(buf);

		showPreviewErrorDialog = 0;
	}
}
