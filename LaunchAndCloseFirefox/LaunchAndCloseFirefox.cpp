// LaunchAndCloseFirefox.cpp 
// Launch Firefox and then close it as a user would do. The goal is to create the Firefox user profile and trigger the "first run only" Firefox 
// default pages like "Welcome to Firefox", "Firefox privacy notice", etc. so that they won't be displayed again when the user will launch Firefox.
//
#include "pch.h"
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>

using namespace std;

// program start time for timing in outputs (logs), used by elapsed_mseconds()
ULONGLONG start_mseconds = GetTickCount64();		// number of milliseconds that have elapsed since the system was started

// Milliseconds elapse since the program started
// requires: global variable start_mseconds
ULONGLONG elapsed_mseconds()
{
	return (GetTickCount64() - start_mseconds);		
}

// required for the function find_main_window() and enum_windows_callback_is_main()
struct process_id_main_window_hwnd {
	unsigned long process_id;
	HWND main_window_handle;
};


// Returns True
// required for the function find_main_window() and enum_windows_callback_is_main()
BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}


// handle (input) is the handle of the enumerated window
// lParam will be the result: it is a pointer to a process_id_main_window_hwnd structure variable
// Used by find_main_window
BOOL CALLBACK enum_windows_callback_is_main(HWND handle, LPARAM lParam)
{
	process_id_main_window_hwnd& data = *(process_id_main_window_hwnd*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle))
		return TRUE;
	data.main_window_handle = handle;
	return FALSE;
}


// Returns the window handle of the main window of a process id 
// returns 0 if there is no main window
// Requires the functions enum_windows_callback_is_main(), is_main_window() and the structure process_id_main_window_hwnd
// Hat down to "Ben" who wrote that, based on his finding about how .Net does this. https://stackoverflow.com/questions/1888863/how-to-get-main-window-handle-from-process-id
HWND find_main_window(unsigned long process_id)
{
	process_id_main_window_hwnd data;
	data.process_id = process_id;
	data.main_window_handle = 0;
	EnumWindows(enum_windows_callback_is_main, (LPARAM)&data);
	return data.main_window_handle;
}


// Send {Alt}+{F4} keybord keys to the application currently having the keyboard input
void SendKeys_Alt_F4() 
{

	// SendInput
	// This structure will be used to create the keyboard input event.
	INPUT ip;

	// Set up a generic keyboard event.
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0; // hardware scan code for key
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	// Press the "Alt" key
	ip.ki.wVk = VK_MENU; // virtual-key code for the "Alt" key
	ip.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &ip, sizeof(INPUT));

	// Press the "F4" key
	ip.ki.wVk = VK_F4; // virtual-key code for the "F4" key
	ip.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &ip, sizeof(INPUT));

	// Release the "F4" key
	ip.ki.wVk = VK_F4; // virtual-key code for the "F4" key
	ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ip, sizeof(INPUT));

	// Release the "Alt" key
	ip.ki.wVk = VK_MENU; // virtual-key code for the "Alt" key
	ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ip, sizeof(INPUT));

}


// Send {ENTER} keybord key to the application currently having the keyboard input
void SendKeys_ENTER()
{

	// SendInput
	// This structure will be used to create the keyboard input event.
	INPUT ip;

	// Set up a generic keyboard event.
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0; // hardware scan code for key
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	// Press the "ENTER" key
	ip.ki.wVk = VK_RETURN;	// virtual-key code for the "RETURN" key
	ip.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &ip, sizeof(INPUT));

	// Release the "ENTER" key
	ip.ki.wVk = VK_RETURN;	// virtual-key code for the "RETURN" key
	ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ip, sizeof(INPUT));

}


// Launch Firefox
BOOL Launch_Firefox(TCHAR *sFirefox_command)
{

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		sFirefox_command,       // Command line to launch Firefox
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)            // Pointer to PROCESS_INFORMATION structure
		)
	{
		cerr << elapsed_mseconds() << " ms: " << "CreateProcess failed " << GetLastError() << endl;
		return FALSE;
	}
	else {
		cout << elapsed_mseconds() << " ms: " << "CreateProcess succeeded. Process id is " << pi.dwProcessId << endl;
	}

	// Waits until the specified process has finished processing its initial input and is waiting for user input with no input pending, or until the time-out interval has elapsed.
	DWORD ret = WaitForInputIdle(pi.hProcess, 20000);

	// Close process and thread handles.
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	switch (ret)
	{
	case 0:
	{
		break;
	}
	case WAIT_TIMEOUT:
	{
		cerr << elapsed_mseconds() << " ms: " << "WaitForInputIdle failed returning WAIT_TIMEOUT." << endl;
		return FALSE;
		break;
	}
	default:
	{
		cerr << elapsed_mseconds() << " ms: " << "WaitForInputIdle failed " << GetLastError() << endl;
		return FALSE;
		break;
	}
	}

	return TRUE;
} 


