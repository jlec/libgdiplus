/*
 * texturebrush.c
 *
 * Copyright (C) Novell, Inc. 2003-2004.
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
 *   Ravindra (rkumar@novell.com)
 *
 */

#include "gdip.h"
#include "gdipImage.h"
#include "texturebrush.h"

static GpStatus gdip_texture_setup (GpGraphics *graphics, GpBrush *brush);
static GpStatus gdip_texture_clone (GpBrush *brush, GpBrush **clonedBrush);
static GpStatus gdip_texture_destroy (GpBrush *brush);

/*
 * we have a single copy of vtable for
 * all instances of texturebrush.
 */

static BrushClass vtable = { BrushTypeTextureFill, 
			     gdip_texture_setup, 
			     gdip_texture_clone,
			     gdip_texture_destroy };

void 
gdip_texture_init (GpTexture *texture)
{
	gdip_brush_init (&texture->base, &vtable);
	texture->wrapMode = WrapModeTile;
	texture->rectangle = NULL;
	texture->pattern = NULL;
	GdipCreateMatrix (&texture->matrix);
}

GpTexture*
gdip_texture_new (void)
{
        GpTexture *result = (GpTexture *) GdipAlloc (sizeof (GpTexture));

	if (result)
	        gdip_texture_init (result);

        return result;
}

/* 
 * functions to create different wrapmodes.
 */
GpStatus
draw_tile_texture (cairo_t *ct, GpBitmap *bitmap, GpTexture *brush)
{
	cairo_surface_t *original;
	cairo_surface_t *texture;
	cairo_pattern_t *pat, *pattern;
	GpRect *rect = brush->rectangle;

	g_return_val_if_fail (rect != NULL, InvalidParameter);

	/* Original image surface */
	original = bitmap->image.surface;
	g_return_val_if_fail (original != NULL, InvalidParameter);

	/* Use the original as a pattern */
	pat = cairo_pattern_create_for_surface (original);
	g_return_val_if_fail (pat != NULL, OutOfMemory);
	cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);

	/* texture surface to be created */
	texture = cairo_surface_create_similar (original, from_cairoformat_to_content (bitmap->cairo_format),
						2 * rect->Width, 2 * rect->Height);
	g_return_val_if_fail (texture != NULL, OutOfMemory);

	{
		cairo_t *ct2;

		/* Draw the texture */
		ct2 = cairo_create (texture);
		cairo_identity_matrix (ct2);
		cairo_set_source (ct2, pat);
		cairo_rectangle (ct2, 0, 0, 2 * rect->Width, 2 * rect->Height);
		cairo_fill (ct2);
		cairo_destroy(ct2);
	}
	brush->pattern = cairo_pattern_create_for_surface (texture);
	cairo_pattern_set_extend (brush->pattern, CAIRO_EXTEND_REPEAT);

	cairo_pattern_destroy (pat);
	cairo_surface_destroy (texture);

	return gdip_get_status (cairo_status (ct));
}

GpStatus
draw_tile_flipX_texture (cairo_t *ct, GpBitmap *bitmap, GpTexture *brush)
{
	cairo_surface_t *original;
	cairo_surface_t *texture;
	cairo_pattern_t *pat, *pattern;
	cairo_matrix_t tempMatrix;
	GpRect *rect = brush->rectangle;
	g_return_val_if_fail (rect != NULL, InvalidParameter);

	/* Original image surface */
	original = bitmap->image.surface;
	g_return_val_if_fail (original != NULL, InvalidParameter);

	/* Use the original as a pattern */
	pat = cairo_pattern_create_for_surface (original);
	g_return_val_if_fail (pat != NULL, OutOfMemory);

	/* texture surface to be created */
	texture = cairo_surface_create_similar (original, from_cairoformat_to_content (bitmap->cairo_format),
						2 * rect->Width, rect->Height);

	if (texture == NULL) {
		return OutOfMemory;
	}

	{
		cairo_t	*ct2;

		/* Draw left part of the texture */
		ct2 = cairo_create (texture);
		cairo_set_source (ct2, pat);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);
		cairo_fill (ct2);
		
		/* Draw right part of the texture */
		cairo_translate (ct2, rect->Width, 0);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);

		/* Not sure if this is a bug, but using rect->Width - 1
		 * avoids the seam.
		 */
		cairo_matrix_translate (&tempMatrix, rect->Width - 1, 0);
		/* scale in -X direction to flip along X */
		cairo_matrix_scale (&tempMatrix, -1.0, 1.0);
		cairo_pattern_set_matrix (pat, &tempMatrix);
		cairo_fill (ct2);
		cairo_destroy(ct2);
	}

	brush->pattern = cairo_pattern_create_for_surface (texture);
	cairo_pattern_set_extend (brush->pattern, CAIRO_EXTEND_REPEAT);

	cairo_pattern_destroy (pat);
	cairo_surface_destroy (texture);

	return gdip_get_status (cairo_status (ct));
}

