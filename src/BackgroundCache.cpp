#include "global.h"

#include "arch/arch.h"
#include "RageDisplay.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageSurface_Load.h"
#include "BackgroundCache.h"
#include "Sprite.h"
#include "PrefsManager.h"
#include "SDL_dither.h"
#include "SDL_rotozoom.h"
#include "RageDisplay.h"
#include "RageTexture.h"
#include "RageTextureManager.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurfaceUtils_Palettize.h"

#include "BackgroundSSM.h"

#define CACHE_DIR "Cache/"
#define BACKGROUND_CACHE_INDEX CACHE_DIR "backgrounds.cache"


/* Call CacheBackground to cache a background by path.  If the background is already
 * cached, it'll be recreated.  This is efficient if the background hasn't changed,
 * but we still only do this in TidyUpData for songs.
 *
 * Call LoadBackground to load a cached background into main memory.  This will call
 * CacheBackground only if needed.  This will not do a date/size check; call CacheBackground
 * directly if you need that.
 *
 * Call LoadCachedBackground to load a background into a texture and retrieve an ID
 * for it.  You can check if the background was actually preloaded by calling
 * TEXTUREMAN->IsTextureRegistered() on the ID; it might not be if the background cache
 * is missing or disabled.
 *
 * Note that each cache entries has two hashes.  The cache path is based soley
 * on the pathname; this way, loading the cache doesn't have to do a stat on every
 * background.  The full hash includes the file size and date, and is used only by
 * CacheBackground to avoid doing extra work.
 */

BackgroundCache *BACKGROUNDCACHE;


static map<CString,RageSurface *> g_BackgroundPathToImage;

CString BackgroundCache::GetBackgroundCachePath( CString BackgroundPath )
{
	/* Use GetHashForString, not ForFile, since we don't want to spend time
	 * checking the file size and date. */
	return ssprintf( CACHE_DIR "Backgrounds/%u", GetHashForString(BackgroundPath) );
}

void BackgroundCache::LoadBackground( CString BackgroundPath )
{
	if( PREFSMAN->m_BackgroundCache != PrefsManager::BGCACHE_LOW_RES || BackgroundPath == "" )
		return;

	/* Load it. */
	const CString CachePath = GetBackgroundCachePath(BackgroundPath);

	for( int tries = 0; tries < 2; ++tries )
	{
		if( g_BackgroundPathToImage.find(BackgroundPath) != g_BackgroundPathToImage.end() )
			return; /* already loaded */

		CHECKPOINT_M( ssprintf( "BackgroundCache::LoadBackground: %s", CachePath.c_str() ) );
		RageSurface *img = RageSurfaceUtils::LoadSurface( CachePath );
		if( img == NULL )
		{
			if(tries == 0)
			{
				/* The file doesn't exist.  It's possible that the background cache file is
				 * missing, so try to create it.  Don't do this first, for efficiency. */
				LOG->Trace( "Cached background load of '%s' ('%s') failed, trying to cache ...", BackgroundPath.c_str(), CachePath.c_str() );
				/* Skip the up-to-date check; it failed to load, so it can't be up
				 * to date. */
				CacheBackgroundInternal( BackgroundPath );
				continue;
			}
			else
			{
				LOG->Trace( "Cached background load of '%s' ('%s') failed", BackgroundPath.c_str(), CachePath.c_str() );
				return;
			}
		}

		g_BackgroundPathToImage[BackgroundPath] = img;
	}
}

void BackgroundCache::OutputStats() const
{
	map<CString,RageSurface *>::const_iterator ban;
	int total_size = 0;
	for( ban = g_BackgroundPathToImage.begin(); ban != g_BackgroundPathToImage.end(); ++ban )
	{
		RageSurface * const &img = ban->second;
		const int size = img->pitch * img->h;
		total_size += size;
	}
	LOG->Info( "%i bytes of backgrounds loaded", total_size );
}

void BackgroundCache::UnloadAllBackgrounds()
{
	map<CString,RageSurface *>::iterator it;
	for( it = g_BackgroundPathToImage.begin(); it != g_BackgroundPathToImage.end(); ++it )
		delete it->second;

	g_BackgroundPathToImage.clear();
}

