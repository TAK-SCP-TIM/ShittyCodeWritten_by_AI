// ForegroundProcessController_Interactive.cpp
// 功能：枚举所有可见窗口（包含本程序自身），让用户选择操作
// 优化：日志只记录用户选择的应用及操作，不再记录完整窗口列表
// 日志路径：%TEMP%\ForegroundProcessController.log（UTF-8编码，追加模式）
// 编译：g++ -mconsole ForegroundProcessController_Interactive.cpp -o controller.exe -lshell32 -static-libgcc -static-libstdc++

#include <Windows.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <exception>
#include <stdexcept>
#include <ctime>
#include <cstdarg>
#include <fstream>

#pragma comment(lib, "shell32.lib")

// 定义 NtSuspendProcess / NtResumeProcess 类型
typedef LONG(NTAPI* pNtSuspendProcess)(HANDLE);
typedef LONG(NTAPI* pNtResumeProcess)(HANDLE);

pNtSuspendProcess NtSuspendProcess = nullptr;
pNtResumeProcess NtResumeProcess = nullptr;

// 当前进程 ID
DWORD g_currentPid = 0;

// 窗口信息结构体
struct WindowInfo {
	HWND hWnd;
	DWORD pid;
	std::wstring title;
	std::wstring processName;
};

// 日志文件指针
FILE* g_logFile = nullptr;

// 初始化日志文件（在临时文件夹中创建/追加）
bool InitLogFile()
{
	wchar_t tempPath[MAX_PATH];
	if (GetTempPathW(MAX_PATH, tempPath) == 0)
		return false;
	
	std::wstring logPath = std::wstring(tempPath) + L"ForegroundProcessController.log";
	g_logFile = _wfopen(logPath.c_str(), L"a, ccs=UTF-8");
	if (!g_logFile)
		return false;
	
	return true;
}

// 关闭日志文件
void CloseLogFile()
{
	if (g_logFile)
	{
		fclose(g_logFile);
		g_logFile = nullptr;
	}
}

// 获取当前时间字符串（格式：YYYY-MM-DD HH:MM:SS）
std::wstring GetCurrentTimeString()
{
	time_t now = time(nullptr);
	struct tm tmbuf;
	localtime_s(&tmbuf, &now);
	wchar_t buf[64];
	wcsftime(buf, 64, L"%Y-%m-%d %H:%M:%S", &tmbuf);
	return buf;
}

// 日志记录函数（格式同 wprintf，自动添加时间戳和换行）
void Log(const wchar_t* format, ...)
{
	if (!g_logFile)
		return;
	
	std::wstring timestamp = GetCurrentTimeString();
	
	va_list args;
	va_start(args, format);
	wchar_t msgBuf[4096];
	vswprintf(msgBuf, 4096, format, args);
	va_end(args);
	
	fwprintf(g_logFile, L"[%ls] %ls\n", timestamp.c_str(), msgBuf);
	fflush(g_logFile);
}

// 初始化 ntdll 函数
void InitNtFunctions()
{
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	if (hNtdll)
	{
		NtSuspendProcess = (pNtSuspendProcess)GetProcAddress(hNtdll, "NtSuspendProcess");
		NtResumeProcess = (pNtResumeProcess)GetProcAddress(hNtdll, "NtResumeProcess");
	}
}

// 检查是否以管理员身份运行
bool IsRunAsAdmin()
{
	BOOL fIsElevated = FALSE;
	HANDLE hToken = nullptr;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION elevation;
		DWORD dwSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &elevation, dwSize, &dwSize))
			fIsElevated = elevation.TokenIsElevated;
		CloseHandle(hToken);
	}
	return fIsElevated ? true : false;
}