GpStatus
draw_tile_flipY_texture (cairo_t *ct, GpBitmap *bitmap, GpTexture *brush)
{
	cairo_surface_t *original;
	cairo_surface_t *texture;
	cairo_pattern_t *pat, *pattern;
	GpMatrix tempMatrix;
	GpRect *rect = brush->rectangle;
	g_return_val_if_fail (rect != NULL, InvalidParameter);

	/* Original image surface */
	original = bitmap->image.surface;
	g_return_val_if_fail (original != NULL, InvalidParameter);

	/* Use the original as a pattern */
	pat = cairo_pattern_create_for_surface (original);
	g_return_val_if_fail (pat != NULL, OutOfMemory);

	/* texture surface to be created */
	texture = cairo_surface_create_similar (original, from_cairoformat_to_content (bitmap->cairo_format),
						rect->Width, 2 * rect->Height);

	if (texture == NULL) {
		return OutOfMemory;
	}

	{
		cairo_t	*ct2;

		/* Draw upper part of the texture */
		ct2 = cairo_create (texture);
		cairo_set_source (ct2, pat);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);
		cairo_fill (ct2);
		
		/* Draw lower part of the texture */
		cairo_translate (ct2, 0, rect->Height);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);

		/* Not sure if this is a bug, but using rect->Height - 1
		 * avoids the seam.
		 */
		cairo_matrix_translate (&tempMatrix, 0, rect->Height - 1);
		/* scale in -Y direction to flip along Y */
		cairo_matrix_scale (&tempMatrix, 1.0, -1.0);
		cairo_pattern_set_matrix (pat, &tempMatrix);
		cairo_fill (ct2);
		cairo_destroy(ct2);
	}

	brush->pattern = cairo_pattern_create_for_surface (texture);
	cairo_pattern_set_extend (brush->pattern, CAIRO_EXTEND_REPEAT);	

	cairo_pattern_destroy (pat);
	cairo_surface_destroy (texture);

	return gdip_get_status (cairo_status (ct));
}

GpStatus
draw_tile_flipXY_texture (cairo_t *ct, GpBitmap *bitmap, GpTexture *brush)
{
	cairo_surface_t *original;
	cairo_surface_t *texture;
	cairo_pattern_t *pat, *pattern;
	GpMatrix tempMatrix;
	GpRect *rect = brush->rectangle;
	g_return_val_if_fail (rect != NULL, InvalidParameter);

	/* Original image surface */
	original = bitmap->image.surface;
	g_return_val_if_fail (original != NULL, InvalidParameter);

	/* Use the original as a pattern */
	pat = cairo_pattern_create_for_surface (original);
	g_return_val_if_fail (pat != NULL, OutOfMemory);

	/* texture surface to be created */
	texture = cairo_surface_create_similar (original, from_cairoformat_to_content (bitmap->cairo_format),
						2 * rect->Width, 2 * rect->Height);

	if (texture == NULL) {
		return OutOfMemory;
	}

	{
		cairo_t *ct2;

		ct2 = cairo_create (texture);
		cairo_set_source (ct2, pat);

		/* Draw upper left part of the texture */
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);
		cairo_fill (ct2);

		/* Draw lower left part of the texture */
		cairo_translate (ct2, 0, rect->Height);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);
		/* Not sure if this is a bug, but using rect->Height - 1
		 * avoids the seam.
		 */
		cairo_matrix_translate (&tempMatrix, 0, rect->Height - 1);
		/* scale in -Y direction to flip along Y */
		cairo_matrix_scale (&tempMatrix, 1.0, -1.0);
		cairo_pattern_set_matrix (pat, &tempMatrix);
		cairo_fill (ct2);

		/* Draw upper right part of the texture */
		cairo_translate (ct2, rect->Width, -rect->Height);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);
		/* Reset the pattern matrix and do fresh transformation */
		cairo_matrix_init_identity (&tempMatrix);
		/* Not sure if this is a bug, but using rect->Width - 1
		 * avoids the seam.
		 */
		cairo_matrix_translate (&tempMatrix, rect->Width - 1, 0);
		/* scale in -X direction to flip along X */
		cairo_matrix_scale (&tempMatrix, -1.0, 1.0);
		cairo_pattern_set_matrix (pat, &tempMatrix);
		cairo_fill (ct2);

		/* Draw lower right part of the texture */
		cairo_translate (ct2, 0, rect->Height);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);
		/* Not sure if this is a bug, but using rect->Height - 1
		 * avoids the seam.
		 */
		cairo_matrix_translate (&tempMatrix, 0, rect->Height - 1);
		/* scale in -Y direction to flip along Y */
		cairo_matrix_scale (&tempMatrix, 1.0, -1.0);
		cairo_pattern_set_matrix (pat, &tempMatrix);
		cairo_fill (ct2);
		cairo_destroy(ct2);
	}

	brush->pattern = cairo_pattern_create_for_surface (texture);
	cairo_pattern_set_extend (brush->pattern, CAIRO_EXTEND_REPEAT);

	cairo_pattern_destroy (pat);
	cairo_surface_destroy (texture);

	return gdip_get_status (cairo_status (ct));
}