BackgroundCache::BackgroundCache()
{
	BackgroundData.ReadFile( BACKGROUND_CACHE_INDEX );	// don't care if this fails
}

BackgroundCache::~BackgroundCache()
{
	UnloadAllBackgrounds();
}

struct BackgroundSSMTexture: public RageTexture
{
	unsigned m_uTexHandle;
	unsigned GetTexHandle() const { return m_uTexHandle; };	// accessed by RageDisplay
	/* This is a reference to a pointer in g_BackgroundPathToImage. */
	RageSurface *&img;
	int width, height;

	BackgroundSSMTexture( RageTextureID name, RageSurface *&img_, int width_, int height_ ):
		RageTexture(name), img(img_), width(width_), height(height_)
	{
		Create();
	}

	~BackgroundSSMTexture()
	{ 
		Destroy();
	}
	
	void Create()
	{
		ASSERT( img );

		/* The image is preprocessed; do as little work as possible. */

		/* The source width is the width of the original file. */
		m_iSourceWidth = width;
		m_iSourceHeight = height;

		/* The image width (within the texture) is always the entire texture. 
		 * Only resize if the max texture size requires it; since these images
		 * are already scaled down, this shouldn't happen often. */
		if( img->w > DISPLAY->GetMaxTextureSize() || 
			img->h > DISPLAY->GetMaxTextureSize() )
		{
			LOG->Warn("Converted %s at runtime", GetID().filename.c_str() );
			int width = min( img->w, DISPLAY->GetMaxTextureSize() );
			int height = min( img->h, DISPLAY->GetMaxTextureSize() );
			RageSurfaceUtils::Zoom( img, width, height );
		}

		/* We did this when we cached it. */
		ASSERT( img->w == power_of_two(img->w) );
		ASSERT( img->h == power_of_two(img->h) );

		m_iTextureWidth = m_iImageWidth = img->w;
		m_iTextureHeight = m_iImageHeight = img->h;

		/* Find a supported texture format.  If it happens to match the stored
		 * file, we won't have to do any conversion here, and that'll happen often
		 * with paletted images. */
		RageDisplay::PixelFormat pf = img->format->BitsPerPixel == 8? RageDisplay::FMT_PAL: RageDisplay::FMT_RGB5A1;
		if( !DISPLAY->SupportsTextureFormat(pf) )
			pf = RageDisplay::FMT_RGBA4;
		ASSERT( DISPLAY->SupportsTextureFormat(pf) );

		ASSERT(img);
		m_uTexHandle = DISPLAY->CreateTexture( pf, img, false );

		CreateFrameRects();
	}

	void Destroy()
	{
		if( m_uTexHandle )
			DISPLAY->DeleteTexture( m_uTexHandle );
		m_uTexHandle = 0;
	}

	void Reload()
	{
		Destroy();
		Create();
	}

	void Invalidate()
	{
		m_uTexHandle = 0; /* don't Destroy() */
	}
};