// Is there any Firefox process running?
BOOL is_Firefox_running()
{

	HWND main_window_handle = 0;
	int Main_window_title_len = 0;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe;
		ZeroMemory(&pe, sizeof(PROCESSENTRY32));
		pe.dwSize = sizeof(PROCESSENTRY32);
		Process32First(hSnap, &pe);
		do
		{
			if (!lstrcmpi(pe.szExeFile, TEXT("firefox.exe")))
			{
				CloseHandle(hSnap);
				return TRUE;
			}

		} while (Process32Next(hSnap, &pe));

	}

	CloseHandle(hSnap);
	return FALSE;
}



// ======================================== MAIN ========================================================================

int _tmain(int argc, TCHAR *argv[])
{

	cout << elapsed_mseconds() << " ms: " << "======= START: LaunchAndCloseFirefox.exe v.1.1 by Denis GAUTIER =======" << endl;
	
	// arg 1: command to launch Firefox
	TCHAR* sCommand_to_launch_Firefox;
	if (argc < 2) {
		cerr << elapsed_mseconds() << " ms: " << "END: FAILURE! Missing argument for the command to launch Firefox." << endl;
		return 255;
	}
	else {
		sCommand_to_launch_Firefox = argv[1];
		wcout << elapsed_mseconds() << " ms: " << "arg 1: command to launch Firefox = [" << sCommand_to_launch_Firefox  << "]" << endl;
	}

	if (is_Firefox_running())
	{
		cout << elapsed_mseconds() << " ms: " << "Firefox was already running." << endl;
	}
	else
	{
		if (Launch_Firefox(sCommand_to_launch_Firefox))
		{
			cout << elapsed_mseconds() << " ms: " << "We launched Firefox because it was not running." << endl;
		}
		else
		{
			cerr << elapsed_mseconds() << " ms: " << "END: FAILURE! Failed to launch Firefox." << endl;
			return 1;
		}
	}

	// start timer for this phase (Firefox closing)
	ULONGLONG start_mseconds_phase = GetTickCount64();		 // number of milliseconds that have elapsed since the system was started.

	HWND main_window_handle = 0;
	int Main_window_title_len = 0;
	HANDLE process_handle = 0;
	DWORD ret = 0;
	HANDLE hSnap = 0;

	// Send {Alt}+{F4} and {Return} keys to the Firefox processes having a main window
	// until no Firefox process is detected or timeout
	do
	{

		hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 pe;
			ZeroMemory(&pe, sizeof(PROCESSENTRY32));
			pe.dwSize = sizeof(PROCESSENTRY32);
			Process32First(hSnap, &pe);
			do
			{
				if (!lstrcmpi(pe.szExeFile, TEXT("firefox.exe")))
				{
					main_window_handle = find_main_window(pe.th32ProcessID);
					if (main_window_handle == 0)
					{
						Main_window_title_len = 0;
						cout << elapsed_mseconds() << " ms: " << "Process ID " << pe.th32ProcessID << ": no Main Window." << endl;
					}
					else 
					{
						
						Main_window_title_len = GetWindowTextLength(main_window_handle);
						cout << elapsed_mseconds() << " ms: " << "Process ID " << pe.th32ProcessID << ", Main Window Title length = " << Main_window_title_len << endl;
						if (!SetForegroundWindow(main_window_handle))
						{
							cerr << elapsed_mseconds() << " ms: " << "SetForegroundWindow failed." << endl;
						}
						else 
						{
							// Send {Alt}+{F4} then {ENTER} to the application currently having the keyboard input
							SendKeys_Alt_F4();
							cout << elapsed_mseconds() << " ms: " << "ok we activated the main window of this process and waited for it to be in input idle state, then we sent {Alt}+{F4}" << endl;
							Sleep(2000);
							// Send {ENTER} to the application currently having the keyboard input
							SendKeys_ENTER();
							cout << elapsed_mseconds() << " ms: " << "ok we sent {ENTER}" << endl;
						}
					}
				}

			} while (Process32Next(hSnap, &pe));

			CloseHandle(hSnap);
			Sleep(1000);

		}
	} while (is_Firefox_running() && ((GetTickCount64() - start_mseconds_phase) <= 30000 ));

	// Successful onky if no Firefox is running now
	if (is_Firefox_running())
	{
		cerr << elapsed_mseconds() << " ms: " << "END: FAILURE! Firefox is still running." << endl;
		return 10;
	} 
	else
	{
		cout << elapsed_mseconds() << " ms: " << "END: SUCCESS!" << endl;
		return 0;
	}
}