GpStatus
draw_clamp_texture (cairo_t *ct, GpBitmap *bitmap, GpTexture *brush)
{
	cairo_surface_t *original;
	cairo_surface_t *texture;
	cairo_pattern_t *pat;
	GpRect *rect = brush->rectangle;
	g_return_val_if_fail (rect != NULL, InvalidParameter);

	/* Original image surface */
	original = bitmap->image.surface;
	g_return_val_if_fail (original != NULL, InvalidParameter);

	/* Use the original as a pattern */
	pat = cairo_pattern_create_for_surface (original);
	g_return_val_if_fail (pat != NULL, OutOfMemory);
	cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);

	/* texture surface to be created */
	texture = cairo_surface_create_similar (original, from_cairoformat_to_content (bitmap->cairo_format),
						rect->Width, rect->Height);

	g_return_val_if_fail (texture != NULL, OutOfMemory);

	{
		cairo_t	*ct2;
		/* Draw the texture */
		ct2 = cairo_create (texture);
		cairo_identity_matrix (ct2);
		cairo_set_source (ct2, pat);
		cairo_rectangle (ct2, 0, 0, rect->Width, rect->Height);
		cairo_fill (ct2);
		cairo_destroy(ct2);
	}

	brush->pattern = cairo_pattern_create_for_surface (texture);

	cairo_pattern_destroy (pat);
	cairo_surface_destroy (texture);

	return gdip_get_status (cairo_status (ct));
}