// 以管理员权限重启自身（无参数模式）
bool RunSelfAsAdmin()
{
	wchar_t szPath[MAX_PATH];
	if (GetModuleFileNameW(nullptr, szPath, MAX_PATH) == 0)
	{
		wprintf(L"获取自身路径失败，错误码: %lu\n", GetLastError());
		Log(L"获取自身路径失败，错误码: %lu", GetLastError());
		return false;
	}
	
	wprintf(L"正在请求管理员权限...\n");
	Log(L"正在请求管理员权限...");
	SHELLEXECUTEINFOW sei = { sizeof(sei) };
	sei.lpVerb = L"runas";
	sei.lpFile = szPath;
	sei.lpParameters = L"";
	sei.nShow = SW_SHOWNORMAL;
	
	if (!ShellExecuteExW(&sei))
	{
		DWORD err = GetLastError();
		if (err == ERROR_CANCELLED)
		{
			wprintf(L"用户取消了UAC提示。\n");
			Log(L"用户取消了UAC提示。");
		}
		else
		{
			wprintf(L"提权失败，错误码: %lu\n", err);
			Log(L"提权失败，错误码: %lu", err);
		}
		return false;
	}
	return true;
}

// 关闭进程
bool CloseProcess(DWORD pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (!hProcess)
	{
		wprintf(L"无法打开进程（PID: %lu），错误码: %lu\n", pid, GetLastError());
		Log(L"无法打开进程（PID: %lu），错误码: %lu", pid, GetLastError());
		return false;
	}
	BOOL result = TerminateProcess(hProcess, 0);
	CloseHandle(hProcess);
	if (!result)
	{
		wprintf(L"终止进程失败，错误码: %lu\n", GetLastError());
		Log(L"终止进程失败，错误码: %lu", GetLastError());
		return false;
	}
	return true;
}

// 冻结进程
bool FreezeProcess(DWORD pid)
{
	if (!NtSuspendProcess)
	{
		wprintf(L"NtSuspendProcess 不可用，无法冻结。\n");
		Log(L"NtSuspendProcess 不可用，无法冻结。");
		return false;
	}
	HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
	if (!hProcess)
	{
		wprintf(L"无法打开进程（PID: %lu），错误码: %lu\n", pid, GetLastError());
		Log(L"无法打开进程（PID: %lu），错误码: %lu", pid, GetLastError());
		return false;
	}
	LONG status = NtSuspendProcess(hProcess);
	CloseHandle(hProcess);
	if (status != 0)
	{
		wprintf(L"冻结进程失败，NTSTATUS: 0x%08lx\n", status);
		Log(L"冻结进程失败，NTSTATUS: 0x%08lx", status);
		return false;
	}
	return true;
}

// 解冻进程
bool UnfreezeProcess(DWORD pid)
{
	if (!NtResumeProcess)
	{
		wprintf(L"NtResumeProcess 不可用，无法解冻。\n");
		Log(L"NtResumeProcess 不可用，无法解冻。");
		return false;
	}
	HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
	if (!hProcess)
	{
		wprintf(L"无法打开进程（PID: %lu），错误码: %lu\n", pid, GetLastError());
		Log(L"无法打开进程（PID: %lu），错误码: %lu", pid, GetLastError());
		return false;
	}
	LONG status = NtResumeProcess(hProcess);
	CloseHandle(hProcess);
	if (status != 0)
	{
		wprintf(L"解冻进程失败，NTSTATUS: 0x%08lx\n", status);
		Log(L"解冻进程失败，NTSTATUS: 0x%08lx", status);
		return false;
	}
	return true;
}

