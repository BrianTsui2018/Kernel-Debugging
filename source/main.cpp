/*
	Course: CS373 - Homework Week 5
	Author: Brian Tsui (tsuio)
	Date: 2/11/2019
	Description: This program reads into the Windows Kernel, and offers 5 functions:
					-  1. Enumerates all running processes
					-  2. List all the running threads within a process's boundary (by PID)
					-  3. Enumerates all loaded modules for a process (by PID)
					-  4. Show only the executable pages for a process (by PID)
					-  5. Reads the memory pages of a process (by PID and address)
	
	Disclaimer:
					> Main framework for taking snapshot and viewing processes
					  https://docs.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes
					> Reading and printing memory
					  https://www.codeproject.com/Articles/4929/Toolhelp32ReadProcessMemory	  	 
					> String to dword
					  http://www.cplusplus.com/forum/beginner/95723/
*/

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <winnt.h>
using namespace std;

/*------------------------------------ 
		Function Headers
------------------------------------*/
BOOL GetProcessList(int);
void enumerate_proc(PROCESSENTRY32 &pe32, const HANDLE &hProcessSnap);
BOOL ListProcessModules(DWORD dwPID, bool exe);
BOOL ListProcessThreads(DWORD dwOwnerPID);
bool fetch_proc(const HANDLE &hProcessSnap, PROCESSENTRY32 &P);
bool fetch_mod(DWORD dwPID, MODULEENTRY32 &M);
LPBYTE read_mem(DWORD dwPID, MODULEENTRY32 me32);
void print_mem(int dwBlockSize, const LPBYTE &pBuffer);
void printError(const char* msg);
bool check_mem(DWORD dwPID, BYTE* address, MEMORY_BASIC_INFORMATION &mem);
bool check_exe(DWORD dwPID, MODULEENTRY32 me32);


/*------------------------------------
		Main Function
------------------------------------*/
int main(void)
{
	string userInput;
	int option;
	bool done = false;
	while (!done)
	{
		cout << endl << endl << "----- Please enter option number ----------------------" << endl <<
			"1. Enumerate all the running processes" << endl <<
			"2. List all the running threads within process boundary" << endl <<
			"3. Enumerate all the loaded modules within the processes" << endl <<
			"4. Show only the executable pages for a process" << endl <<
			"5. Read the memory pages of a process" << endl <<
			"6. Exit." << endl <<
			"Enter a number :   ";

		/*		User Input		*/
		getline(cin, userInput);
		option = atoi(&userInput[0]);

		/*		Quit handle		*/
		if (option == 6)
		{
			done = true;
		}
		else if (option >= 1 && option <= 5)
		{
			GetProcessList(option);
		}

	}
	return 0;
}

/*------------------------------------
		Get Process List
------------------------------------*/
BOOL GetProcessList(int o)
{
	HANDLE hProcessSnap;	
	PROCESSENTRY32 pe32;
	MODULEENTRY32 me32;
	LPBYTE pBuffer;

	/*		Take a snapshot of all processes in the system		*/
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
		return(FALSE);
	}

	/*		Set the size of the structure before using it		*/
	pe32.dwSize = sizeof(PROCESSENTRY32);


	/*		Retrieve information about the first process		
			and exit if unsuccessful							*/

	if (!Process32First(hProcessSnap, &pe32))			// read the first process from the snapshot to pe32
	{
		printError(TEXT("Process32First"));				// show cause of failure
		CloseHandle(hProcessSnap);						// clean the snapshot object
		return(FALSE);
	}

	/*		Functions		*/
	switch (o)
	{
		case 1:	enumerate_proc(pe32, hProcessSnap); 
				break;

		case 2:	if (fetch_proc(hProcessSnap, pe32)) {
					ListProcessThreads(pe32.th32ProcessID);
				}				
				break;		

		case 3:	if (fetch_proc(hProcessSnap, pe32)) {
					ListProcessModules(pe32.th32ProcessID, false);
				}
				break;

		case 4:	if (fetch_proc(hProcessSnap, pe32)) {
					ListProcessModules(pe32.th32ProcessID, true);
				}
				break;
		case 5:	if (fetch_proc(hProcessSnap, pe32)) {
					if (fetch_mod(pe32.th32ProcessID, me32)) 
					{
						pBuffer = read_mem(pe32.th32ProcessID, me32);
						print_mem(me32.modBaseSize, pBuffer);
					}
				}
				break;
	}
	
	CloseHandle(hProcessSnap);								// Cleaning the process snapshot
	cout << "\n\n--------------------------------"<< endl;	// Indicating end of program
	system("pause");
	return(TRUE);
}