GpStatus
gdip_texture_setup (GpGraphics *graphics, GpBrush *brush)
{
	cairo_t *ct;
	cairo_pattern_t *pattern;
	GpTexture *texture;
	GpImage *img, *gr_img;
	GpBitmap *bmp, *gr_bmp;
	cairo_format_t format;
	unsigned int width;
	unsigned int height;
	GpStatus status = Ok;
	BOOL dispose_bitmap;

	g_return_val_if_fail (graphics != NULL, InvalidParameter);
	g_return_val_if_fail (brush != NULL, InvalidParameter);

	texture = (GpTexture *) brush;
	img = texture->image;
	g_return_val_if_fail (img != NULL, InvalidParameter);
	if (img->type == imageBitmap) {
		bmp = (GpBitmap *) img;
		if (gdip_is_an_indexed_pixelformat (bmp->data.PixelFormat)) {
			/* Unable to create a surface for the bitmap; it is an indexed image.
			* Instead, it will first be converted to 32-bit RGB.
			*/
			bmp = gdip_convert_indexed_to_rgb (bmp);
			g_return_val_if_fail (bmp != NULL, OutOfMemory);
			gdip_bitmap_ensure_surface (bmp);
			dispose_bitmap = TRUE;
		} else {
			dispose_bitmap = FALSE;
		}
		width = bmp->data.Width;
		height = bmp->data.Height;
		format = bmp->data.PixelFormat;
	} else {
		return NotImplemented;
	}

	ct = graphics->ct;
	g_return_val_if_fail (ct != NULL, InvalidParameter);

	/* We create the new pattern for brush, if the brush is changed
	 * or if pattern has not been created yet.
	 */
	if (texture->base.changed || (texture->pattern) == NULL) {
		/* destroy the existing pattern */
		if (texture->pattern)
			cairo_pattern_destroy (texture->pattern);

		switch (texture->wrapMode) {

		case WrapModeTile:
			status = draw_tile_texture (ct, bmp, texture);
			break;

		case WrapModeTileFlipX:
			status = draw_tile_flipX_texture (ct, bmp, texture);
			break;

		case WrapModeTileFlipY:
			status = draw_tile_flipY_texture (ct, bmp, texture);
			break;

		case WrapModeTileFlipXY:
			status = draw_tile_flipXY_texture (ct, bmp, texture);
			break;

		case WrapModeClamp:
			status = draw_clamp_texture (ct, bmp, texture);
			break;

		default:
			status = InvalidParameter;
		}
	}

	if (dispose_bitmap) {
		GdipDisposeImage((GpImage *)bmp);
	}

	if (status == Ok) {
		if (texture->pattern == NULL)
			status = GenericError;
		else {
			GpMatrix *product = NULL;
			GdipCreateMatrix (&product);
		/* Cairo Bug: REPEAT and transformation do not go together.        */
		/* To work around this we can create an intermediate surface first */
		/* with REPEAT and use that with transformation as a pattern. But, */
		/* that gets another problem of handling rotation. Since, we need  */
		/* absolute rotation instead of rotation around 0,0.               */
		/* There are two ways to get around this problem:                  */
		/* 1. Uncommenting the following commented code once we know a     */
		/* better way to handle the rotation axis problem.      OR         */
		/* 2. Cairo bug gets fixed. Following commented code won't be      */
		/* needed in that case.                                            */
	/*
		cairo_save (ct);
		{
			temp = cairo_surface_create_similar (cairo_get_target (ct),
							     format, width, height);
			
			if (temp == NULL) {
				cairo_matrix_destroy (product);
				return OutOfMemory;
			}
			
			cairo_set_target_surface (ct, temp);
			cairo_set_source (ct, texture->pattern);
			cairo_rectangle (ct, 0, 0, width, height);
			cairo_fill (ct);
		}
		cairo_restore (ct);
	
		pattern = cairo_pattern_create_for_surface (temp);
	*/
			pattern = texture->pattern;

			if (pattern != NULL) {
				/* Use both the matrices */
				cairo_matrix_multiply (product, texture->matrix, graphics->copy_of_ctm);
				cairo_matrix_invert (product);
				cairo_pattern_set_matrix (pattern, product);
				cairo_set_source (ct, pattern);
				/*	cairo_pattern_destroy (pattern); */
			}
			status = gdip_get_status (cairo_status (ct));
			/*	cairo_surface_destroy (temp); */
			GdipDeleteMatrix (product);
		}
	}

	return status;
}

GpStatus
gdip_texture_clone (GpBrush *brush, GpBrush **clonedBrush)
{
	GpTexture *result;
	GpTexture *texture;

	g_return_val_if_fail (brush != NULL, InvalidParameter);

	result = (GpTexture *) GdipAlloc (sizeof (GpTexture));

	g_return_val_if_fail (result != NULL, OutOfMemory);

	texture = (GpTexture *) brush;
	result->base = texture->base;
	result->wrapMode = texture->wrapMode;

	/* Let the clone create its own pattern. */
	result->pattern = NULL;
	result->base.changed = TRUE;

	gdip_cairo_matrix_copy (&result->matrix, &texture->matrix);
	result->rectangle = (GpRect *) GdipAlloc (sizeof (GpRect));

	if (result->rectangle == NULL) {
		GdipFree (result);
		return OutOfMemory;
	}

	memcpy (result->rectangle, texture->rectangle, sizeof (GpRect));

	result->image = texture->image;
	cairo_surface_reference (result->image->surface);

	*clonedBrush = (GpBrush *) result;

	return Ok;
}

GpStatus
gdip_texture_destroy (GpBrush *brush)
{
	GpTexture *texture;

	g_return_val_if_fail (brush != NULL, InvalidParameter);

	texture = (GpTexture *) brush;

	if (texture->rectangle) {
		GdipFree (texture->rectangle);
		texture->rectangle = NULL;
	}

	if (texture->pattern) {
		cairo_pattern_destroy (texture->pattern);
		texture->pattern = NULL;
	}

	if (texture->image && texture->image->surface) {
		cairo_surface_destroy (texture->image->surface);
		texture->image->surface = NULL;
	}

	if (texture->matrix) {
		GdipDeleteMatrix (texture->matrix);
		texture->matrix = NULL;
	}

	GdipFree (texture);

	return Ok;
}