RageTextureID BackgroundCache::LoadCachedBackground( CString BackgroundPath )
{
	RageTextureID ID( GetBackgroundCachePath(BackgroundPath) );

	if( BackgroundPath == "" )
		return ID;

	LOG->Trace("BackgroundCache::LoadCachedBackground(%s): %s", BackgroundPath.c_str(), ID.filename.c_str() );

	/* Hack: make sure Background::Load doesn't change our return value and end up
	 * reloading. */
	ID = Sprite::SongBackgroundSSMTexture(ID);

	/* It's not in a texture.  Do we have it loaded? */
	if( g_BackgroundPathToImage.find(BackgroundPath) == g_BackgroundPathToImage.end() )
	{
		/* Oops, the image is missing.  Warn and continue. */
		LOG->Warn( "Background cache for '%s' wasn't loaded", BackgroundPath.c_str() );
		return ID;
	}

	/* This is a reference to a pointer.  BackgroundSSMTexture's ctor may change it
	 * when converting; this way, the conversion will end up in the map so we
	 * only have to convert once. */
	RageSurface *&img = g_BackgroundPathToImage[BackgroundPath];
	ASSERT( img );

	int src_width = 0, src_height = 0;
	bool WasRotatedBackground = false;
	BackgroundData.GetValue( BackgroundPath, "Width", src_width );
	BackgroundData.GetValue( BackgroundPath, "Height", src_height );
	BackgroundData.GetValue( BackgroundPath, "Rotated", WasRotatedBackground );
	if(src_width == 0 || src_height == 0)
	{
		LOG->Warn("Couldn't load '%s'", BackgroundPath.c_str() );
		return ID;
	}

	if( WasRotatedBackground )
	{
		/* We need to tell Sprite that this was originally a rotated
		 * sprite. */
		ID.filename += "(was rotated)";
	}

	/* Is the background already in a texture? */
	if( TEXTUREMAN->IsTextureRegistered(ID) )
		return ID; /* It's all set. */

	LOG->Trace("Loading background texture %s; src %ix%i; image %ix%i",
		ID.filename.c_str(), src_width, src_height, img->w, img->h );
	RageTexture *pTexture = new BackgroundSSMTexture( ID, img, src_width, src_height );

	ID.Policy = RageTextureID::TEX_VOLATILE;
	TEXTUREMAN->RegisterTexture( ID, pTexture );
	TEXTUREMAN->UnloadTexture( pTexture );

	return ID;
}

static inline int closest( int num, int n1, int n2 )
{
	if( abs(num - n1) > abs(num - n2) )
		return n2;
	return n1;
}

/* We write the cache even if we won't use it, so we don't have to recache everything
 * if the memory or settings change. */
void BackgroundCache::CacheBackground( CString BackgroundPath )
{
	if( PREFSMAN->m_BackgroundCache != PrefsManager::BGCACHE_LOW_RES )
		return;

	CHECKPOINT_M( BackgroundPath );
	if( !DoesFileExist(BackgroundPath) )
		return;

	const CString CachePath = GetBackgroundCachePath(BackgroundPath);

	/* Check the full file hash.  If it's the loaded and identical, don't recache. */
	if( DoesFileExist(CachePath) )
	{
		unsigned CurFullHash;
		const unsigned FullHash = GetHashForFile( BackgroundPath );
		if( BackgroundData.GetValue( BackgroundPath, "FullHash", CurFullHash ) &&
			CurFullHash == FullHash )
		{
			/* It's identical.  Just load it. */
			LoadBackground( BackgroundPath );
			return;
		}
	}

	CacheBackgroundInternal( BackgroundPath );
}