/*------------------------------------
		Fetch Process Entry
------------------------------------*/
bool fetch_proc(const HANDLE &hProcessSnap, PROCESSENTRY32 &P)
{
	string userInput;
	DWORD thisPID;
	stringstream ss;
	bool found = false;
	
	/*		Takes user input to search for a process		*/		
	cout << "\n\nPlease enter a PID in hex (i.e. For \"0x0000A0FA\", just enter \"A0FA\") :   ";
	getline(cin, userInput);
	cout << endl;
	
	/*		Converting the user input string into DWORD		*/
	ss << userInput;
	ss >> hex >> thisPID;

	/*		Walk the snapshot of processes, and
			find the matching process, 
			copying the struct to P							*/
	HANDLE hProcess;
	DWORD dwPriorityClass;	   
	do																			// Loop through the list of processes in the snapshot
	{
		dwPriorityClass = 0;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, P.th32ProcessID);		// Open process to handle
		if (hProcess != NULL)
		{
			dwPriorityClass = GetPriorityClass(hProcess);
			if (!dwPriorityClass)
				printError(TEXT("GetPriorityClass"));
			CloseHandle(hProcess);
		}
	/*		Case of finding a match				*/
		if (P.th32ProcessID == thisPID)											// If the PID matches, return with the processEntry32 object 
		{
			_tprintf(TEXT("\nPROCESS NAME:  %s"), P.szExeFile);
			found = true;
			break;
		}

	} while (Process32Next(hProcessSnap, &P));				

	/*		Case of not finding any match		*/
	if (!found)	
	{
		cout << "Error!! fetch_proc() cannot find the PID you've entered!" << endl;
		return(FALSE);
	}

	return(TRUE);
}

/*------------------------------------
		Enumerate Processes
------------------------------------*/
void enumerate_proc(PROCESSENTRY32 &pe32, const HANDLE &hProcessSnap)
{
	/*		Now walk the snapshot of processes, and
			display information about each process in turn		*/
	HANDLE hProcess;
	DWORD dwPriorityClass;
	cout << "\nPID	--------- PROCESS NAME" << endl << "_________________________________________________________________________" << endl;
	
	/*		Loop through all the processes in the snapshot		*/	
	do
	{
		/*		Retrieve the priority class		*/
		dwPriorityClass = 0;

		/*		Open process to handle			*/		
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		if (hProcess != NULL)
		{
			dwPriorityClass = GetPriorityClass(hProcess);
			if (!dwPriorityClass)
				printError(TEXT("GetPriorityClass"));
			CloseHandle(hProcess);
		}

		/*		Print the process PID and name		*/	
		_tprintf(TEXT("\n0x%08X --------- %s"), pe32.th32ProcessID, pe32.szExeFile);

	} while (Process32Next(hProcessSnap, &pe32));

	cout << endl;
}



