/*
 * solidbrush.c
 * 
 * Copyright (c) 2003 Alexandre Pigolkine
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 * and associated documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial 
 * portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Authors:
 *   Alexandre Pigolkine(pigolkine@gmx.de)
 *   Ravindra (rkumar@novell.com)
 */

#include "gdip.h"
#include "solidbrush.h"

static GpStatus gdip_solidfill_setup (GpGraphics *graphics, GpBrush *brush);
static GpStatus gdip_solidfill_clone (GpBrush *brush, GpBrush **clonedBrush);
static GpStatus gdip_solidfill_destroy (GpBrush *brush);

/*
 * we have a single copy of vtable for
 * all instances of solidbrush.
 */

static BrushClass vtable = { BrushTypeSolidColor,
			     gdip_solidfill_setup, 
			     gdip_solidfill_clone,
			     gdip_solidfill_destroy };

void 
gdip_solidfill_init (GpSolidFill *brush)
{
	gdip_brush_init (&brush->base, &vtable);
	brush->color = 0;
}

GpSolidFill *
gdip_solidfill_new (void)
{
	GpSolidFill *result = (GpSolidFill *) GdipAlloc (sizeof (GpSolidFill));
	
	if (result)
		gdip_solidfill_init (result);

	return result;
}

GpStatus
gdip_solidfill_setup (GpGraphics *graphics, GpBrush *brush)
{
	GpSolidFill *solid;

	g_return_val_if_fail (graphics != NULL, InvalidParameter);
	g_return_val_if_fail (brush != NULL, InvalidParameter);

	solid = (GpSolidFill *) brush;
        int R = (solid->color & 0x00FF0000) >> 16;
        int G = (solid->color & 0x0000FF00) >> 8;
        int B = (solid->color & 0x000000FF);
        cairo_set_rgb_color (graphics->ct, (double) R / 255.0, (double) G / 255.0, (double) B / 255.0);

	return Ok;
}

GpStatus
gdip_solidfill_clone (GpBrush *brush, GpBrush **clonedBrush)
{
	GpSolidFill *result;
	GpSolidFill *solid;

	g_return_val_if_fail (brush != NULL, InvalidParameter);

	result = (GpSolidFill *) GdipAlloc (sizeof (GpSolidFill));

	g_return_val_if_fail (result != NULL, OutOfMemory);

	solid = (GpSolidFill *) brush;

	result->base = solid->base;
        result->color = solid->color;

	*clonedBrush = (GpBrush *) result;

	return Ok;
}

GpStatus
gdip_solidfill_destroy (GpBrush *brush)
{
	g_return_val_if_fail (brush != NULL, InvalidParameter);

	GdipFree (brush);

	return Ok;
}

GpStatus 
GdipCreateSolidFill (int color, GpSolidFill **brush)
{
	*brush = gdip_solidfill_new ();

	g_return_val_if_fail (*brush != NULL, OutOfMemory);

	(*brush)->color = color;
	return Ok;
}

GpStatus
GdipSetSolidFillColor (GpSolidFill *brush, int color)
{
	g_return_val_if_fail (brush != NULL, InvalidParameter);

        brush->color = color;
        return Ok;
}

GpStatus
GdipGetSolidFillColor (GpSolidFill *brush, int *color)
{
	g_return_val_if_fail (brush != NULL, InvalidParameter);

        *color = brush->color;
        return Ok;
}