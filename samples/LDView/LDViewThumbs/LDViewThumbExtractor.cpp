// LDViewThumbExtractor.cpp : Implementation of CLDViewThumbExtractor
#include "stdafx.h"
#include "LDViewThumbs.h"
#include "LDViewThumbExtractor.h"
#include <TCFoundation/TCUserDefaults.h>
#include <TCFoundation/TCImage.h>
#include <TCFoundation/mystring.h>

#define INSTALL_PATH_KEY "InstallPath"
#define INSTALL_PATH_4_1_KEY "InstallPath 4.1"

#ifdef _DEBUG
//#define DEBUG_LOG
#endif // _DEBUG
#ifdef DEBUG_LOG
FILE *g_logFile = NULL;
#endif // DEBUG_LOG

/////////////////////////////////////////////////////////////////////////////
// CLDViewThumbExtractor

bool CLDViewThumbExtractor::findLDView(void)
{
	TCUserDefaults::setAppName("Travis Cobbs/LDView");
	char *lDViewDir = TCUserDefaults::stringForKey(INSTALL_PATH_4_1_KEY);

	if (lDViewDir == NULL)
	{
		lDViewDir = TCUserDefaults::stringForKey(INSTALL_PATH_KEY);
	}
	if (lDViewDir != NULL)
	{
		FILE *lDView;

		m_ldviewDir = lDViewDir;
		delete lDViewDir;
		m_ldviewPath = m_ldviewDir + "\\LDView.exe";
		lDView = fopen(m_ldviewPath.c_str(), "rb");
		if (lDView)
		{
			fclose(lDView);
			return true;
		}
		m_ldviewPath = m_ldviewDir + "\\LDView64.exe";
		lDView = fopen(m_ldviewPath.c_str(), "rb");
		if (lDView)
		{
			fclose(lDView);
			return true;
		}
		m_ldviewPath = "";
	}
	return false;
}

/*
static bool copyToClipboard(const wchar_t *value)
{
	int len = wcslen(value) + 1;
	HGLOBAL hBuf = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
		len * sizeof(wchar_t));

	if (hBuf)
	{
		wchar_t *buf = (wchar_t*)GlobalLock(hBuf);

		wcscpy(buf, value);
		GlobalUnlock(hBuf);
		if (OpenClipboard(NULL))
		{
			EmptyClipboard();
			SetClipboardData(CF_UNICODETEXT, hBuf);
			CloseClipboard();
			return true;
		}
		else
		{
			GlobalFree(hBuf);
		}
	}
	return false;
}
*/

bool CLDViewThumbExtractor::processFile(
	const wchar_t *datPath,
	const char *imageFilename)
{
	wchar_t commandLine[1024];
	STARTUPINFOW startupInfo;
	PROCESS_INFORMATION processInfo;
	int windowWidth = m_size.cx;
	int windowHeight = m_size.cy;
	DWORD priority = NORMAL_PRIORITY_CLASS;

	if (windowWidth < 320)
	{
		windowWidth = 320;
	}
	if (windowHeight < 240)
	{
		windowHeight = 240;
	}
	wsprintfW(commandLine, L"\"%S\" \"%s\" -SaveSnapshot=%S -SaveActualSize=0 "
		L"-SaveWidth=%d -SaveHeight=%d -CheckPartTracker=0"
		L"-WindowWidth=%d -WindowHeight=%d -SaveZoomToFit=1 "
		L"-PreferenceSet=Thumbnails -SnapshotSuffix=.bmp",
		m_ldviewPath.c_str(), datPath, imageFilename, m_size.cx, m_size.cy,
		windowWidth, windowHeight);
	//copyToClipboard(commandLine);
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags = STARTF_USEPOSITION;
	startupInfo.dwX = 0;
	startupInfo.dwY = 0;
	USES_CONVERSION;
#ifdef DEBUG_LOG
	if (g_logFile != NULL)
	{
		fwrite(commandLine, wcslen(commandLine) * 2, 1, g_logFile);
		fwrite(L"\r\n", 4, 1, g_logFile);
	}
#endif // DEBUG_LOG
	debugLog1s("ThumbsLog", L"Launching: %s\r\n", commandLine);
	if (CreateProcessW(A2W(m_ldviewPath.c_str()), commandLine, NULL, NULL, FALSE,
		DETACHED_PROCESS | priority, NULL, NULL, &startupInfo, &processInfo))
	{
		while (1)
		{
			DWORD exitCode;

			GetExitCodeProcess(processInfo.hProcess, &exitCode);
			if (exitCode != STILL_ACTIVE)
			{
				debugLog("ThumbsLog", L"LDView exit code: %d\r\n", exitCode);
				printf("Done.\n");
				return true;
			}
			Sleep(50);
		}
	}
	else
	{
		DWORD error = GetLastError();
		char *buf;

		printf("Failed!\n");
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
			error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf,
			0, NULL);
		printf("%s\n", buf);
		debugLog("ThumbsLog", L"Error launching:\r\n%s\r\n", buf);
		LocalFree(buf);
		return false;
	}
}



////////////////////////////////////////////////////////////////////////////
// IPersistFile Begin
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CLDViewThumbExtractor::Load(
	/* [in] */ LPCOLESTR pszFileName,
	/* [in] */ DWORD /*dwMode*/)
{
	if (!findLDView())
	{
		return E_FAIL;
	}
	m_path = pszFileName;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// IPersistFile End
////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////
// IExtractImage2 Begin
////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLDViewThumbExtractor::GetLocation( 
	/* [size_is][out] */ LPWSTR pszPathBuffer,
	/* [in] */ DWORD cch,
	/* [unique][out][in] */ DWORD * /*pdwPriority*/,
	/* [in] */ const SIZE *prgSize,
	/* [in] */ DWORD /*dwRecClrDepth*/,
	/* [out][in] */ DWORD *pdwFlags)
{
	if (cch > 0)
	{
		pszPathBuffer[0] = 0;
	}
	m_size = *prgSize;
	*pdwFlags |= IEIFLAG_CACHE;
	if (*pdwFlags & IEIFLAG_ASYNC)
	{
		return E_PENDING;
	}
	else
	{
		return S_OK;
	}
}

