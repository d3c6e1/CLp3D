#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <Windows.h>

// columns
unsigned int CNSL_WIDTH = 120;
// rows
unsigned int CNSL_HEIGHT = 40;
// world map dimensions
const unsigned int MAP_WIDTH = 16;
const unsigned int MAP_HEIGHT = 16;

// player start position
float fPlayerX = 14.7f;
float fPlayerY = 5.09f;

// Player Start Rotation
float fPlayerA = 0.0f;

// Field of View
float fFOV = 3.14159f / 4.0f;

// Maximum rendering distance
float fDepth = 16.0f;

// Walking Speed
float fSpeed = 5.0f;

int main()
{
	// get screen dimensions
	// const unsigned int SCRN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
	// const unsigned int SCRN_HEIGHT = GetSystemMetrics(SM_CYSCREEN);

	// CNSL_HEIGHT = SCRN_HEIGHT/10;
	// CNSL_WIDTH = SCRN_WIDTH/10;

	// fullscreen console
	// keybd_event(VK_MENU,0x38,0,0);
	// keybd_event(VK_RETURN,0x1c,0,0);
	// keybd_event(VK_RETURN,0x1c,KEYEVENTF_KEYUP,0);
	// keybd_event(VK_MENU,0x38,KEYEVENTF_KEYUP,0);
	
	// cursor to the center
	// SetCursorPos(SCRN_WIDTH/2,SCRN_HEIGHT/2);
	// console colors
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0f);

	// Set console window size
	_SMALL_RECT rect;
    rect.Top = 0;
    rect.Left = 0;
    rect.Bottom = CNSL_HEIGHT - 1; 
    rect.Right = CNSL_WIDTH - 1; 
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

	// Set console font
	_CONSOLE_FONT_INFOEX font;
	ZeroMemory(&font, sizeof(font));
	font.cbSize = sizeof(font);
	lstrcpyW(font.FaceName, L"Consolas");
	font.dwFontSize.Y = 16;
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &font);
	
	// Create Screen Buffer
	wchar_t *screen = new wchar_t[CNSL_WIDTH * CNSL_HEIGHT];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;
	
	// Create Map of world space # = wall block, . = space
	std::wstring map;
	map += L"################";
	map += L"#..............#";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	
	while (true)
	{
		// We'll need time differential per frame to calculate modification
		// to movement speeds, to ensure consistant movement, as ray-tracing
		// is non-deterministic
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();
	
	
		// Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= (fSpeed * 0.75f) * fElapsedTime;
	
		// Handle CW Rotation
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += (fSpeed * 0.75f) * fElapsedTime;
		
		// Handle Forwards movement & collision
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;;
			fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;;
			if (map.c_str()[(int)fPlayerX * MAP_WIDTH + (int)fPlayerY] == '#')
			{
				fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;;
				fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;;
			}			
		}
	
		// Handle backwards movement & collision
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;;
			fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;;
			if (map.c_str()[(int)fPlayerX * MAP_WIDTH + (int)fPlayerY] == '#')
			{
				fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;;
				fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;;
			}
		}
	
		for (int x = 0; x < CNSL_WIDTH; x++)
		{
			// For each column, calculate the projected ray angle into world space
			float fRayAngle = (fPlayerA - fFOV/2.0f) + ((float)x / (float)CNSL_WIDTH) * fFOV;
	
			// Find distance to wall
			float fStepSize = 0.1f;		  // Increment size for ray casting, decrease to increase										
			float fDistanceToWall = 0.0f; //                                      resolution
	
			bool bHitWall = false;		// Set when ray hits wall block
			bool bBoundary = false;		// Set when ray hits boundary between two wall blocks
	
			float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);
	
			// Incrementally cast ray from player, along ray angle, testing for 
			// intersection with a block
			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += fStepSize;
				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);
				
				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= MAP_WIDTH || nTestY < 0 || nTestY >= MAP_HEIGHT)
				{
					bHitWall = true;			// Just set distance to maximum depth
					fDistanceToWall = fDepth;
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map.c_str()[nTestX * MAP_WIDTH + nTestY] == '#')
					{
						// Ray has hit wall
						bHitWall = true;
	
						// To highlight tile boundaries, cast a ray from each corner
						// of the tile, to the player. The more coincident this ray
						// is to the rendering ray, the closer we are to a tile 
						// boundary, which we'll shade to add detail to the walls
						std::vector<std::pair<float, float>> p;
	
						// Test each corner of hit tile, storing the distance from
						// the player, and the calculated dot product of the two rays
						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								// Angle of corner to eye
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx*vx + vy*vy); 
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(std::make_pair(d, dot));
							}
	
						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const std::pair<float, float> &left, const std::pair<float, float> &right) {return left.first < right.first; });
						
						// First two/three are closest (we will never see all four)
						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
						if (acos(p.at(2).second) < fBound) bBoundary = true;
					}
				}
			}
		
			// Calculate distance to ceiling and floor
			int nCeiling = (float)(CNSL_HEIGHT/2.0) - CNSL_HEIGHT / ((float)fDistanceToWall);
			int nFloor = CNSL_HEIGHT - nCeiling;
	
			// Shader walls based on distance
			short nShade = ' ';
			if (fDistanceToWall <= fDepth / 4.0f)			nShade = 0x2588;	// Very close	
			else if (fDistanceToWall < fDepth / 3.0f)		nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0f)		nShade = 0x2592;
			else if (fDistanceToWall < fDepth)				nShade = 0x2591;
			else											nShade = ' ';		// Too far away
	
			if (bBoundary)		nShade = ' '; // Black it out
			
			for (int y = 0; y < CNSL_HEIGHT; y++)
			{
				// Each Row
				if(y <= nCeiling)
					screen[y*CNSL_WIDTH + x] = ' ';
				else if(y > nCeiling && y <= nFloor)
					screen[y*CNSL_WIDTH + x] = nShade;
				else // Floor
				{				
					// Shade floor based on distance
					float b = 1.0f - (((float)y -CNSL_HEIGHT/2.0f) / ((float)CNSL_HEIGHT / 2.0f));
					if (b < 0.25)		nShade = '#';
					else if (b < 0.5)	nShade = 'x';
					else if (b < 0.75)	nShade = '.';
					else if (b < 0.9)	nShade = '-';
					else				nShade = ' ';
					screen[y*CNSL_WIDTH + x] = nShade;
				}
			}
		}
	
		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f/fElapsedTime);
	
		// Display Map
		for (int nx = 0; nx < MAP_WIDTH; nx++)
			for (int ny = 0; ny < MAP_WIDTH; ny++)
			{
				screen[(ny+1)*CNSL_WIDTH + nx] = map[ny * MAP_WIDTH + nx];
			}
		screen[((int)fPlayerX+1) * CNSL_WIDTH + (int)fPlayerY] = 'P';
	
		// Display Frame
		screen[CNSL_WIDTH * CNSL_HEIGHT - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, CNSL_WIDTH * CNSL_HEIGHT, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}