void BackgroundCache::CacheBackgroundInternal( CString BackgroundPath )
{
	CString error;
	RageSurface *img = RageSurfaceUtils::LoadFile( BackgroundPath, error );
	if( img == NULL )
	{
		LOG->Warn( "BackgroundCache::CacheBackground: Couldn't load %s: %s", BackgroundPath.c_str(), error.c_str() );
		return;
	}

	bool WasRotatedBackground = false;

	if( Sprite::IsDiagonalBackground(img->w , img->h) )
	{
		/* Ack.  It's a diagonal background.  Problem: if we resize a diagonal background, we
		 * get ugly checker patterns.  We need to un-rotate it.
		 *
		 * If we spin the background by hand, we need to do a linear filter, or the
		 * fade to the full resolution background is misaligned, which looks strange.
		 *
		 * To do a linear filter, we need to lose the palette.  Oh well.
		 *
		 * This also makes the background take less memory, though that could also be
		 * done by RLEing the surface.
		 */
		RageSurfaceUtils::ApplyHotPinkColorKey( img );

		RageSurfaceUtils::ConvertSurface(img, img->w, img->h, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
		
		RageSurface *dst = CreateSurface(
            256, 64, img->format->BitsPerPixel, 
			img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask );

		if( img->format->BitsPerPixel == 8 ) 
		{
			ASSERT( img->format->palette );
			dst->fmt.palette = img->fmt.palette;
		}

		const float fCustomImageCoords[8] = {
			0.02f,	0.78f,	// top left
			0.22f,	0.98f,	// bottom left
			0.98f,	0.22f,	// bottom right
			0.78f,	0.02f,	// top right
		};

		RageSurfaceUtils::BlitTransform( img, dst, fCustomImageCoords );

//		SDL_SaveBMP( dst, BackgroundPath + "-test.bmp" );
		delete img;
		img = dst;

		WasRotatedBackground = true;
	}



	const int src_width = img->w, src_height = img->h;

	int width = img->w / 2, height = img->h / 2;
//	int width = img->w, height = img->h;

	/* Round to the nearest power of two.  This simplifies the actual texture load. */
	width = closest(width, power_of_two(width), power_of_two(width) / 2);
	height = closest(height, power_of_two(height), power_of_two(height) / 2);

	/* Don't resize the image to less than 32 pixels in either dimension or the next
	 * power of two of the source (whichever is smaller); it's already very low res. */
	width = max( width, min(32, power_of_two(src_width)) );
	height = max( height, min(32, power_of_two(src_height)) );

	RageSurfaceUtils::ApplyHotPinkColorKey( img );

	RageSurfaceUtils::Zoom( img, width, height );

	/*
	 * When paletted background cache is enabled, cached backgrounds are paletted.  Cached
	 * 32-bit backgrounds take 1/16 as much memory, 16-bit backgrounds take 1/8, and paletted
	 * backgrounds take 1/4.
	 *
	 * When paletted background cache is disabled, cached backgrounds are stored in 16-bit
	 * RGBA.  Cached 32-bit backgrounds take 1/8 as much memory, cached 16-bit backgrounds
	 * take 1/4, and cached paletted backgrounds take 1/2.
	 *
	 * Paletted cache is disabled by default because palettization takes time, causing
	 * the initial cache run to take longer.  Also, newer ATI hardware doesn't supported
	 * paletted textures, which would slow down runtime, because we have to depalettize
	 * on use.  They'd still have the same memory benefits, though, since we only load
	 * one cached background into a texture at once, and the speed hit may not matter on
	 * newer ATI cards.  RGBA is safer, though.
	 */
	if( PREFSMAN->m_bPalettedBackgroundCache )
	{
		if( img->fmt.BytesPerPixel != 1 )
			RageSurfaceUtils::Palettize( img );
	} else {
		/* Dither to the final format.  We use A1RGB5, since that's usually supported
		 * natively by both OpenGL and D3D. */
		RageSurface *dst = CreateSurface( img->w, img->h, 16,
			0x7C00, 0x03E0, 0x001F, 0x8000 );

		/* OrderedDither is still faster than ErrorDiffusionDither, and
		 * these images are very small and only displayed briefly. */
		RageSurfaceUtils::OrderedDither( img, dst );
		delete img;
		img = dst;
	}

	const CString CachePath = GetBackgroundCachePath(BackgroundPath);
	RageSurfaceUtils::SaveSurface( img, CachePath );

	if( PREFSMAN->m_BackgroundCache == PrefsManager::BGCACHE_LOW_RES )
	{
		/* If an old image is loaded, free it. */
		if( g_BackgroundPathToImage.find(BackgroundPath) != g_BackgroundPathToImage.end() )
		{
			RageSurface *oldimg = g_BackgroundPathToImage[BackgroundPath];
			delete oldimg;
			g_BackgroundPathToImage.erase(BackgroundPath);
		}

		/* Keep it; we're just going to load it anyway. */
		g_BackgroundPathToImage[BackgroundPath] = img;
	}
	else
		delete img;

	/* Remember the original size. */
	BackgroundData.SetValue( BackgroundPath, "Path", CachePath );
	BackgroundData.SetValue( BackgroundPath, "Width", src_width );
	BackgroundData.SetValue( BackgroundPath, "Height", src_height );
	BackgroundData.SetValue( BackgroundPath, "FullHash", GetHashForFile( BackgroundPath ) );
	/* Remember this, so we can hint Sprite. */
	BackgroundData.SetValue( BackgroundPath, "Rotated", WasRotatedBackground );
	BackgroundData.WriteFile( BACKGROUND_CACHE_INDEX );
}

/*
 * (c) 2003 Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
