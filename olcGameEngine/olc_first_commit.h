/*
OneLoneCoder.com - Command Line Game Engine
"Who needs a frame buffer?" - @Javidx9
Disclaimer
~~~~~~~~~~
I don't care what you use this for. It's intended to be educational, and perhaps
to the oddly minded - a little bit of fun. Please hack this, change it and use it
in any way you see fit. BUT, you acknowledge that I am not responsible for anything
bad that happens as a result of your actions. However, if good stuff happens, I
would appreciate a shout out, or at least give the blog some publicity for me.
Cheers!
Background
~~~~~~~~~~
If you've seen any of my videos - I like to do things using the windows console. It's quick
and easy, and allows you to focus on just the code that matters - ideal when you're 
experimenting. Thing is, I have to keep doing the same initialisation and display code
each time, so this class wraps that up.
Author
~~~~~~
Twitter: @javidx9
Blog: www.onelonecoder.com
Video:
~~~~~~
https://youtu.be/cWc0hgYwZyc
Last Updated: 10/07/2017
Usage:
~~~~~~
This class is abstract, so you must inherit from it. Override the OnUserCreate() function
with all the stuff you need for your application (for thready reasons it's best to do
this in this function and not your class constructor). Override the OnUserUpdate(float fElapsedTime)
function with the good stuff, it gives you the elapsed time since the last call so you
can modify your stuff dynamically. Both functions should return true, unless you need
the application to close.
	int main()
	{
		// Use olcConsoleGameEngine derived app
		OneLoneCoder_Example game;
		// Create a console with resolution 160x100 characters
		// Each character occupies 8x8 pixels
		game.ConstructConsole(160, 100, 8, 8);
		// Start the engine!
		game.Start();
		return 0;
	}
Input is also handled for you - interrogate the m_keys[] array with the virtual
keycode you want to know about. bPressed is set for the frame the key is pressed down
in, bHeld is set if the key is held down, bReleased is set for the frame the key
is released in.
The draw routines treat characters like pixels. By default they are set to white solid
blocks - but you can draw any unicode character, using any of the colours listed below.
There may be bugs! 
See my other videos for examples!
*/

#pragma once

#include <iostream>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <condition_variable>
using namespace std;

#include <windows.h>

enum COLOUR
{
	FG_BLACK		= 0x0000,
	FG_DARK_BLUE    = 0x0001,	
	FG_DARK_GREEN   = 0x0002,
	FG_DARK_CYAN    = 0x0003,
	FG_DARK_RED     = 0x0004,
	FG_DARK_MAGENTA = 0x0005,
	FG_DARK_YELLOW  = 0x0006,
	FG_GREY         = 0x0007,
	FG_BLUE			= 0x0009,
	FG_GREEN		= 0x000A,
	FG_CYAN			= 0x000B,
	FG_RED			= 0x000C,
	FG_MAGENTA		= 0x000D,
	FG_YELLOW		= 0x000E,
	FG_WHITE		= 0x000F,
	BG_BLACK		= 0x0000,
	BG_DARK_BLUE	= 0x0010,
	BG_DARK_GREEN	= 0x0020,
	BG_DARK_CYAN	= 0x0030,
	BG_DARK_RED		= 0x0040,
	BG_DARK_MAGENTA = 0x0050,
	BG_DARK_YELLOW	= 0x0060,
	BG_GREY			= 0x0070,
	BG_BLUE			= 0x0090,
	BG_GREEN		= 0x00A0,
	BG_CYAN			= 0x00B0,
	BG_RED			= 0x00C0,
	BG_MAGENTA		= 0x00D0,
	BG_YELLOW		= 0x00E0,
	BG_WHITE		= 0x00F0,
};

enum PIXEL_TYPE
{
	PIXEL_SOLID = 0x2588,
	PIXEL_HALF = 0x2592,
};


