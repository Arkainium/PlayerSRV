#include "Surveyor.h"
#include "metrobotics.h"
using namespace metrobotics;

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <cstdlib>
#include <stack>
using namespace std;

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_gfxPrimitives.h"
#include "SDL/SDL_rotozoom.h"

/* Global variables because I'm a lazy SOB. */
/* I'm going to regret this later... Never mind, I already do. */
bool fQuitApplication = false;

Surveyor*  srv = 0;
SDL_mutex* srvMutex = 0;

const int    SCREEN_WIDTH  = 640;
const int    SCREEN_HEIGHT = 240;
const char*  APP_TITLE = "BlobConfig";
SDL_Surface* screen = 0;

bool fMouseDown = false;
int mouseX1 = 0;
int mouseX2 = 0;
int mouseY1 = 0;
int mouseY2 = 0;

const int    IMAGE_WIDTH  = 320;
const int    IMAGE_HEIGHT = 240;
SDL_Surface* jpegImage = 0;
SDL_Surface* jpegImageScaled = 0;
SDL_mutex*   jpegImageMutex = 0;
SDL_Surface* blobImage = 0;
SDL_Surface* blobImageScaled = 0;
SDL_mutex*   blobImageMutex = 0;

stack<YUVRange> undoList;
YUVRange currentRange("FF00FF00FF00"); // empty range to start
YUVRange newRange;

Surveyor* CreateSurveyor(string devName, Surveyor::CameraResolution camRes, double timeout);
void Shutdown();

bool fUpdatingImage = false;
bool fUpdatedImage  = false;
int UpdateImage(void* data);
SDL_Thread* imageThread = 0;

bool fUpdatedBlobs  = false;
int BlobFinder(void* data);
SDL_Thread* blobThread = 0;

bool fUpdatingBlobWindow = false;
int UpdateBlobWindow(void* data);
SDL_Thread* blobWindowThread = 0;

bool fYUVRangeChange = false;
bool fUpdatingYUVRange = false;
int UpdateYUVRange(void* data);
SDL_Thread* updateRangeThread = 0;