// 以管理员权限重启目标进程并关闭原进程
bool RunAsAdminAndCloseOriginal(DWORD pid)
{
	wchar_t szPath[MAX_PATH];
	DWORD dwSize = MAX_PATH;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!hProcess)
	{
		wprintf(L"无法打开进程（PID: %lu），错误码: %lu\n", pid, GetLastError());
		Log(L"无法打开进程（PID: %lu），错误码: %lu", pid, GetLastError());
		return false;
	}
	BOOL success = QueryFullProcessImageNameW(hProcess, 0, szPath, &dwSize);
	CloseHandle(hProcess);
	if (!success)
	{
		wprintf(L"获取进程路径失败，错误码: %lu\n", GetLastError());
		Log(L"获取进程路径失败，错误码: %lu", GetLastError());
		return false;
	}
	
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	std::wstring cmdLine = L"\"" + std::wstring(szPath) + L"\"";
	if (!CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
	{
		wprintf(L"启动新进程失败，错误码: %lu\n", GetLastError());
		Log(L"启动新进程失败，错误码: %lu", GetLastError());
		return false;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	
	if (!CloseProcess(pid))
		wprintf(L"新进程已启动，但无法关闭原进程。\n");
	Log(L"新进程已启动，但无法关闭原进程。");
	return true;
}

// 切换（激活）到指定窗口
bool SwitchToWindow(HWND hWnd)
{
	if (IsIconic(hWnd))
		ShowWindow(hWnd, SW_RESTORE);
	if (SetForegroundWindow(hWnd))
		return true;
	BringWindowToTop(hWnd);
	return SetForegroundWindow(hWnd) != FALSE;
}

// 切换窗口的置顶状态
void ToggleTopmost(HWND hWnd)
{
	LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
	if (exStyle & WS_EX_TOPMOST)
	{
		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		wprintf(L"已取消置顶。\n");
		Log(L"已取消置顶 (HWND: %p)", hWnd);
	}
	else
	{
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		wprintf(L"已设置为置顶。\n");
		Log(L"已设置为置顶 (HWND: %p)", hWnd);
	}
}

// 根据PID获取进程名
std::wstring GetProcessNameFromPID(DWORD pid)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) return L"";
	PROCESSENTRY32W pe = { sizeof(pe) };
	if (Process32FirstW(hSnapshot, &pe))
	{
		do {
			if (pe.th32ProcessID == pid)
			{
				CloseHandle(hSnapshot);
				return pe.szExeFile;
			}
		} while (Process32NextW(hSnapshot, &pe));
	}
	CloseHandle(hSnapshot);
	return L"";
}

// 枚举窗口的回调函数（包含自身进程）
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	std::vector<WindowInfo>* pList = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
	
	if (!IsWindowVisible(hWnd))
		return TRUE;
	
	wchar_t title[256];
	int len = GetWindowTextW(hWnd, title, 256);
	if (len == 0)
		return TRUE;
	
	DWORD pid;
	GetWindowThreadProcessId(hWnd, &pid);
	if (pid == 0)
		return TRUE;
	
	std::wstring processName = GetProcessNameFromPID(pid);
	if (processName.empty())
		processName = L"<未知>";
	
	WindowInfo info;
	info.hWnd = hWnd;
	info.pid = pid;
	info.title = title;
	info.processName = processName;
	pList->push_back(info);
	
	return TRUE;
}

// 检查并处理 Alt+F4 退出
bool HandleAltF4(int ch)
{
	if (ch == 0 || ch == 0xE0)
	{
		int scan = _getwch();
		if (scan == 0x3E)
		{
			if (GetAsyncKeyState(VK_MENU) & 0x8000)
			{
				wprintf(L"\nAlt+F4 按下，退出程序。\n");
				Log(L"用户按下 Alt+F4 退出程序。");
				exit(0);
			}
		}
		return true;
	}
	return false;
}

// 读取一行数字输入（支持退格），返回宽字符串，空字符串表示取消/无效
std::wstring ReadNumberInput()
{
	std::wstring input;
	while (true)
	{
		wchar_t ch = _getwch();
		if (HandleAltF4(ch))
			continue;
		
		if (ch == L'\r')
		{
			break;
		}
		else if (ch == L'\b' || ch == 127)
		{
			if (!input.empty())
			{
				input.pop_back();
				putwchar(L'\b');
				putwchar(L' ');
				putwchar(L'\b');
			}
		}
		else if (ch >= L'0' && ch <= L'9')
		{
			input.push_back(ch);
			putwchar(ch);
		}
		else if (ch == 0 || ch == 0xE0)
		{
			_getwch();
		}
		else
		{
			// 忽略
		}
	}
	return input;
}