class olcConsoleGameEngine
{
public:
	olcConsoleGameEngine()
	{
		m_nScreenWidth = 80;
		m_nScreenHeight = 30;

		m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		m_keyNewState = new short[256];
		m_keyOldState = new short[256];
		memset(m_keyNewState, 0, 256 * sizeof(short));
		memset(m_keyOldState, 0, 256 * sizeof(short));

		memset(m_keys, 0, 256 * sizeof(sKeyState));


		m_sAppName = L"Default";

	}

	int ConstructConsole(int width, int height, int fontw = 12, int fonth = 12)
	{
		m_nScreenWidth = width;
		m_nScreenHeight = height;

		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(cfi);
		cfi.nFont = 0;
		cfi.dwFontSize.X = fontw;
		cfi.dwFontSize.Y = fonth;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;
		wcscpy_s(cfi.FaceName, L"Consolas");

		if (!SetCurrentConsoleFontEx(m_hConsole, false, &cfi))
			return Error(L"SetCurrentConsoleFontEx");

		COORD coordLargest = GetLargestConsoleWindowSize(m_hConsole);
		if (m_nScreenHeight > coordLargest.Y)
			return Error(L"Game Height Too Big");
		if (m_nScreenWidth > coordLargest.X)
			return Error(L"Game Width Too Big");

		COORD buffer = { (short)m_nScreenWidth, (short)m_nScreenHeight };
		if (!SetConsoleScreenBufferSize(m_hConsole, buffer))
			Error(L"SetConsoleScreenBufferSize");

		m_rectWindow = { 0, 0, (short)m_nScreenWidth - 1, (short)m_nScreenHeight - 1 };
		if (!SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow))
			Error(L"SetConsoleWindowInfo");

		m_bufScreen = new CHAR_INFO[m_nScreenWidth*m_nScreenHeight];

		return 1;
	}

	void Draw(int x, int y, wchar_t c = 0x2588, short col = 0x000F)
	{
		if (x >= 0 && x < m_nScreenWidth && y >= 0 && y < m_nScreenHeight)
		{
			m_bufScreen[y * m_nScreenWidth + x].Char.UnicodeChar = c;
			m_bufScreen[y * m_nScreenWidth + x].Attributes = col;
		}
	}

	void Fill(int x1, int y1, int x2, int y2, wchar_t c = 0x2588, short col = 0x000F)
	{
		Clip(x1, y1);
		Clip(x2, y2);
		for (int x = x1; x < x2; x++)
			for (int y = y1; y < y2; y++)
				Draw(x, y, c, col);
	}

	void DrawString(int x, int y, wstring c, short col = 0x000F)
	{
		for (size_t i = 0; i < c.size(); i++)
		{
			m_bufScreen[y * m_nScreenWidth + x + i].Char.UnicodeChar = c[i];
			m_bufScreen[y * m_nScreenWidth + x + i].Attributes = col;
		}
	}

	void Clip(int &x, int &y)
	{
		if (x < 0) x = 0;
		if (x >= m_nScreenWidth) x = m_nScreenWidth;
		if (y < 0) y = 0;
		if (y >= m_nScreenHeight) y = m_nScreenHeight;
	}

	void DrawLine(int x1, int y1, int x2, int y2, wchar_t c = 0x2588, short col = 0x000F)
	{
		int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
		dx = x2 - x1;
		dy = y2 - y1;
		dx1 = abs(dx);
		dy1 = abs(dy);
		px = 2 * dy1 - dx1;
		py = 2 * dx1 - dy1;
		if (dy1 <= dx1)
		{
			if (dx >= 0)
			{
				x = x1;
				y = y1;
				xe = x2;
			}
			else
			{
				x = x2;
				y = y2;
				xe = x1;
			}
			Draw(x, y, c, col);
			for (i = 0; x<xe; i++)
			{
				x = x + 1;
				if (px<0)
					px = px + 2 * dy1;
				else
				{
					if ((dx<0 && dy<0) || (dx>0 && dy>0))
						y = y + 1;
					else
						y = y - 1;
					px = px + 2 * (dy1 - dx1);
				}
				Draw(x, y, c, col);
			}
		}
		else
		{
			if (dy >= 0)
			{
				x = x1;
				y = y1;
				ye = y2;
			}
			else
			{
				x = x2;
				y = y2;
				ye = y1;
			}
			Draw(x, y, c, col);
			for (i = 0; y<ye; i++)
			{
				y = y + 1;
				if (py <= 0)
					py = py + 2 * dx1;
				else
				{
					if ((dx<0 && dy<0) || (dx>0 && dy>0))
						x = x + 1;
					else
						x = x - 1;
					py = py + 2 * (dx1 - dy1);
				}
				Draw(x, y, c, col);
			}
		}
	}



	~olcConsoleGameEngine()
	{
		SetConsoleActiveScreenBuffer(m_hOriginalConsole);
		delete[] m_bufScreen;
	}

