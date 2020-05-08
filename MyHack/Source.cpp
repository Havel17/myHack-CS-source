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
const DWORD plr_list_offset = 0x003B51C4; // значение хуйня, надо искать другую
const DWORD hp_offset = 0xDA4; // зависит от верхней хуиты

bool wh=false;
int hp;
DWORD plr_addr;
DWORD client_dll_base; // во вторых
DWORD server_dll_base; //во первых1
HANDLE hProcess; // открыть процесс?
PROCESSENTRY32 pe32; // типа переменная в которой хранится адрес процесса

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
	DWORD addrc = client_dll_base + plr_list_offset; // ссылка на структуру с игроками client.dll
	//DWORD addrs = server_dll_base + plr_list_offset; // ссылка на струткуру с игроками server.dll
	read_bytes((LPCVOID)addrc, 4, &plr_addr); // считываем со структуры игрока
	DWORD hpa = plr_addr + hp_offset; // узнаем адресс хп игрока выше
	
//		system("cls");
		read_bytes((LPCVOID)hpa, 4, &hp); // считываем хп игрока и записываем в переменную hp
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
//прочитать тданные из дллки
int read_bytes(LPCVOID addr, int num, void* buf) {
	SIZE_T sz = 0;
	int r = ReadProcessMemory(hProcess, addr, buf, num, &sz);  // та самая считывалка, не ебу как работает
	if (r == 0 || sz == 0) {
//		printf("RPM error, %08X\n", GetLastError());
		return 0;
	}
	return 1;
}
//========================================================================
// отлавливание процесса hl2.exe
HANDLE HandleProcessa(const char* szFilename) {
	//от плюсиков до плюсиков исчет процесс, тоже не ебу как работает, но работает
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	HANDLE hProcessSnap;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return false;
	pe32.dwSize = sizeof(PROCESSENTRY32);


	do {
		if (!Process32Next(hProcessSnap, &pe32)) // перечисляем процессы
			return FALSE;
	} while (lstrcmpi(pe32.szExeFile, szFilename)); // ищем нужный процесс. Его PID будет в поле pe32.th32ProcessID              

	
	CloseHandle(hProcessSnap);
//	if (pe32.th32ProcessID) {
//		printf("Counter-Strike Source found! [Pid: %d] ", (DWORD)pe32.th32ProcessID);
//	}
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pe32.th32ProcessID); // открытие  процесса?

	//нахождение client.dll и server.dll (захардкодил только на client.dll)
	//=======================================================================================
	HMODULE hMods[1024];
	int i;
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &pe32.th32ProcessID) == 0) {// if {хз че тут, просто скопировал}
//		printf("enumprocessmodules failed, %08X\n", GetLastError());
	}
	else {																				//else находим длллки в процессе
		for (i = 0; i < (pe32.th32ProcessID / sizeof(HMODULE)); i++) {
			TCHAR szModName[MAX_PATH];													// переменная, где будет хранится названия дллки, которые мы прогоняем через цикл
			if (GetModuleFileNameEx(hProcess, hMods[i], szModName,            
				sizeof(szModName) / sizeof(TCHAR))) {									//берет ту самую дллку из процесса(процесс, модуль(типа смещение в памяти что бы найти длл),буфер,size буфера, что бы функция дала true)
//				printf("Pizda brone %s\n", szModName);									// очередной дебаг
				if (strstr(szModName, SERVERDLL) != 0) {								// во первых сверяем буфер с server.dll
//					printf("server.dll base: %08X\n", hMods[i]);
					server_dll_base = (DWORD)hMods[i];									// в третих типа если да то записываем ее
					
				}
				if (strstr(szModName, CLIENTDLL) != 0) {								// во вторых смотри хуйню во первых
//					printf("client.dll base: %08X\n", hMods[i]);			
					client_dll_base = (DWORD)hMods[i];									//иди нахуй
					break;
				}
			}
		}
	}
	//================================================================================================
	return hProcess;
};