// 列出所有窗口并让用户选择操作
void ListAndSelectWindow()
{
	while (_kbhit()) _getwch();
	
	std::vector<WindowInfo> windows;
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));
	
	if (windows.empty())
	{
		wprintf(L"未找到任何可见窗口。\n");
		Log(L"未找到任何可见窗口。");
		return;
	}
	
	wprintf(L"\n===== 当前可见窗口列表 =====\n");
	// 不再记录整个列表到日志
	for (size_t i = 0; i < windows.size(); ++i)
	{
		wprintf(L"[%2zu] PID: %-6lu | 进程: %-20ls | 标题: %ls\n",
			i + 1,
			windows[i].pid,
			windows[i].processName.c_str(),
			windows[i].title.c_str());
	}
	wprintf(L"============================\n");
	
	wprintf(L"请选择窗口序号 (1-%zu)，输入 0 刷新列表，输入 q 退出: ", windows.size());
	
	wchar_t first = _getwch();
	if (HandleAltF4(first))
		return;
	
	if (first == L'q' || first == L'Q')
	{
		wprintf(L"%lc\n", first);
		wprintf(L"退出程序。\n");
		Log(L"用户选择退出程序。");
		exit(0);
	}
	if (first == L'0')
	{
		wprintf(L"0\n");
		Log(L"用户选择刷新列表。");
		return;
	}
	if (first == 0 || first == 0xE0)
	{
		_getwch();
		wprintf(L"\n");
		return;
	}
	if (first < L'1' || first > L'9')
	{
		wprintf(L"\n");
		return;
	}
	
	std::wstring input;
	input.push_back(first);
	wprintf(L"%lc", first);
	
	while (true)
	{
		wchar_t ch = _getwch();
		if (HandleAltF4(ch))
			continue;
		
		if (ch == L'\r')
		{
			wprintf(L"\n");
			break;
		}
		else if (ch == L'\b' || ch == 127)
		{
			if (!input.empty())
			{
				input.pop_back();
				putwchar(L'\b');
				putwchar(L' ');
				putwchar(L'\b');
			}
		}
		else if (ch >= L'0' && ch <= L'9')
		{
			input.push_back(ch);
			putwchar(ch);
		}
		else if (ch == 0 || ch == 0xE0)
		{
			_getwch();
		}
		else
		{
			// 忽略
		}
	}
	
	int index = _wtoi(input.c_str());
	if (index < 1 || index > (int)windows.size())
	{
		wprintf(L"序号超出范围，按任意键重新选择...\n");
		Log(L"用户输入序号 %d 超出范围 (1-%zu)", index, windows.size());
		_getwch();
		return;
	}
	
	// 记录用户选择的窗口
	Log(L"用户选择窗口序号 %d (PID: %lu, 进程: %ls, 标题: %ls)", index, windows[index-1].pid, windows[index-1].processName.c_str(), windows[index-1].title.c_str());
	
	HWND hWnd = windows[index-1].hWnd;
	DWORD pid = windows[index-1].pid;
	wprintf(L"\n选择操作：\n");
	wprintf(L"  1. 关闭进程\n");
	wprintf(L"  2. 冻结进程\n");
	wprintf(L"  3. 解冻进程\n");
	wprintf(L"  4. 以管理员身份重启\n");
	wprintf(L"  5. 切换到此窗口\n");
	wprintf(L"  6. 置顶/取消置顶\n");
	wprintf(L"  7. 返回重新选择\n");
	wprintf(L"请输入选项 (1-7): ");
	
	wchar_t op = 0;
	while (true)
	{
		op = _getwch();
		if (HandleAltF4(op))
			continue;
		
		if (op >= L'1' && op <= L'7')
		{
			wprintf(L"%lc\n", op);
			break;
		}
		else if (op == L'q' || op == L'Q')
		{
			wprintf(L"%lc\n", op);
			wprintf(L"退出程序。\n");
			Log(L"用户选择退出程序。");
			exit(0);
		}
		else if (op == 0 || op == 0xE0)
		{
			_getwch();
		}
		else
		{
			// 忽略
		}
	}
	
	Log(L"用户选择操作 %lc", op);
	
	bool needRefresh = false;
	switch (op)
	{
	case L'1':
		if (CloseProcess(pid))
		{
			wprintf(L"进程已关闭。\n");
			Log(L"进程 (PID: %lu) 已关闭。", pid);
		}
		needRefresh = true;
		break;
	case L'2':
		if (FreezeProcess(pid))
		{
			wprintf(L"进程已冻结。\n");
			Log(L"进程 (PID: %lu) 已冻结。", pid);
		}
		needRefresh = true;
		break;
	case L'3':
		if (UnfreezeProcess(pid))
		{
			wprintf(L"进程已解冻。\n");
			Log(L"进程 (PID: %lu) 已解冻。", pid);
		}
		needRefresh = true;
		break;
	case L'4':
		if (RunAsAdminAndCloseOriginal(pid))
		{
			wprintf(L"进程已以管理员身份重新启动。\n");
			Log(L"进程 (PID: %lu) 已以管理员身份重新启动。", pid);
		}
		needRefresh = true;
		break;
	case L'5':
		if (SwitchToWindow(hWnd))
		{
			wprintf(L"已切换到该窗口。\n");
			Log(L"已切换到窗口 (HWND: %p)", hWnd);
		}
		else
		{
			wprintf(L"切换窗口失败。\n");
			Log(L"切换窗口失败 (HWND: %p)", hWnd);
		}
		needRefresh = true;
		break;
	case L'6':
		ToggleTopmost(hWnd);
		needRefresh = true;
		break;
	case L'7':
		needRefresh = true;
		break;
	}
	
	if (needRefresh)
	{
		wprintf(L"\n按任意键返回窗口列表...\n");
		Log(L"等待用户按键返回窗口列表...");
		ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
		wprintf(L"（控制台已最小化，请点击任务栏图标激活后继续）\n");
		while (true)
		{
			if (_kbhit())
			{
				int ch = _getwch();
				if (HandleAltF4(ch))
					continue;
				break;
			}
			Sleep(100);
		}
		ShowWindow(GetConsoleWindow(), SW_RESTORE);
		system("cls");
	}
}