/*------------------------------------
		List Process Modules
------------------------------------*/
BOOL ListProcessModules(DWORD dwPID, bool exe)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;
	bool mod_is_exe;
	MEMORY_BASIC_INFORMATION mem = {};

	/*		 Take a snapshot of all modules in the specified process		*/
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
	if (hModuleSnap == INVALID_HANDLE_VALUE)
	{
		printError(TEXT("CreateToolhelp32Snapshot (of modules)"));
		return(FALSE);
	}

	/*		Set the size of the structure before using it		*/
	me32.dwSize = sizeof(MODULEENTRY32);

	/*		Retrieve information about the first module,
			and exit if unsuccessful							*/
	if (!Module32First(hModuleSnap, &me32))
	{
		printError(TEXT("Module32First"));			// show cause of failure
		CloseHandle(hModuleSnap);					// clean the snapshot object
		return(FALSE);
	}

	/*		Now walk the module list of the process,
			and display information about each module			*/
	do
	{
		/*		Check the memory's attributes		*/
		mod_is_exe = check_mem(dwPID, me32.modBaseAddr, mem);
		//mod_is_exe = check_exe(dwPID, me32);

		/*		Print		*/
		if (exe == false || (exe == true && mod_is_exe))
		{
			_tprintf(TEXT("\n\n     MODULE NAME:     %s"), me32.szModule);
			_tprintf(TEXT("\n     Base address   = 0x%08X"), (DWORD)me32.modBaseAddr);
			_tprintf(TEXT("\n     Base size      = %d"), me32.modBaseSize);
			_tprintf(TEXT("\n     AllocationProtect    = 0x%08X"), mem.AllocationProtect);
			if (mod_is_exe)
			{
				_tprintf(TEXT("     Is Executable	"));
			}
		}
	} while (Module32Next(hModuleSnap, &me32));

	CloseHandle(hModuleSnap);
	return(TRUE);
}


/*------------------------------------------
		Check Memory is Executable
------------------------------------------*/
bool check_mem(DWORD dwPID, BYTE* address, MEMORY_BASIC_INFORMATION &mem)
{
	LPCVOID lpAddress = address;
	int returnVal = -5;

	returnVal = VirtualQuery(lpAddress, &mem, sizeof mem);
	
	if (returnVal == 0)
	{
		printError(TEXT("VirtualQuery"));
	}	

	if (mem.AllocationProtect == (DWORD)128 || mem.AllocationProtect == (DWORD)64 || mem.AllocationProtect == (DWORD)32 || mem.AllocationProtect == (DWORD)16)
	{
		//_tprintf(TEXT("\n\n     AllocationProtect     0x%08X"), mem.AllocationProtect);
		return 1;
	}

	return 0;
}

/*-----------------------------------------------------------
		Check Memory is Executable (method 2)
-----------------------------------------------------------*/
bool check_exe(DWORD dwPID, MODULEENTRY32 me32)
{
	LPBYTE pBuffer = read_mem(dwPID, me32);

	if (pBuffer != NULL)
	{
		cout << "First 2 Bytes = [" << *pBuffer << " " << *(pBuffer + 1) << "]" << endl;
		if (*pBuffer == 77 && *(pBuffer + 1) == 90)
		{
			return(TRUE);
		}
		else
		{
			cout << "Check 1: not MZ" << endl;
		}

	}
	else
	{
		cout << "Check 2: pBuffer cannot read (is null)" << endl;
	}

	return(FALSE);
}

/*---------------------------------
		Fetch Module
---------------------------------*/
bool fetch_mod(DWORD dwPID, MODULEENTRY32 &M)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	string userInput;
	stringstream ss;
	DWORD thisAddr;
	bool found = false;

	cout << "\n\nPlease enter an memory address in hex (i.e. For \"0x0000A0FA\", just enter \"0000A0F0\") :   ";
	getline(cin, userInput);
	cout << endl;

	ss << userInput;
	ss >> hex >> thisAddr;

	/*		 Take a snapshot of all modules in the specified proces			*/
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
	if (hModuleSnap == INVALID_HANDLE_VALUE)
	{
		printError(TEXT("CreateToolhelp32Snapshot (of modules)"));
		return(FALSE);
	}

	/*		Set the size of the structure before using it		*/
	M.dwSize = sizeof(MODULEENTRY32);

	/*		Retrieve information about the first module,
			and exit if unsuccessful							*/
	if (!Module32First(hModuleSnap, &M))
	{
		printError(TEXT("Module32First"));			// show cause of failure
		CloseHandle(hModuleSnap);					// clean the snapshot object
		return(FALSE);
	}

	/*		Now walk the module list of the modules,
			and search for the matching module address			*/
	do
	{
		if ((DWORD)M.modBaseAddr == (DWORD)thisAddr)
		{
			//cout << "!!__________ FOUND ____________!!";
			//_tprintf(TEXT("\n     %s"), M.szModule);
			return true;
		}
	} while (Module32Next(hModuleSnap, &M));

	return false;
}