int main(int argc, char** argv)
{
	// Shutdown automatically.
	atexit(Shutdown);

	// Enable debugging output.
	PosixSerial::debugging(false);
	Surveyor::debugging(false);

	// To which Surveyor are we connecting?
	string devName = "/dev/ttyUSB0";
	if (argc > 1) {
		devName = argv[1];
	}

	// Connect to the Surveyor.
	// WARNING: Camera resolution MUST be 160x128 for blob finding to work.
	// I was puzzled for hours until I figured this out.
	if ((srv = CreateSurveyor(devName, Surveyor::CAMSIZE_160x128, 5)) == 0) {
		cerr << "Failed to connect to the Surveyor." << endl;
		exit(-1);
	}

	// Initialize SDL.
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		cerr << "Failed to initialize SDL: " << SDL_GetError();
		exit(-1);
	}
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 0, SDL_DOUBLEBUF);
	if (screen == 0) {
		cerr << "Failed to set video mode: " << SDL_GetError();
		exit(-1);
	}
	SDL_WM_SetCaption(APP_TITLE, 0);

	// Create our mutexes.
	if ((srvMutex = SDL_CreateMutex()) == 0 ||
		(jpegImageMutex = SDL_CreateMutex()) == 0 ||
	    (blobImageMutex = SDL_CreateMutex()) == 0) {
		cerr << "Failed to create mutex: " << SDL_GetError();
		exit(-1);
	}

	if ((blobThread = SDL_CreateThread(BlobFinder, 0)) == 0) {
		cerr << "Failed to start blob finder: " << SDL_GetError() << endl;
		exit(-1);
	}

	// Enter main loop.
	bool fQuitApplication = false;
	while (!fQuitApplication) {
		// Handle all events.
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					fQuitApplication = true;
				} break;
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE: {
							fQuitApplication = true;
						} break;
						case SDLK_i: { // Image
							if (!fUpdatingImage) {
								if ((imageThread = SDL_CreateThread(UpdateImage, 0)) == 0) {
									cerr << "Failed to get image: " << SDL_GetError() << endl;
								} else {
									fUpdatingImage = true;
								}
							}
						} break;
						case SDLK_u: { // Undo
							if (!undoList.empty()) {
								currentRange =  newRange = undoList.top();
								undoList.pop();
								fYUVRangeChange = true;
							}
						} break;
						case SDLK_r: { // Reset
							currentRange = newRange = YUVRange("FF00FF00FF00");
							fYUVRangeChange = true;
						} break;
						case SDLK_p: { // Print
							cout << endl;
							cout << "Hex: " << currentRange.toHexString() << '\n'
							     << "Y-range: " << setw(5) << currentRange.getYMin() << " - " << setw(3) << currentRange.getYMax() << '\n'
							     << "U-range: " << setw(5) << currentRange.getUMin() << " - " << setw(3) << currentRange.getUMax() << '\n'
							     << "V-range: " << setw(5) << currentRange.getVMin() << " - " << setw(3) << currentRange.getVMax() << '\n';
							cout << endl;
						} break;
						default: {
						} break;
					}
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					if (event.button.button == SDL_BUTTON_LEFT) {
						mouseX1 = mouseX2 = event.button.x;
						mouseY1 = mouseY2 = event.button.y;
						if (mouseX1 >= 0 && mouseX1 < IMAGE_WIDTH &&
						    mouseY1 >= 0 && mouseY1 < IMAGE_HEIGHT) {
							fMouseDown = true;
						}
					}
				} break;
				case SDL_MOUSEBUTTONUP: {
					if (event.button.button == SDL_BUTTON_LEFT) {
						if (fMouseDown) {
							fMouseDown = false;
							mouseX2 = event.button.x;
							mouseY2 = event.button.y;
							// Clip
							if (mouseX2 < 0) mouseX2 = 0;
							if (mouseX2 >= IMAGE_WIDTH) mouseX2 = IMAGE_WIDTH-1;
							if (mouseY2 < 0) mouseY2 = 0;
							if (mouseY2 >= IMAGE_HEIGHT) mouseY2 = IMAGE_HEIGHT-1;
							// Flip Y-coordinate
							mouseY1 = SCREEN_HEIGHT - mouseY1 - 1;
							mouseY2 = SCREEN_HEIGHT - mouseY2 - 1;
							// Compute BBox
							int xmin = min(mouseX1, mouseX2) * (80.0/IMAGE_WIDTH);
							int xmax = max(mouseX1, mouseX2) * (80.0/IMAGE_WIDTH);
							int ymin = min(mouseY1, mouseY2) * (64.0/IMAGE_HEIGHT);
							int ymax = max(mouseY1, mouseY2) * (64.0/IMAGE_HEIGHT);
							cerr << "selection: " << xmin << ", " << xmax << ", " << ymin << ", " << ymax << endl;
							Rect blobWin(xmin, xmax, ymin, ymax);
							if (!fUpdatingBlobWindow) {
								if ((blobWindowThread = SDL_CreateThread(UpdateBlobWindow, &blobWin)) == 0) {
									cerr << "Failed to update blob window: " << SDL_GetError() << endl;
								} else {
									fUpdatingBlobWindow = true;
								}
							}
						}
					}
				} break;
				case SDL_MOUSEMOTION: {
					if (fMouseDown) {
						mouseX2 = event.motion.x;
						mouseY2 = event.motion.y;
						if (mouseX2 < 0) mouseX2 = 0;
						if (mouseX2 >= IMAGE_WIDTH) mouseX2 = IMAGE_WIDTH-1;
						if (mouseY2 < 0) mouseY2 = 0;
						if (mouseY2 >= IMAGE_HEIGHT) mouseY2 = IMAGE_HEIGHT-1;
					}
				} break;
				default: {
				} break;
			}
		}

		// Update YUV range.
		if (!fUpdatingYUVRange && fYUVRangeChange) {
			if ((updateRangeThread = SDL_CreateThread(UpdateYUVRange, &newRange)) == 0) {
				cerr << "Failed to update yuv range: " << SDL_GetError() << endl;
			} else {
				fUpdatingYUVRange = true;
			}
		}

		// Update image.
		if (fUpdatedImage) {
			SDL_mutexP(jpegImageMutex);
				double scaleFactorX = IMAGE_WIDTH  / (double)jpegImage->w;
				double scaleFactorY = IMAGE_HEIGHT / (double)jpegImage->h;
				if ((jpegImageScaled = zoomSurface(jpegImage, scaleFactorX, scaleFactorY, 0)) == 0) {
					cerr << "Failed to scale image: " << SDL_GetError() << endl;
				}
			SDL_mutexV(jpegImageMutex);
			fUpdatedImage = false;
		}

		// Update blobs.
		if (fUpdatedBlobs) {
			SDL_mutexP(blobImageMutex);
				double scaleFactorX = IMAGE_WIDTH  / (double)blobImage->w;
				double scaleFactorY = IMAGE_HEIGHT / (double)blobImage->h;
				if ((blobImageScaled = zoomSurface(blobImage, scaleFactorX, scaleFactorY, 0)) == 0) {
					cerr << "Failed to scale image: " << SDL_GetError() << endl;
				}
			SDL_mutexV(blobImageMutex);
			fUpdatedBlobs = false;
		}

		// Clear screen.
		SDL_FillRect(screen, 0, 0);
		// Draw image.
		if (jpegImageScaled != 0) {
			SDL_BlitSurface(jpegImageScaled, 0, screen, 0);
		}
		// Draw blobs.
		SDL_Rect blobPos; blobPos.x = IMAGE_WIDTH; blobPos.y = 0;
		if (blobImageScaled != 0) {
			SDL_BlitSurface(blobImageScaled, 0, screen, &blobPos);
		}
		// Draw mouse selection.
		if (fMouseDown) {
			rectangleColor(screen, mouseX1, mouseY1, mouseX2, mouseY2, SDL_MapRGB(screen->format, 255, 0, 255));
		}

		SDL_Flip(screen);
	}

	return 0;
}

