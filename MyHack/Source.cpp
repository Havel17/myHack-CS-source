#include <stdio.h> 
#include <Windows.h> 
#include <Winnt.h> 
#include <tlhelp32.h>
#include <Psapi.h>
#include <time.h>

#define WINDOWNAME "hl2.exe"
#define PLRSZ 0x10
#define CLIENTDLL "client.dll"
#define SERVERDLL "server.dll"
#define ENGINEDLL "engine.dll"

//const DWORD plr_list_offset = 0X003FB2F0;
const DWORD plr_list_offset = 0x003B51C4; // �������� �����, ���� ������ ������
const DWORD hp_offset = 0xDA4; // ������� �� ������� �����

bool wh=false;
int hp;
DWORD plr_addr;
DWORD client_dll_base; // �� ������
DWORD server_dll_base; //�� ������1
HANDLE hProcess; // ������� �������?
PROCESSENTRY32 pe32; // ���� ���������� � ������� �������� ����� ��������

HANDLE HandleProcessa(const char* szFilename);
int read_bytes(LPCVOID addr, int num, void* buf);
void esp();
void Init();

int main() {
	Init();
	return 0;
}

void Init() {
	hProcess = HandleProcessa(WINDOWNAME);
	esp();
	CloseHandle(hProcess);
}

//===================================================================================
void esp() {
	while (true) {
	DWORD addrc = client_dll_base + plr_list_offset; // ������ �� ��������� � �������� client.dll
	//DWORD addrs = server_dll_base + plr_list_offset; // ������ �� ��������� � �������� server.dll
	read_bytes((LPCVOID)addrc, 4, &plr_addr); // ��������� �� ��������� ������
	DWORD hpa = plr_addr + hp_offset; // ������ ������ �� ������ ����
	
//		system("cls");
		read_bytes((LPCVOID)hpa, 4, &hp); // ��������� �� ������ � ���������� � ���������� hp
//		printf("Player hp: %d\n", hp);

		if (GetAsyncKeyState(VK_F8)) {
			wh = !wh;
		}
		if (wh) {
			if (hp > 0 && hp < 237 || hp > 239 && hp < 500) {
				keybd_event(MapVirtualKey(0x76, 0), 0x41, KEYEVENTF_EXTENDEDKEY, 0);     //https://www.delphiplus.org/apparatnye-sredstva-pc/sken-kody.html
				keybd_event(MapVirtualKey(0x76, 0), 0x41, KEYEVENTF_KEYUP, 0);			//http://vsokovikov.narod.ru/New_MSDN_API/Other/virtual_key_code.htm
				Sleep(50);
			}
		}
		Sleep(1);
	}
}
//=============================================================================
//��������� ������� �� �����
int read_bytes(LPCVOID addr, int num, void* buf) {
	SIZE_T sz = 0;
	int r = ReadProcessMemory(hProcess, addr, buf, num, &sz);  // �� ����� ����������, �� ��� ��� ��������
	if (r == 0 || sz == 0) {
//		printf("RPM error, %08X\n", GetLastError());
		return 0;
	}
	return 1;
}
//========================================================================
// ������������ �������� hl2.exe
HANDLE HandleProcessa(const char* szFilename) {
	//�� �������� �� �������� ����� �������, ���� �� ��� ��� ��������, �� ��������
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	HANDLE hProcessSnap;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return false;
	pe32.dwSize = sizeof(PROCESSENTRY32);


	do {
		if (!Process32Next(hProcessSnap, &pe32)) // ����������� ��������
			return FALSE;
	} while (lstrcmpi(pe32.szExeFile, szFilename)); // ���� ������ �������. ��� PID ����� � ���� pe32.th32ProcessID              

	
	CloseHandle(hProcessSnap);
//	if (pe32.th32ProcessID) {
//		printf("Counter-Strike Source found! [Pid: %d] ", (DWORD)pe32.th32ProcessID);
//	}
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pe32.th32ProcessID); // ��������  ��������?

	//���������� client.dll � server.dll (����������� ������ �� client.dll)
	//=======================================================================================
	HMODULE hMods[1024];
	int i;
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &pe32.th32ProcessID) == 0) {// if {�� �� ���, ������ ����������}
//		printf("enumprocessmodules failed, %08X\n", GetLastError());
	}
	else {																				//else ������� ������ � ��������
		for (i = 0; i < (pe32.th32ProcessID / sizeof(HMODULE)); i++) {
			TCHAR szModName[MAX_PATH];													// ����������, ��� ����� �������� �������� �����, ������� �� ��������� ����� ����
			if (GetModuleFileNameEx(hProcess, hMods[i], szModName,            
				sizeof(szModName) / sizeof(TCHAR))) {									//����� �� ����� ����� �� ��������(�������, ������(���� �������� � ������ ��� �� ����� ���),�����,size ������, ��� �� ������� ���� true)
//				printf("Pizda brone %s\n", szModName);									// ��������� �����
				if (strstr(szModName, SERVERDLL) != 0) {								// �� ������ ������� ����� � server.dll
//					printf("server.dll base: %08X\n", hMods[i]);
					server_dll_base = (DWORD)hMods[i];									// � ������ ���� ���� �� �� ���������� ��
					
				}
				if (strstr(szModName, CLIENTDLL) != 0) {								// �� ������ ������ ����� �� ������
//					printf("client.dll base: %08X\n", hMods[i]);			
					client_dll_base = (DWORD)hMods[i];									//��� �����
					break;
				}
			}
		}
	}
	//================================================================================================
	return hProcess;
};