/*---------------------------------
		Read Memory Pages
---------------------------------*/
LPBYTE read_mem(DWORD dwPID, MODULEENTRY32 me32)
{
	LPCVOID baseAddr = (void*)me32.modBaseAddr;
	LPBYTE pBuffer;
	int dwBlockSize = me32.modBaseSize;

	pBuffer = new BYTE[dwBlockSize];
	
	_tprintf(TEXT("\nReading memory pages from MODULE :     %s\n\n"), me32.szModule);
	
	cout << "OFFSET\t    BYTES\t\t\t\t\t\t\tASCII\n" << endl;
	if (Toolhelp32ReadProcessMemory(dwPID, baseAddr, pBuffer, dwBlockSize, NULL))
	{
		return pBuffer;
		//print_mem(me32.modBaseSize, pBuffer);
	}
	return NULL;
		
}

/*---------------------------------
		Read Memory Pages
---------------------------------*/
void print_mem(int dwBlockSize,const LPBYTE &pBuffer)
{
	BYTE byte;
	DWORD dwBytesRead, dwBlock, dwOffset;

	/*		Go through the entire block			*/
	for (dwBlock = 0; dwBlock < dwBlockSize; dwBlock += 16)
	{
		/*		16 bytes per line		*/
		for (dwOffset = 0; dwOffset < 16; dwOffset++)
		{
			if (dwOffset == 0)
			{
				_tprintf(TEXT("0x%08X  "), dwBlock);
			}
			byte = *(pBuffer + dwBlock + dwOffset);

			_tprintf(TEXT("%02X "), byte);

			if (dwOffset % 8 == 7)
			{
				cout << "   ";
			}
		}
		cout << "\t";
		for (dwOffset = 0; dwOffset < 16; dwOffset++)
		{
			byte = *(pBuffer + dwBlock + dwOffset);

			//_tprintf(TEXT("%02X "), byte);

			/*		Account for non-printable characters		*/
			if (32 <= byte && byte < 127)
				cout << byte << " ";
			else
				cout << ". ";

			if (dwOffset % 8 == 7)
			{
				cout << "   ";
			}
		}

		if ((dwBlock / 16) == 40 || (dwBlock / 16) == 80)
		{
			cout << endl;
			system("pause");
		}

		cout << endl;
	}
	cout << endl;
}

/*----------------------------------------------
		List all threads for a process
----------------------------------------------*/
BOOL ListProcessThreads(DWORD dwOwnerPID)
{
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	/*		Take a snapshot of all running threads				*/
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return(FALSE);

	/*		Fill in the size of the structure before using it.	*/
	te32.dwSize = sizeof(THREADENTRY32);

	/*		Retrieve information about the first thread,
			and exit if unsuccessful							*/
	if (!Thread32First(hThreadSnap, &te32))
	{
		printError(TEXT("Thread32First"));			// show cause of failure
		CloseHandle(hThreadSnap);					// clean the snapshot object
		return(FALSE);
	}

	/*		Now walk the thread list of the system,
			and display information about each thread
			associated with the specified process				*/
	do
	{
		if (te32.th32OwnerProcessID == dwOwnerPID)
		{
			_tprintf(TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID);
			_tprintf(TEXT("\n     Base priority  = %d"), te32.tpBasePri);
			_tprintf(TEXT("\n     Delta priority = %d"), te32.tpDeltaPri);
			_tprintf(TEXT("\n"));
		}
	} while (Thread32Next(hThreadSnap, &te32));

	CloseHandle(hThreadSnap);
	return(TRUE);
}

/*-------------------------------
		Error Printing
--------------------------------*/
void printError(const char* msg)
{
	DWORD eNum;
	char sysMsg[256];
	char* p;

	eNum = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		sysMsg, 256, NULL);

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while ((*p > 31) || (*p == 9))
		++p;
	do { *p-- = 0; } while ((p >= sysMsg) &&
		((*p == '.') || (*p < 33)));

	// Display the message
	_tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}