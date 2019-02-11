# Kernel-Debugging

Course: CS373 - Homework Week 5
Author: Brian Tsui (tsuio)
Date: 2/11/2019

## Description: 
This program reads into the Windows Kernel, and offers 5 functions -
1. Enumerates all running processes
2. List all the running threads within a process's boundary (by PID)
3. Enumerates all loaded modules for a process (by PID)
4. Show only the executable pages for a process (by PID)
5. Reads the memory pages of a process (by PID and address)

## Code reference disclaimer:

*	Main framework for taking snapshot and viewing processes
https://docs.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes

*	Reading and printing memory
https://www.codeproject.com/Articles/4929/Toolhelp32ReadProcessMemory	  	 

*	String to dword
http://www.cplusplus.com/forum/beginner/95723/

## To compile
1. Open solution (.sln) in Visual Studio 2017. 
2. Make sure that your debug platform is 64x (otherwise you would see error code 299 often).
3. Build solution.


## To test run
Run "Kernel Debugging.exe".
(Only tested in Windows10 64-bit)
