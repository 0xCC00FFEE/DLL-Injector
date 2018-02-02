#include<Windows.h>
#include<TlHelp32.h>
#include<iostream>
#include<string>

#define DLLPATHLEN 256

class Injector
{
private:
	DWORD dwProcessID;
	PROCESSENTRY32 pe32;
	HANDLE hProcSnapShot;
	HANDLE hProcHandle;
	HANDLE hThreadHandle;
	LPVOID DllAllocateAddress;
	FARPROC LoadLibraryAddress;
	LPTHREAD_START_ROUTINE startExecutionAddress;
	TCHAR DllAbsPath[DLLPATHLEN];

public:
	Injector(LPCTSTR DllPath, const char* ProcessName) :
		dwProcessID(0),
		pe32({sizeof(PROCESSENTRY32)}), 
		hProcSnapShot(NULL),
		hProcHandle(NULL),
		hThreadHandle(NULL),
		DllAllocateAddress(NULL),
		LoadLibraryAddress(NULL),
		startExecutionAddress(NULL)
	{
		// Getting the absolute path of the DLL file
		if (!GetFullPathName(DllPath, DLLPATHLEN, DllAbsPath, NULL))
			Debug(0x01);

		// Start looking for our target process
		hProcSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (hProcSnapShot == INVALID_HANDLE_VALUE)
			Debug(0x02);
		
		if (!Process32First(hProcSnapShot, &pe32))
			Debug(0x03);

		do
		{
			if (!strcmp(pe32.szExeFile, ProcessName))
			{
				dwProcessID = pe32.th32ProcessID;
				CloseHandle(hProcSnapShot);
				break;
			}
		} while (Process32Next(hProcSnapShot, &pe32));
	}

	~Injector()
	{
		CloseHandle(hProcHandle);
		VirtualFreeEx(hProcHandle, DllAllocateAddress, strlen(DllAbsPath), MEM_DECOMMIT | MEM_RELEASE);
	}

	void Debug(BYTE ErrorCode = 0x00)
	{

		switch (ErrorCode)
		{
		case 0x01:
			std::cout << "[!] Error: Couldn't get DLL Absolute Path!" << std::endl;
			break;
		case 0x02:
			std::cout << "[!] Error: Couldn't open process snapshot handle!" << std::endl;
			break;
		case 0x03:
			std::cout << "[!] Error: Couldn't retrieve information about the first process in system snapshot!" << std::endl;
			break;
		case 0x04:
			std::cout << "[!] Error: Couldn't open process memory space!" << std::endl;
			break;
		case 0x05:
			break;
		case 0x06:
			std::cout << "[!] Error: Couldn't write DLL into allocated memory space!" << std::endl;
			break;
		case 0x07:
			std::cout << "[!] Error: Couldn't load LoadLibrary function!" << std::endl;
			break;
		case 0x08:
			std::cout << "[!] Error: Couldn't create new thread!" << std::endl;
			break;
		default:
			std::cout << "Process ID: " << dwProcessID << std::endl;
			std::cout << "DLL Absolute Path: " << DllAbsPath << std::endl;
			return;
		}
		system("PAUSE");
		exit(-1);
	}

	bool InjectDll()
	{
		// Attach to process memory space
		hProcHandle = OpenProcess(PROCESS_ALL_ACCESS, false, dwProcessID);

		if (!hProcHandle)
			Debug(0x04);

		// Allocate memory space to inject the DLL absolute path in target process
		DllAllocateAddress = VirtualAllocEx(hProcHandle, NULL, strlen(DllAbsPath), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

		if (!DllAllocateAddress)
			Debug(0x05);

		// Write DLL into target allocated memory space
		if (!WriteProcessMemory(hProcHandle, DllAllocateAddress, DllAbsPath, strlen(DllAbsPath), NULL))
			Debug(0x06);

		// Load the written DLL using LoadLibrary
		LoadLibraryAddress = GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryA");

		if (!LoadLibraryAddress)
			Debug(0x07);


		// Create and start executing new thread
		startExecutionAddress = (LPTHREAD_START_ROUTINE)LoadLibraryAddress;

		hThreadHandle = CreateRemoteThread(hProcHandle, NULL, 0, startExecutionAddress, DllAllocateAddress, 0, NULL);

		if (!hThreadHandle)
			Debug(0x08);

		Debug();
		std::cout << "[~] Injected Successfully!" << std::endl;
		system("PAUSE");

		return true;
	}

};

int main()
{
	LPCTSTR Dll_Path = "Absolute-Path-Of-Your-DLL-File";
	const char *ProcessName = "ac_client.exe";				// For Assault Cube, you can change that to anything you want
	
	Injector InjectorObject(Dll_Path, ProcessName);
	InjectorObject.InjectDll();
	return 0;
}