public:
	void Start()
	{
		m_bAtomActive = true;

		// Star the thread
		thread t = thread(&olcConsoleGameEngine::GameThread, this);

		// Wait for thread to be exited
		m_cvGameFinished.wait(unique_lock<mutex>(m_muxGame));

		// Tidy up
		t.join();
	}

	int ScreenWidth()
	{
		return m_nScreenWidth;
	}

	int ScreenHeight() 
	{
		return m_nScreenHeight;
	}

private:
	void GameThread()
	{
		// Create user resources as part of this thread
		if (!OnUserCreate())
			return;

		auto tp1 = chrono::system_clock::now();
		auto tp2 = chrono::system_clock::now();

		// Run as fast as possible
		while (m_bAtomActive)
		{				
			// Handle Timing
			tp2 = chrono::system_clock::now();
			chrono::duration<float> elapsedTime = tp2 - tp1;
			tp1 = tp2;
			float fElapsedTime = elapsedTime.count();

			// Handle Input
			for (int i = 0; i < 256; i++)
			{
				m_keyNewState[i] = GetAsyncKeyState(i);

				m_keys[i].bPressed = false;
				m_keys[i].bReleased = false;

				if (m_keyNewState[i] != m_keyOldState[i])
				{
					if (m_keyNewState[i] & 0x8000)
					{
						m_keys[i].bPressed = !m_keys[i].bHeld;
						m_keys[i].bHeld = true;
					}
					else
					{
						m_keys[i].bReleased = true;
						m_keys[i].bHeld = false;
					}
				}

				m_keyOldState[i] = m_keyNewState[i];
			}


			// Handle Frame Update
			if (!OnUserUpdate(fElapsedTime))
				m_bAtomActive = false;

			// Update Title & Present Screen Buffer
			wchar_t s[128];
			swprintf_s(s, 128, L"OneLoneCoder.com - Console Game Engine - %s - FPS: %3.2f ", m_sAppName.c_str(), 1.0f / fElapsedTime);
			SetConsoleTitle(s);
			WriteConsoleOutput(m_hConsole, m_bufScreen, { (short)m_nScreenWidth, (short)m_nScreenHeight }, { 0,0 }, &m_rectWindow);
		}

		m_cvGameFinished.notify_one();
	}

public:
	// User MUST OVERRIDE THESE!!
	virtual bool OnUserCreate() = 0;
	virtual bool OnUserUpdate(float fElapsedTime) = 0;	


protected:
	int m_nScreenWidth;
	int m_nScreenHeight;
	CHAR_INFO *m_bufScreen;
	atomic<bool> m_bAtomActive;
	condition_variable m_cvGameFinished;
	mutex m_muxGame;
	wstring m_sAppName;

	struct sKeyState
	{
		bool bPressed;
		bool bReleased;
		bool bHeld;
	} m_keys[256];


protected:
	int Error(wchar_t *msg)
	{
		wchar_t buf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
		SetConsoleActiveScreenBuffer(m_hOriginalConsole);
		wprintf(L"ERROR: %s\n\t%s\n", msg, buf);
		return -1;
	}

private:
	HANDLE m_hOriginalConsole;
	CONSOLE_SCREEN_BUFFER_INFO m_OriginalConsoleInfo;
	HANDLE m_hConsole;
	SMALL_RECT m_rectWindow;
	short *m_keyOldState;
	short *m_keyNewState;
}; 