// 确保有控制台窗口并设置 UTF-16 输出模式
void EnsureConsole()
{
	if (GetConsoleWindow() == NULL)
	{
		AllocConsole();
		FILE* fDummy;
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);
	}
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
	_setmode(_fileno(stdin), _O_U16TEXT);
}

// 交互模式主循环
void InteractiveMode()
{
	wprintf(L"进入交互模式。将列出所有可见窗口（包含本程序自身）。\n");
	wprintf(L"注意：无标题窗口已被过滤。\n\n");
	Log(L"进入交互模式。");
	
	while (true)
	{
		ListAndSelectWindow();
	}
}

// 主函数，添加全局异常处理和日志初始化
int main()
{
	if (!InitLogFile())
	{
		wprintf(L"警告：无法创建日志文件。\n");
	}
	
	Log(L"程序启动。");
	
	try
	{
		g_currentPid = GetCurrentProcessId();
		EnsureConsole();
		
		wprintf(L"程序已启动。\n");
		InitNtFunctions();
		
		if (!IsRunAsAdmin())
		{
			wprintf(L"当前不是管理员权限，正在请求提升...\n");
			Log(L"当前不是管理员权限，正在请求提升...");
			if (RunSelfAsAdmin())
			{
				wprintf(L"提权请求已发送，原进程退出。\n");
				Log(L"提权请求已发送，原进程退出。");
				wprintf(L"按任意键退出...\n");
				_getwch();
				return 0;
			}
			else
			{
				wprintf(L"无法获得管理员权限，程序退出。\n");
				Log(L"无法获得管理员权限，程序退出。");
				wprintf(L"按任意键退出...\n");
				_getwch();
				return 1;
			}
		}
		
		wprintf(L"已获得管理员权限，进入交互模式。\n");
		Log(L"已获得管理员权限，进入交互模式。");
		InteractiveMode();
	}
	catch (const std::exception& e)
	{
		wprintf(L"\n发生标准异常: %hs\n", e.what());
		Log(L"发生标准异常: %hs", e.what());
		wprintf(L"按任意键退出...\n");
		_getwch();
		CloseLogFile();
		return 2;
	}
	catch (...)
	{
		wprintf(L"\n发生未知异常！\n");
		Log(L"发生未知异常！");
		wprintf(L"按任意键退出...\n");
		_getwch();
		CloseLogFile();
		return 3;
	}
	
	CloseLogFile();
	return 0;
}