GpStatus
GdipCreateTexture (GpImage *image, GpWrapMode wrapMode, GpTexture **texture)
{
	GpBitmap *bitmap;
	cairo_surface_t *imageSurface;

	g_return_val_if_fail (image != NULL, InvalidParameter);

	if (image->type == imageBitmap)
		bitmap = (GpBitmap *) image;
	else
		return NotImplemented;

	imageSurface = cairo_image_surface_create_for_data ((unsigned char *)bitmap->data.Scan0,
					bitmap->cairo_format, bitmap->data.Width,
					bitmap->data.Height, bitmap->data.Stride);

	g_return_val_if_fail (imageSurface != NULL, OutOfMemory);

	/* texture surface to be used by brush */
	image->surface = imageSurface;

	*texture = gdip_texture_new ();

	if (*texture == NULL) {
		cairo_surface_destroy (imageSurface);
		return OutOfMemory;
	}

	(*texture)->wrapMode = wrapMode;
	(*texture)->image = image;
	(*texture)->rectangle = (GpRect *) GdipAlloc (sizeof (GpRect));

	if ( (*texture)->rectangle == NULL) {
		cairo_surface_destroy (imageSurface);
		GdipFree (*texture);
		return OutOfMemory;
	}

	(*texture)->rectangle->X = 0;
	(*texture)->rectangle->Y = 0;
	(*texture)->rectangle->Width = bitmap->data.Width;
	(*texture)->rectangle->Height = bitmap->data.Height;

	return Ok;
}

GpStatus
GdipCreateTexture2 (GpImage *image, GpWrapMode wrapMode, float x, float y, float width, float height, GpTexture **texture)
{
	return GdipCreateTexture2I (image, wrapMode, (int) x, (int) y, (int) width, (int) height, texture);
}

GpStatus
GdipCreateTexture2I (GpImage *image, GpWrapMode wrapMode, int x, int y, int width, int height, GpTexture **texture)
{
	int bmpWidth;
	int bmpHeight;
	GpBitmap *bitmap;
	cairo_t *ct;
	cairo_surface_t *original, *new;

	g_return_val_if_fail (image != NULL, InvalidParameter);

	if (image->type == imageBitmap)
		bitmap = (GpBitmap *) image;
	else
		return NotImplemented;

	bmpWidth = bitmap->data.Width;
	bmpHeight = bitmap->data.Height;

	/* MS behaves this way */
	if (x < 0 || y < 0 || width < 0 || height < 0 
		  || (bmpWidth < (x + width)) || (bmpHeight < (y + height)))
	     return OutOfMemory;

	original = cairo_image_surface_create_for_data ((unsigned char *)bitmap->data.Scan0, 
			bitmap->cairo_format, x + width, y + height, bitmap->data.Stride);

	g_return_val_if_fail (original != NULL, OutOfMemory);

	/* texture surface to be used by brush */
	new = cairo_surface_create_similar (original, from_cairoformat_to_content (bitmap->cairo_format), width, height);

	if (new == NULL) {
		cairo_surface_destroy (original);
		return OutOfMemory;
	}

	/* clip the rectangle from original image surface and create new surface */
	ct = cairo_create (new);
	cairo_translate (ct, -x, -y);
	cairo_set_source_surface (ct, original, x + width, y + height);
	cairo_paint (ct);
	cairo_destroy (ct);
	cairo_surface_destroy (original);

	image->surface = new;

	*texture = gdip_texture_new ();

	if (*texture == NULL) {
		cairo_surface_destroy (new);
		return OutOfMemory;
	}

	(*texture)->wrapMode = wrapMode;
	(*texture)->image = image;
	(*texture)->rectangle = (GpRect *) GdipAlloc (sizeof (GpRect));

	if ( (*texture)->rectangle == NULL) {
		cairo_surface_destroy (new);
		GdipFree (*texture);
		return OutOfMemory;
	}

	(*texture)->rectangle->X = x;
	(*texture)->rectangle->Y = y;
	(*texture)->rectangle->Width = width;
	(*texture)->rectangle->Height = height;

	return Ok;
}

GpStatus
GdipCreateTextureIA (GpImage *image, GpImageAttributes *imageAttributes, float x, float y, float width, float height, GpTexture **texture)
{
	/* MonoTODO: Make use of ImageAttributes parameter when
	 * ImageAttributes is implemented
	 */
	return GdipCreateTexture2 (image, WrapModeTile, x, y, width, height, texture);
}

