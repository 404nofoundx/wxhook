#include "stdafx.h"
#include "CInjectTools.h"
#include <direct.h>
#include <stdlib.h>
#include <TlHelp32.h>
#include <stdio.h>

#define WECHAT_PROCESS_NAME "WeChat.exe"
#define DLLNAME "WeChatHelper.dll"


//************************************************************
// ��������: ProcessNameFindPID
// ����˵��: ͨ���������ҵ�����ID
// ��    ��: GuiShou
// ʱ    ��: 2019/6/30
// ��    ��: ProcessName ������
// �� �� ֵ: DWORD ����PID
//************************************************************
DWORD ProcessNameFindPID(const char* ProcessName)
{
	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(hProcess, &pe32) == TRUE)
	{
		do
		{
			USES_CONVERSION;
			if (strcmp(ProcessName, W2A(pe32.szExeFile)) == 0)
			{
				return pe32.th32ProcessID;
			}
		} while (Process32Next(hProcess, &pe32));
	}
	return 0;
}




//************************************************************
// ��������: InjectDll
// ����˵��: ע��DLL
// ��    ��: GuiShou
// ʱ    ��: 2019/6/30
// ��    ��: void
// �� �� ֵ: void
//************************************************************
BOOL InjectDll()
{
	//��ȡ��ǰ����Ŀ¼�µ�dll
	char szPath[MAX_PATH] = { 0 };
	char* buffer = NULL;
	if ((buffer = _getcwd(NULL, 0)) == NULL)
	{
		MessageBoxA(NULL, "��ȡ��ǰ����Ŀ¼ʧ��", "����", 0);
		return FALSE;
	}
	sprintf_s(szPath, "D:\\github\\wxhook\\WeChatRobot\\Debug\\WeChatHelper.dll");

	//��ȡ΢��Pid
	DWORD dwPid = ProcessNameFindPID(WECHAT_PROCESS_NAME);
	if (dwPid == 0)
	{
		MessageBoxA(NULL, "û���ҵ�΢�Ž��� ��������΢��", "����", 0);
		return FALSE;
	}
	//���dll�Ƿ��Ѿ�ע��
	if (CheckIsInject(dwPid))
	{
		//�򿪽���
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
		if (hProcess == NULL)
		{
			MessageBoxA(NULL, "���̴�ʧ��", "����", 0);
			return FALSE;
		}
		//��΢�Ž����������ڴ�
		LPVOID pAddress = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (pAddress == NULL)
		{
			MessageBoxA(NULL, "�ڴ����ʧ��", "����", 0);
			return FALSE;
		}
		//д��dll·����΢�Ž���
		DWORD dwWrite = 0;
		if (WriteProcessMemory(hProcess, pAddress, szPath, MAX_PATH, &dwWrite) == 0)
		{
			MessageBoxA(NULL, "·��д��ʧ��", "����", 0);
			return FALSE;
		}

		//��ȡLoadLibraryA������ַ
		FARPROC pLoadLibraryAddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
		if (pLoadLibraryAddress == NULL)
		{
			MessageBoxA(NULL, "��ȡLoadLibraryA������ַʧ��", "����", 0);
			return FALSE;
		}
		//Զ���߳�ע��dll
		HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryAddress, pAddress, 0, NULL);
		if (hRemoteThread == NULL)
		{
			MessageBoxA(NULL, "Զ���߳�ע��ʧ��", "����", 0);
			return FALSE;
		}

		CloseHandle(hRemoteThread);
		CloseHandle(hProcess);
	}
	else
	{
		MessageBoxA(NULL, "dll�Ѿ�ע�룬�����ظ�ע��", "��ʾ", 0);
		return FALSE;
	}
	 
	return TRUE;	
}

//************************************************************
// ��������: CheckIsInject
// ����˵��: ����Ƿ��Ѿ�ע��dll
// ��    ��: GuiShou
// ʱ    ��: 2019/6/30
// ��    ��: void
// �� �� ֵ: BOOL 
//************************************************************
BOOL CheckIsInject(DWORD dwProcessid)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	//��ʼ��ģ����Ϣ�ṹ��
	MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
	//����ģ����� 1 �������� 2 ����ID
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessid);
	//��������Ч�ͷ���false
	if (hModuleSnap == INVALID_HANDLE_VALUE)
	{
		MessageBoxA(NULL, "����ģ�����ʧ��", "����", MB_OK);
		return FALSE;
	}
	//ͨ��ģ����վ����ȡ��һ��ģ�����Ϣ
	if (!Module32First(hModuleSnap, &me32))
	{
		MessageBoxA(NULL, "��ȡ��һ��ģ�����Ϣʧ��", "����", MB_OK);
		//��ȡʧ����رվ��
		CloseHandle(hModuleSnap);
		return FALSE;
	}
	do
	{
		if (StrCmpW(me32.szModule,L"WeChatHelper.dll")==0)
		{
			return FALSE;
		}

	} while (Module32Next(hModuleSnap, &me32));
	return TRUE;
}



//************************************************************
// ��������: UnloadDll
// ����˵��: ж��DLL
// ��    ��: GuiShou
// ʱ    ��: 2019/6/30
// ��    ��: void
// �� �� ֵ: void
//************************************************************
void UnloadDll()
{
	//��ȡ΢��Pid
	DWORD dwPid = ProcessNameFindPID(WECHAT_PROCESS_NAME);
	if (dwPid == 0)
	{
		MessageBoxA(NULL, "û���ҵ�΢�Ž��̻���΢��û������", "����", 0);
		return;
	}

	//����ģ��
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
	MODULEENTRY32 ME32 = { 0 };
	ME32.dwSize = sizeof(MODULEENTRY32);
	BOOL isNext = Module32First(hSnap, &ME32);
	BOOL flag = FALSE;
	while (isNext)
	{
		USES_CONVERSION;
		if (strcmp(W2A(ME32.szModule), DLLNAME) == 0)
		{
			flag = TRUE;
			break;
		}
		isNext = Module32Next(hSnap, &ME32);
	}
	if (flag == FALSE)
	{
		MessageBoxA(NULL, "�Ҳ���Ŀ��ģ��", "����", 0);
		return;
	}

	//��Ŀ�����
	HANDLE hPro = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	//��ȡFreeLibrary������ַ
	FARPROC pFun = GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary");

	//����Զ���߳�ִ��FreeLibrary
	HANDLE hThread = CreateRemoteThread(hPro, NULL, 0, (LPTHREAD_START_ROUTINE)pFun, ME32.modBaseAddr, 0, NULL);
	if (!hThread)
	{
		MessageBoxA(NULL, "����Զ���߳�ʧ��", "����", 0);
		return;
	}

	CloseHandle(hSnap);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hPro);
	MessageBoxA(NULL, "dllж�سɹ�", "Tip", 0);
}