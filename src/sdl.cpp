/***************************************************************************
 *   Copyright (C) 2009-2013 by Veselin Georgiev, Slavomir Kaslev et al    *
 *   admin@raytracing-bg.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <SDL/SDL.h>
#include <stdio.h>
#include <algorithm>
#ifdef __MINGW32__
#	include <windows.h> // for AllocConsole()
#endif
#include "sdl.h"
using std::min;
using std::max;


SDL_Surface* screen = NULL;
SDL_Thread *render_thread;
SDL_mutex *render_lock;
volatile bool rendering = false;
bool render_async, wantToQuit = false;

void setupConsole(void)
{
	// under Linux, no special setup is necessary - Code::Blocks there does a sufficiently good job
	// On Windows, we have to setup the console explicitly.
#ifdef __MINGW32__
	AllocConsole();
	freopen("CONIN$",  "r",  stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif
}

/// try to create a frame window with the given dimensions
bool initGraphics(int frameWidth, int frameHeight)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Cannot initialize SDL: %s\n", SDL_GetError());
		return false;
	}
	screen = SDL_SetVideoMode(frameWidth, frameHeight, 32, 0);
	if (!screen) {
		printf("Cannot set video mode %dx%d - %s\n", frameWidth, frameHeight, SDL_GetError());
		return false;
	}
	render_lock = SDL_CreateMutex();
	return true;
}

/// closes SDL graphics
void closeGraphics(void)
{
	SDL_FreeSurface(screen);
	SDL_DestroyMutex(render_lock);
	SDL_Quit();
}

/// displays a VFB (virtual frame buffer) to the real framebuffer, with the necessary color clipping
void displayVFB(Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE])
{
	int rs = screen->format->Rshift;
	int gs = screen->format->Gshift;
	int bs = screen->format->Bshift;
	for (int y = 0; y < screen->h; y++) {
		Uint32 *row = (Uint32*) ((Uint8*) screen->pixels + y * screen->pitch);
		for (int x = 0; x < screen->w; x++)
			row[x] = vfb[y][x].toRGB32(rs, gs, bs);
	}
	SDL_Flip(screen);
}

void setWindowCaption(const char* msg, float renderTime)
{
	if (renderTime >= 0) {
		char message[128];
		sprintf(message, msg, renderTime);
		SDL_WM_SetCaption(message, NULL);
	} else {
		SDL_WM_SetCaption(msg, NULL);
	}
}

/// returns the frame width
int frameWidth(void)
{
	if (screen) return screen->w;
	return 0;
}

/// returns the frame height
int frameHeight(void)
{
	if (screen) return screen->h;
	return 0;
}

void Rect::clip(int W, int H)
{
	x1 = min(x1, W);
	y1 = min(y1, H);
	w = max(0, x1 - x0);
	h = max(0, y1 - y0);
}

static void handleEvent(SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_QUIT:
			wantToQuit = true;
			return;
		case SDL_KEYDOWN:
		{
			switch (ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					wantToQuit = true;
					return;
				default:
					break;
			}
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			extern void handleMouse(SDL_MouseButtonEvent *mev);
			handleMouse(&ev.button);
			break;
		}
		default:
			break;
	}
}

/// waits the user to indicate he wants to close the application (by either clicking on the "X" of the window,
/// or by pressing ESC)
void waitForUserExit(void)
{
	SDL_Event ev;
	while (!wantToQuit) {
		while (!wantToQuit && SDL_WaitEvent(&ev)) {
			handleEvent(ev);
		}
	}
}

std::vector<Rect> getBucketsList(void)
{
	std::vector<Rect> res;
	const int BUCKET_SIZE = 48;
	int W = frameWidth();
	int H = frameHeight();
	int BW = (W - 1) / BUCKET_SIZE + 1;
	int BH = (H - 1) / BUCKET_SIZE + 1;
	for (int y = 0; y < BH; y++) {
		if (y % 2 == 0)
			for (int x = 0; x < BW; x++)
				res.push_back(Rect(x * BUCKET_SIZE, y * BUCKET_SIZE, (x + 1) * BUCKET_SIZE, (y + 1) * BUCKET_SIZE));
		else
			for (int x = BW - 1; x >= 0; x--)
				res.push_back(Rect(x * BUCKET_SIZE, y * BUCKET_SIZE, (x + 1) * BUCKET_SIZE, (y + 1) * BUCKET_SIZE));
	}
	for (int i = 0; i < (int) res.size(); i++)
		res[i].clip(W, H);
	return res;
}

class MutexRAII {
	SDL_mutex* mutex;
public:
	MutexRAII(SDL_mutex* _mutex)
	{
		mutex = _mutex;
		SDL_mutexP(mutex);
	}
	~MutexRAII()
	{
		SDL_mutexV(mutex);
	}
};

bool drawRect(Rect r, const Color& c)
{
	MutexRAII raii(render_lock);
	
	if (render_async && !rendering) return false;
	
	r.clip(frameWidth(), frameHeight());
	
	int rs = screen->format->Rshift;
	int gs = screen->format->Gshift;
	int bs = screen->format->Bshift;
	
	Uint32 clr = c.toRGB32(rs, gs, bs);
	for (int y = r.y0; y < r.y1; y++) {
		Uint32 *row = (Uint32*) ((Uint8*) screen->pixels + y * screen->pitch);
		for (int x = r.x0; x < r.x1; x++)
			row[x] = clr;
	}
	SDL_UpdateRect(screen, r.x0, r.y0, r.w, r.h);
	
	return true;
}

bool displayVFBRect(Rect r, Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE])
{
	MutexRAII raii(render_lock);

	if (render_async && !rendering) return false;
	
	r.clip(frameWidth(), frameHeight());
	int rs = screen->format->Rshift;
	int gs = screen->format->Gshift;
	int bs = screen->format->Bshift;
	for (int y = r.y0; y < r.y1; y++) {
		Uint32 *row = (Uint32*) ((Uint8*) screen->pixels + y * screen->pitch);
		for (int x = r.x0; x < r.x1; x++)
			row[x] = vfb[y][x].toRGB32(rs, gs, bs);
	}
	SDL_UpdateRect(screen, r.x0, r.y0, r.w, r.h);
	
	return true;
}

bool markRegion(Rect r)
{
	MutexRAII raii(render_lock);

	if (render_async && !rendering) return false;
	
	r.clip(frameWidth(), frameHeight());
	const int L = 8;
	if (r.w < L || r.h < L) return true; // region is too small to be marked
	Uint32* row;
	const Uint32 BRACKET_COLOR = Color(0.0f, 0.0f, 0.5f).toRGB32();
	row = (Uint32*) ((Uint8*) screen->pixels + (r.y0) * screen->pitch);
	for (int x = r.x0; x < r.x0 + L; x++) row[x] = BRACKET_COLOR;
	for (int x = r.x1 - L - 1; x < r.x1; x++) row[x] = BRACKET_COLOR;
	row = (Uint32*) ((Uint8*) screen->pixels + (r.y1 - 1) * screen->pitch);
	for (int x = r.x0; x < r.x0 + L; x++) row[x] = BRACKET_COLOR;
	for (int x = r.x1 - L; x < r.x1; x++) row[x] = BRACKET_COLOR;
	for (int y = r.y0 + 1; y < r.y0 + L; y++) {
		row = (Uint32*) ((Uint8*) screen->pixels + (y) * screen->pitch);
		row[r.x0] = row[r.x1 - 1] = BRACKET_COLOR;
	}
	for (int y = r.y1 - L - 1; y < r.y1 - 1; y++) {
		row = (Uint32*) ((Uint8*) screen->pixels + (y) * screen->pitch);
		row[r.x0] = row[r.x1 - 1] = BRACKET_COLOR;
	}
	SDL_UpdateRect(screen, r.x0, r.y0, r.w, r.h);
	
	return true;
}

bool renderScene_Threaded(void)
{
	setWindowCaption("trinity: rendering");

	render_async = true;
	rendering = true;
	extern int renderSceneThread(void*);
	render_thread = SDL_CreateThread(renderSceneThread, NULL);
	
	if(render_thread == NULL) { //Failed to start for some bloody reason
		rendering = render_async = false;
		return false;
	}
	
	while (!wantToQuit) {
		{
			MutexRAII raii(render_lock);
			if (!rendering) break;
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {
				handleEvent(ev);
				if (wantToQuit) break;
			}
		}
		SDL_Delay(100);
	}
	rendering = false;
	SDL_WaitThread(render_thread, NULL);
	render_thread = NULL;
	
	render_async = false;
	return true;
}
