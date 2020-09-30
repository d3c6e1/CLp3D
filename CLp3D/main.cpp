#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>
#include <Windows.h>

// columns
const UINT CNSL_WIDTH = 120;
// rows
const UINT CNSL_HEIGHT = 40;
// world map dimensions
const UINT MAP_WIDTH = 16;
const UINT MAP_HEIGHT = 16;

// player start position
float playerX = 14.7f;
float playerY = 5.09f;

// Player Start Rotation
float playerA = 0.0f;

// Field of View
float fov = 3.14159f / 3.0f;

// Maximum rendering distance
float depth = 16.0f;

// Walking Speed
float speed = 5.0f;

void main()
{
	// get screen dimensions
	// const UINT SCRN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
	// const UINT SCRN_HEIGHT = GetSystemMetrics(SM_CYSCREEN);

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
	SMALL_RECT rect;
    rect.Top = 0;
    rect.Left = 0;
    rect.Bottom = CNSL_HEIGHT - 1; 
    rect.Right = CNSL_WIDTH - 1; 
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

	// Set console font
	CONSOLE_FONT_INFOEX font;
	ZeroMemory(&font, sizeof(font));
	font.cbSize = sizeof(font);
	lstrcpyW(font.FaceName, L"Consolas");
	font.dwFontSize.Y = 16;
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &font);
	
	// Create Screen Buffer
	auto screen = new WCHAR[CNSL_WIDTH * CNSL_HEIGHT + 1];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;
	
	// Create Map of world space # = wall block, . = space
	std::wstring map;
	map += L"################";
	map += L"#..............#";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#..........#...#";
	map += L"#......#.......#";
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
		if (GetAsyncKeyState(static_cast<unsigned short>('A')) & 0x8000)
		{
			playerA -= (speed * 0.75f) * fElapsedTime;
		}
	
		// Handle CW Rotation
		if (GetAsyncKeyState(static_cast<unsigned short>('D')) & 0x8000)
		{
			playerA += (speed * 0.75f) * fElapsedTime;
		}

		// Handle movent and collision
		if (GetAsyncKeyState(static_cast<unsigned short>('W')) & 0x8000)
		{
			playerX += sinf(playerA) * speed * fElapsedTime;
			playerY += cosf(playerA) * speed * fElapsedTime;
			if (map.c_str()[static_cast<int>(playerX) * MAP_WIDTH + static_cast<int>(playerY)] == '#')
			{
				playerX -= sinf(playerA) * speed * fElapsedTime;
				playerY -= cosf(playerA) * speed * fElapsedTime;
			}			
		}

		if (GetAsyncKeyState(static_cast<unsigned short>('S')) & 0x8000)
		{
			playerX -= sinf(playerA) * speed * fElapsedTime;
			playerY -= cosf(playerA) * speed * fElapsedTime;
			if (map.c_str()[static_cast<int>(playerX) * MAP_WIDTH + static_cast<int>(playerY)] == '#')
			{
				playerX += sinf(playerA) * speed * fElapsedTime;
				playerY += cosf(playerA) * speed * fElapsedTime;
			}
		}

		// Handle app closing
		if(GetAsyncKeyState(VK_ESCAPE))
		{
			return;
		}
	
		for (UINT x = 0; x < CNSL_WIDTH; x++)
		{
			// For each column, calculate the projected ray angle into world space
			float fRayAngle = (playerA - fov/2.0f) + (static_cast<float>(x) / static_cast<float>(CNSL_WIDTH)) * fov;
	
			// Find distance to wall
			float fStepSize = 0.1f;		  // Increment size for ray casting, decrease to increase resolution
			float fDistanceToWall = 0.0f;
	
			bool bHitWall = false;		// Set when ray hits wall block
			bool bBoundary = false;		// Set when ray hits boundary between two wall blocks
	
			float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);
	
			// Incrementally cast ray from player, along ray angle, testing for 
			// intersection with a block
			while (!bHitWall && fDistanceToWall < depth)
			{
				fDistanceToWall += fStepSize;
				int nTestX = (int)(playerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(playerY + fEyeY * fDistanceToWall);
				
				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= MAP_WIDTH || nTestY < 0 || nTestY >= MAP_HEIGHT)
				{
					bHitWall = true;			// Just set distance to maximum depth
					fDistanceToWall = depth;
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
								float vx = static_cast<float>(nTestX) + static_cast<float>(tx) - playerX;
								float vy = static_cast<float>(nTestY) + static_cast<float>(ty) - playerY;
								float d = static_cast<float>(std::sqrt(vx * vx + vy * vy));
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.emplace_back(d, dot);
							}

						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const std::pair<float, float> &left, const std::pair<float, float> &right) {return left.first < right.first; });
						
						// First two/three are closest (we will never see all four)
						float fBound = 0.002f;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			float nCeiling = static_cast<float>(CNSL_HEIGHT / 2.0) - CNSL_HEIGHT / static_cast<float>(fDistanceToWall);
			UINT nFloor = CNSL_HEIGHT - static_cast<UINT>(nCeiling);
	
			// Shader walls based on distance
			short nShade = ' ';
			
			// Very close
			if (fDistanceToWall <= depth / 4.0f)        nShade = 0x2588;
		    else if (fDistanceToWall < depth / 3.0f)    nShade = 0x2593;
		    else if (fDistanceToWall < depth / 2.0f)    nShade = 0x2592;
		    else if (fDistanceToWall < depth)           nShade = 0x2591;
		    else                                        nShade = ' ';

			// fill borders
			if (bBoundary)
			{
				nShade = ' ';
			}
			
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
					float b = 1.0f - ((static_cast<float>(y) -CNSL_HEIGHT/2.0f) / (static_cast<float>(CNSL_HEIGHT) / 2.0f));
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
		swprintf_s(screen, 75, L"move:W/S, rotate:A/D, exit:ESC X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f", playerX, playerY, playerA, 1.0f/fElapsedTime);
	
		// Display Map
		for (UINT nx = 0; nx < MAP_WIDTH; nx++)
			for (UINT ny = 0; ny < MAP_WIDTH; ny++)
			{
				screen[(ny+1)*CNSL_WIDTH + nx] = map[ny * MAP_WIDTH + nx];
			}
		screen[(static_cast<int>(playerX)+1) * CNSL_WIDTH + static_cast<int>(playerY)] = '@';
	
		// Display Frame
		screen[CNSL_WIDTH * CNSL_HEIGHT] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, CNSL_WIDTH * CNSL_HEIGHT, { 0,0 }, &dwBytesWritten);
	}
}