bool CLDViewThumbExtractor::isLDrawFile(void)
{
	std::string filename;
	FILE *file;
	bool retValue = false;

	wstringtostring(filename, m_path);
	file = fopen(filename.c_str(), "rb");
	if (file != NULL)
	{
		while (!retValue)
		{
			char buf[1024];
			char *spot = buf;

			if (fgets(buf, sizeof(buf), file) == NULL)
			{
				break;
			}
			buf[sizeof(buf) - 1] = 0;
			replaceStringCharacter(buf, '\t', ' ');
			stripCRLF(buf);
			while (spot[0] == ' ' || spot[0] == '\r' || spot[0] == '\n')
			{
				spot++;
			}
			if (spot[0] == '0' && (spot[1] == ' ' || spot[1] == 0))
			{
			}
			else if (spot[0] >= '1' && spot[0] <= '5' && spot[1] == ' ')
			{
				retValue = true;
			}
			else if (spot[0])
			{
#ifdef DEBUG_LOG
			if (g_logFile != NULL)
			{
				std::wstring message = L"FAILED on line: ";
				std::wstring tempString;

				stringtowstring(tempString, buf);
				message += tempString;
				message += L"\r\n";
				fwrite(message.c_str(), message.size() * 2, 1, g_logFile);
			}
#endif // DEBUG_LOG
				break;
			}
		}
		fclose(file);
	}
	return retValue;
}

STDMETHODIMP CLDViewThumbExtractor::Extract(/* [out] */ HBITMAP *phBmpThumbnail)
{
	HRESULT hr = E_FAIL;

#ifdef DEBUG_LOG
	g_logFile = fopen("C:\\temp\\LDViewThumbs.log", "ab");
#endif // DEBUG_LOG
	*phBmpThumbnail = NULL;
	if (findLDView() && isLDrawFile())
	{
		TCHAR szTempPath[1024];
		TCHAR szTempFilename[MAX_PATH + 16];

		if (GetTempPath(sizeof(szTempPath) / sizeof(szTempPath[0]), szTempPath) == 0)
		{
			strcpy(szTempPath, "C:\\Temp");
		}
		if (GetTempFileName(szTempPath, "LDVThumb", 0, szTempFilename))
		{
			if (processFile(m_path.c_str(), szTempFilename))
			{
				TCImage *image = new TCImage;

				image->setFlipped(true);
				image->setLineAlignment(4);
				if (image->loadFile(szTempFilename))
				{
					BITMAPINFO bmInfo;
					TCByte *bmData;
					TCByte *imageData;
					int imageWidth;
					int imageHeight;
					int srcRowSize;
					int dstRowSize;
					int row;
					int col;
					HBITMAP hbm;

					SetLastError(0);
					bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);
					bmInfo.bmiHeader.biWidth = image->getWidth();
					bmInfo.bmiHeader.biHeight = image->getHeight();
					bmInfo.bmiHeader.biPlanes = 1;
					bmInfo.bmiHeader.biBitCount = 24;
					bmInfo.bmiHeader.biCompression = BI_RGB;
					bmInfo.bmiHeader.biSizeImage = 0;
					bmInfo.bmiHeader.biXPelsPerMeter = 1;
					bmInfo.bmiHeader.biYPelsPerMeter = 1;
					bmInfo.bmiHeader.biClrUsed = 0;
					bmInfo.bmiHeader.biClrImportant = 0;
					HDC hMEMDC = CreateCompatibleDC(NULL);
					hbm = CreateDIBSection(hMEMDC, &bmInfo, DIB_RGB_COLORS,
						(void**)&bmData, NULL, 0);
					imageData = image->getImageData();
					imageWidth = image->getWidth();
					imageHeight = image->getHeight();
					srcRowSize = image->getRowSize();
					dstRowSize = TCImage::roundUp(imageWidth * 3, 4);
					for (row = 0; row < imageHeight; row++)
					{
						for (col = 0; col < imageWidth; col++)
						{
							int srcOffset = row * srcRowSize + col * 3;
							int dstOffset = row * dstRowSize + col * 3;

							bmData[dstOffset] = imageData[srcOffset + 2];
							bmData[dstOffset + 1] = imageData[srcOffset + 1];
							bmData[dstOffset + 2] = imageData[srcOffset];
						}
					}
					*phBmpThumbnail = hbm;
					hr = S_OK;
				}
				image->release();
			}
			DeleteFile(szTempFilename);
		}
	}
#ifdef DEBUG_LOG
	if (g_logFile != NULL)
	{
		fclose(g_logFile);
		g_logFile = NULL;
	}
#endif // DEBUG_LOG
	return hr;
}

STDMETHODIMP CLDViewThumbExtractor::GetDateStamp(/* [out] */ FILETIME *pDateStamp)
{
    FILETIME ftCreate, ftAccess, ftWrite;
	HANDLE hFile = CreateFileW(m_path.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!hFile)
	{
		return E_ACCESSDENIED;
	}
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
	{
		CloseHandle(hFile);
		return E_ACCESSDENIED;
	}
	*pDateStamp = ftWrite;
	CloseHandle(hFile);
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// IExtractImage2 End
////////////////////////////////////////////////////////////////////////////