Surveyor* CreateSurveyor(string devName, Surveyor::CameraResolution camRes, double timeout)
{
	// Create the Surveyor object.
	Surveyor* ret = 0;
	PosixTimer pt;
	bool fDone = false;
	while (!fDone) {
		try {
			ret = new Surveyor(devName, camRes);
			ret->setColorBin(5, currentRange);
			fDone = true;
		} catch (...) {
			fDone = false;
			// Are we out of time?
			if (timeout > 0 && pt.elapsed() > timeout) {
				return 0;
			}
		}
	}
	return ret;
}

int UpdateImage(void* data)
{
	double timeout = data != 0 ? *((double*)data) : 5;
	Picture pic;
	SDL_mutexP(srvMutex);
	PosixTimer pt;
	bool fDone = false;
	while (!fDone) {
		try {
			pic = srv->takePicture();
			fDone = true;
		} catch (...) {
			// Are we out of time?
			if (timeout > 0 && pt.elapsed() > timeout) {
				fDone = true;
			} else {
				fDone = false;
			}
		}
	}
	SDL_mutexV(srvMutex);

	if (pic) {
		SDL_mutexP(jpegImageMutex);
			if (jpegImage != 0) {
				SDL_FreeSurface(jpegImage);
			}
			if ((jpegImage = IMG_Load_RW(SDL_RWFromConstMem(pic.data(), pic.size()), 1)) == 0) {
				cerr << "Failed to load image: " << SDL_GetError() << endl;
			} else {
				fUpdatedImage  = true;
			}
		SDL_mutexV(jpegImageMutex);
	}

	fUpdatingImage = false;
	return 0;
}

int BlobFinder(void* data)
{
	const unsigned int blobColor = SDL_MapRGB(screen->format, 255, 0, 255);
	const unsigned int blobBPP   = screen->format->BitsPerPixel;
	const unsigned int blobWinW  = 80;
	const unsigned int blobWinH  = 64;
	list<Blob> blobs;
	while (!fQuitApplication) {
		// Get blobs.
		SDL_mutexP(srvMutex);
		PosixTimer pt;
		bool fDone = false;
		while (!fDone) {
			try {
				srv->getBlobs(5, blobs);
				fDone = true;
			} catch (...) {
				if (pt.elapsed() > 5) {
					cerr << "Blob finder timed out." << endl;
					fDone = true;
					blobs.clear();
				} else {
					fDone = false;
				}
			}
		}
		SDL_mutexV(srvMutex);

		SDL_mutexP(blobImageMutex);
			if (blobImage == 0) {
				if ((blobImage = SDL_CreateRGBSurface(SDL_SWSURFACE,blobWinW,blobWinH,blobBPP,0,0,0,0)) < 0) {
					cerr << "Failed to create blob surface: " << SDL_GetError() << endl;
				}
			}
			if (blobImage != 0) {
				// Clear it.
				SDL_FillRect(blobImage, 0, 0);
				// Draw blobs.
				for (list<Blob>::iterator iter = blobs.begin(); iter != blobs.end(); ++iter) {
					SDL_Rect rect;
					rect.x = iter->getX1();
					rect.y = blobWinH - iter->getY2() - 1;
					rect.w = abs(iter->getX2() - iter->getX1());
					rect.h = abs(iter->getY2() - iter->getY1());
					SDL_FillRect(blobImage, &rect, blobColor);
				}
				fUpdatedBlobs = true;
			}
		SDL_mutexV(blobImageMutex);
		usleep(10000);
	}

	return 0;
}