GpStatus
GdipCreateTextureIAI (GpImage *image, GpImageAttributes *imageAttributes, int x, int y, int width, int height, GpTexture **texture)
{
	/* MonoTODO: Make use of ImageAttributes parameter when
	 * ImageAttributes is implemented
	 */
	return GdipCreateTexture2I (image, WrapModeTile, x, y, width, height, texture);
}

GpStatus
GdipGetTextureTransform (GpTexture *texture, GpMatrix *matrix)
{
	g_return_val_if_fail (texture != NULL, InvalidParameter);
	g_return_val_if_fail (matrix != NULL, InvalidParameter);

	gdip_cairo_matrix_copy(matrix, &texture->matrix);
	return Ok;
}

GpStatus
GdipSetTextureTransform (GpTexture *texture, GDIPCONST GpMatrix *matrix)
{
	g_return_val_if_fail (texture != NULL, InvalidParameter);
	g_return_val_if_fail (matrix != NULL, InvalidParameter);

	gdip_cairo_matrix_copy(&texture->matrix, matrix);
	texture->base.changed = TRUE;

	return Ok;
}

GpStatus
GdipResetTextureTransform (GpTexture *texture)
{
	g_return_val_if_fail (texture != NULL, InvalidParameter);

	 cairo_matrix_init_identity (texture->matrix);
	texture->base.changed = TRUE;
	return Ok;
}

GpStatus
GdipMultiplyTextureTransform (GpTexture *texture, GpMatrix *matrix, GpMatrixOrder order)
{
	GpStatus status;
	g_return_val_if_fail (texture != NULL, InvalidParameter);
	g_return_val_if_fail (matrix != NULL, InvalidParameter);

	/* FIXME: How to take care of rotation here ? */
	status = GdipMultiplyMatrix (texture->matrix, matrix, order);
	if (status == Ok)
		texture->base.changed = TRUE;
	return status;
}

GpStatus
GdipTranslateTextureTransform (GpTexture *texture, float dx, float dy, GpMatrixOrder order)
{
	GpStatus status;
	g_return_val_if_fail (texture != NULL, InvalidParameter);

	status = GdipTranslateMatrix (texture->matrix, dx, dy, order);
	if (status == Ok)
		texture->base.changed = TRUE;
	return status;
}

GpStatus
GdipScaleTextureTransform (GpTexture *texture, float sx, float sy, GpMatrixOrder order)
{
	GpStatus status;
	g_return_val_if_fail (texture != NULL, InvalidParameter);

	status = GdipScaleMatrix (texture->matrix, sx, sy, order);
	if (status == Ok)
		texture->base.changed = TRUE;
	return status;
}

GpStatus
GdipRotateTextureTransform (GpTexture *texture, float angle, GpMatrixOrder order)
{
	GpStatus status;
	GpPointF axis;

	g_return_val_if_fail (texture != NULL, InvalidParameter);

	/* Cairo uses origin (0,0) as the axis of rotation. However, we need 
	 * to do absolute rotation. Following approach for shifting the axis
	 * of rotation was suggested by Carl.
	 */

	/* Our pattern size is 2*(texture->rect) hence its centre is following */
	axis.X = texture->rectangle->Width;
	axis.Y = texture->rectangle->Height;

	if ((status = GdipTranslateMatrix (texture->matrix, -axis.X, -axis.Y, order)) == Ok)
		if ((status = GdipRotateMatrix (texture->matrix, angle, order)) == Ok)
			if ((status = GdipTranslateMatrix (texture->matrix, axis.X, axis.Y, order)) == Ok)
				texture->base.changed = TRUE;
	return status;
}

GpStatus
GdipSetTextureWrapMode (GpTexture *texture, GpWrapMode wrapMode)
{
	g_return_val_if_fail (texture != NULL, InvalidParameter);

	texture->wrapMode = wrapMode;
	texture->base.changed = TRUE;

	return Ok;
}

GpStatus
GdipGetTextureWrapMode (GpTexture *texture, GpWrapMode *wrapMode)
{
	g_return_val_if_fail (texture != NULL, InvalidParameter);

	*wrapMode = texture->wrapMode;
	return Ok;
}

GpStatus
GdipGetTextureImage (GpTexture *texture, GpImage **image)
{
	g_return_val_if_fail (texture != NULL, InvalidParameter);
	g_return_val_if_fail (image != NULL, InvalidParameter);

	*image = texture->image;
	return Ok;
}