int UpdateBlobWindow(void* data)
{
	if (data == 0) return 0;
	Rect rect = *((Rect*)data);
	SDL_mutexP(srvMutex);
	PosixTimer pt;
	bool fDone = false;
	while (!fDone) {
		try {
			newRange = srv->grabColorBin(7, rect);
			fYUVRangeChange = true;
			fDone = true;
		} catch (...) {
			// Are we out of time?
			if (pt.elapsed() > 5) {
				cerr << "Updating blob window timed out." << endl;
				fYUVRangeChange = false;
				fDone = true;
			} else {
				fYUVRangeChange = false;
				fDone = false;
			}
		}
	}
	SDL_mutexV(srvMutex);
	fUpdatingBlobWindow = false;
	return 0;
}

int UpdateYUVRange(void* data)
{
	if (data == 0) return 0;
	YUVRange yuv = *((YUVRange*)data);
	SDL_mutexP(srvMutex);
	PosixTimer pt;
	bool fDone = false;
	while (!fDone) {
		try {
			if (currentRange == yuv) {
				srv->setColorBin(5, currentRange);
			} else {
				srv->setColorBin(5, currentRange+yuv);
				undoList.push(currentRange);
				currentRange += yuv;
			}
			fYUVRangeChange = false;
			fDone = true;
		} catch (...) {
			// Are we out of time?
			if (pt.elapsed() > 5) {
				cerr << "Updating yuv range window timed out." << endl;
				fYUVRangeChange = true;
				fDone = true;
			} else {
				fYUVRangeChange = true;
				fDone = false;
			}
		}
	}
	SDL_mutexV(srvMutex);
	fUpdatingYUVRange = false;
	return 0;
}

void Shutdown()
{
	fQuitApplication = true;

	if (updateRangeThread != 0) {
		SDL_WaitThread(updateRangeThread, 0);
		updateRangeThread = 0;
	}

	if (blobWindowThread != 0) {
		SDL_WaitThread(blobWindowThread, 0);
		blobWindowThread = 0;
	}

	if (imageThread != 0) {
		SDL_WaitThread(imageThread, 0);
		imageThread = 0;
	}

	if (blobThread != 0) {
		SDL_WaitThread(blobThread, 0);
		blobThread = 0;
	}

	if (srvMutex != 0) {
		SDL_mutexP(srvMutex);
			if (srv != 0) {
				delete srv;
				srv = 0;
			}
		SDL_mutexV(srvMutex);
		SDL_DestroyMutex(srvMutex);
		srvMutex = 0;
	}

	if (jpegImageMutex != 0) {
		SDL_mutexP(jpegImageMutex);
			if (jpegImage != 0) {
				SDL_FreeSurface(jpegImage);
				jpegImage = 0;
			}
		SDL_mutexV(jpegImageMutex);
		SDL_DestroyMutex(jpegImageMutex);
		jpegImageMutex = 0;
	}

	if (blobImageMutex != 0) {
		SDL_mutexP(blobImageMutex);
			if (blobImage != 0) {
				SDL_FreeSurface(blobImage);
				blobImage = 0;
			}
		SDL_mutexV(blobImageMutex);
		SDL_DestroyMutex(blobImageMutex);
		blobImageMutex = 0;
	}

	if (jpegImageScaled != 0) {
		SDL_FreeSurface(jpegImageScaled);
		jpegImageScaled = 0;
	}

	if (blobImageScaled != 0) {
		SDL_FreeSurface(blobImageScaled);
		blobImageScaled = 0;
	}

	SDL_Quit();
}
