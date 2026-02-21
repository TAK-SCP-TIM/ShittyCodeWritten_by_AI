#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <windows.h>

// 启动模式枚举
enum LaunchMode {
	SINGLE_WINDOW,  // 同窗口，等待结束
	NEW_WINDOW      // 新窗口，不等待
};

// 全局调试模式标志（供错误处理函数使用）
bool g_debugMode = false;

// 获取当前可执行文件所在目录
std::string getExecutableDir() {
	char buffer[MAX_PATH];
	if (GetModuleFileNameA(NULL, buffer, MAX_PATH) == 0) {
		DWORD err = GetLastError();
		std::string errMsg = "获取可执行文件路径失败，错误码: " + std::to_string(err);
		std::cerr << errMsg << std::endl;
		// 无法记录日志，因为日志路径尚未确定，只能输出到控制台
		return ".\\";
	}
	std::string path(buffer);
	size_t pos = path.find_last_of("\\/");
	return (pos != std::string::npos) ? path.substr(0, pos + 1) : "";
}

// 去除字符串首尾空白
std::string trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

// 获取日志文件路径（在用户的 Temp 文件夹，按日期命名）
std::string getLogFilePath() {
	char tempPath[MAX_PATH];
	if (GetTempPathA(MAX_PATH, tempPath) == 0) {
		DWORD err = GetLastError();
		std::string warnMsg = "获取临时目录失败，错误码: " + std::to_string(err) + "，日志将保存在当前目录";
		std::cerr << warnMsg << std::endl;
		strcpy_s(tempPath, ".\\");
	}
	
	time_t now = time(nullptr);
	struct tm t;
	if (localtime_s(&t, &now) != 0) {
		std::string warnMsg = "获取当前时间失败，将使用固定日志文件名";
		std::cerr << warnMsg << std::endl;
		return std::string(tempPath) + "launcher_error.log";
	}
	char dateStr[20];
	strftime(dateStr, sizeof(dateStr), "%Y%m%d", &t);
	std::string filename = std::string("launcher_") + dateStr + ".log";
	return std::string(tempPath) + filename;
}

// 辅助函数：带颜色输出（仅在全局调试模式开启时使用指定颜色）
void coloredOutput(const std::string& message, WORD color) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole != INVALID_HANDLE_VALUE && g_debugMode) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
			WORD originalColor = csbi.wAttributes;
			SetConsoleTextAttribute(hConsole, color);
			std::cout << message;
			SetConsoleTextAttribute(hConsole, originalColor);
			return;
		}
	}
	std::cout << message;
}

// 写入日志（追加模式）
void logMessage(const std::string& logPath, const std::string& message) {
	std::ofstream log(logPath, std::ios::app);
	if (!log.is_open()) {
		// 无法写入日志时，输出到控制台警告
		std::cerr << "警告：无法写入日志文件 " << logPath << std::endl;
		return;
	}
	
	time_t now = time(nullptr);
	struct tm t;
	if (localtime_s(&t, &now) != 0) {
		log << "[unknown time] " << message << std::endl;
	} else {
		char timeStr[30];
		strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &t);
		log << "[" << timeStr << "] " << message << std::endl;
	}
	log.close();
}

// 读取配置文件 config.txt（与 exe 同目录）
bool readConfig(const std::string& configPath, bool& debugMode, LaunchMode& launchMode) {
	logMessage(getLogFilePath(), "开始读取配置文件: " + configPath);
	std::ifstream file(configPath);
	if (!file.is_open()) {
		logMessage(getLogFilePath(), "配置文件不存在，将使用默认值");
		return false;
	}
	
	std::string line;
	int lineNum = 0;
	while (std::getline(file, line)) {
		lineNum++;
		line = trim(line);
		if (line.empty() || line[0] == '#') continue;
		
		size_t eqPos = line.find('=');
		if (eqPos == std::string::npos) {
			std::string warnMsg = "配置文件格式错误（第" + std::to_string(lineNum) + "行），已忽略：" + line;
			coloredOutput(warnMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(getLogFilePath(), warnMsg);
			continue;
		}
		
		std::string key = trim(line.substr(0, eqPos));
		std::string value = trim(line.substr(eqPos + 1));
		
		if (key == "debug") {
			bool oldDebug = debugMode;
			if (value == "on" || value == "true" || value == "1")
				debugMode = true;
			else
				debugMode = false;
			logMessage(getLogFilePath(), "配置项 debug 从 " + std::string(oldDebug ? "on" : "off") + " 变为 " + std::string(debugMode ? "on" : "off"));
		}
		else if (key == "launch_mode") {
			LaunchMode oldMode = launchMode;
			if (value == "new_window")
				launchMode = NEW_WINDOW;
			else if (value == "single_window")
				launchMode = SINGLE_WINDOW;
			else {
				std::string warnMsg = "配置文件未知启动模式值（第" + std::to_string(lineNum) + "行），使用默认 single_window";
				coloredOutput(warnMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
				logMessage(getLogFilePath(), warnMsg);
				launchMode = SINGLE_WINDOW;
			}
			logMessage(getLogFilePath(), "配置项 launch_mode 从 " + std::string(oldMode == NEW_WINDOW ? "new_window" : "single_window") + " 变为 " + std::string(launchMode == NEW_WINDOW ? "new_window" : "single_window"));
		}
		else {
			std::string warnMsg = "配置文件包含未知键（第" + std::to_string(lineNum) + "行），已忽略：" + key;
			coloredOutput(warnMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(getLogFilePath(), warnMsg);
		}
	}
	
	if (file.bad()) {
		std::string errMsg = "读取配置文件时发生错误";
		coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
		logMessage(getLogFilePath(), errMsg);
	}
	file.close();
	logMessage(getLogFilePath(), "配置文件读取完成");
	return true;
}

// 创建默认配置文件（静默创建，若失败则输出警告）
void createDefaultConfig(const std::string& configPath) {
	logMessage(getLogFilePath(), "尝试创建默认配置文件: " + configPath);
	std::ofstream file(configPath);
	if (!file.is_open()) {
		DWORD err = GetLastError();
		std::string warnMsg = "无法创建默认配置文件 " + configPath + "，错误码: " + std::to_string(err);
		coloredOutput(warnMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
		logMessage(getLogFilePath(), warnMsg);
		return;
	}
	
	file << "# 调试模式默认状态：on 或 off\n";
	file << "debug = off\n";
	file << "# 启动模式：single_window 或 new_window\n";
	file << "launch_mode = single_window\n";
	if (!file.good()) {
		DWORD err = GetLastError();
		std::string errMsg = "写入默认配置文件时发生错误，错误码: " + std::to_string(err);
		coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
		logMessage(getLogFilePath(), errMsg);
	} else {
		logMessage(getLogFilePath(), "默认配置文件创建成功");
	}
	file.close();
}

// 保存当前配置到文件
void saveConfig(const std::string& configPath, bool debugMode, LaunchMode launchMode) {
	logMessage(getLogFilePath(), "尝试保存配置文件: " + configPath);
	std::ofstream file(configPath);
	if (!file.is_open()) {
		DWORD err = GetLastError();
		std::string warnMsg = "无法保存配置文件 " + configPath + "，错误码: " + std::to_string(err);
		coloredOutput(warnMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
		logMessage(getLogFilePath(), warnMsg);
		return;
	}
	
	file << "# 调试模式默认状态：on 或 off\n";
	file << "debug = " << (debugMode ? "on" : "off") << "\n";
	file << "# 启动模式：single_window 或 new_window\n";
	file << "launch_mode = " << (launchMode == NEW_WINDOW ? "new_window" : "single_window") << "\n";
	if (!file.good()) {
		DWORD err = GetLastError();
		std::string errMsg = "写入配置文件时发生错误，错误码: " + std::to_string(err);
		coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
		logMessage(getLogFilePath(), errMsg);
	} else {
		logMessage(getLogFilePath(), "配置文件保存成功");
	}
	file.close();
}

// 线程函数：检测ESC键，若按下则终止子进程（仅同窗口模式使用）
DWORD WINAPI EscMonitorThread(LPVOID lpParam) {
	HANDLE hProcess = (HANDLE)lpParam;
	std::string logPath = getLogFilePath(); // 线程中获取日志路径
	logMessage(logPath, "ESC监控线程启动");
	while (true) {
		DWORD exitCode;
		if (!GetExitCodeProcess(hProcess, &exitCode)) {
			DWORD err = GetLastError();
			std::string errMsg = "ESC监控线程无法获取进程状态，错误码: " + std::to_string(err);
			coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(logPath, errMsg);
			break;
		}
		if (exitCode != STILL_ACTIVE) {
			logMessage(logPath, "ESC监控线程检测到子进程已自然结束，退出监控");
			break;
		}
		
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			logMessage(logPath, "ESC键被按下，尝试强制终止子进程");
			if (!TerminateProcess(hProcess, 1)) {
				DWORD err = GetLastError();
				std::string errMsg = "强制终止进程失败，错误码: " + std::to_string(err);
				coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
				logMessage(logPath, errMsg);
			} else {
				logMessage(logPath, "强制终止进程成功");
			}
			break;
		}
		Sleep(50);
	}
	logMessage(logPath, "ESC监控线程退出");
	return 0;
}

// 启动指定程序
void runProgram(const std::string& programName, bool debugMode, LaunchMode launchMode, const std::string& logPath) {
	logMessage(logPath, "准备启动程序: " + programName + " 模式: " + (launchMode == NEW_WINDOW ? "新窗口" : "同窗口"));
	
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	
	// 根据模式构造命令行
	std::vector<char> cmdLine;
	DWORD creationFlags = 0;
	
	if (launchMode == SINGLE_WINDOW) {
		// 同窗口：直接启动程序
		cmdLine.assign(programName.begin(), programName.end());
		cmdLine.push_back('\0');
		creationFlags = 0; // 继承父进程控制台
		logMessage(logPath, "同窗口模式命令行: " + programName);
	} else {
		// 新窗口：使用 cmd /c "程序名 & pause" 确保结束后暂停
		std::string fullCmd = "cmd /c \"" + programName + " & pause\"";
		cmdLine.assign(fullCmd.begin(), fullCmd.end());
		cmdLine.push_back('\0');
		creationFlags = CREATE_NEW_CONSOLE; // 新建控制台窗口
		logMessage(logPath, "新窗口模式命令行: " + fullCmd);
	}
	
	if (!CreateProcessA(NULL, cmdLine.data(), NULL, NULL, FALSE, creationFlags, NULL, NULL, &si, &pi)) {
		DWORD err = GetLastError();
		std::string errMsg = "启动 " + programName + " 失败";
		if (err == 2) {
			errMsg += "（文件不存在）";
		} else if (err == 5) {
			errMsg += "（拒绝访问）";
		} else {
			errMsg += "，错误代码: " + std::to_string(err);
		}
		coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
		logMessage(logPath, errMsg);
		return;
	}
	
	logMessage(logPath, "进程创建成功，进程ID: " + std::to_string(pi.dwProcessId));
	
	if (launchMode == SINGLE_WINDOW) {
		std::string prompt = "程序 " + programName + " 已启动（同窗口），按 ESC 可强制结束...\n";
		coloredOutput(prompt, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		
		HANDLE hThread = CreateThread(NULL, 0, EscMonitorThread, pi.hProcess, 0, NULL);
		if (hThread == NULL) {
			DWORD err = GetLastError();
			std::string errMsg = "无法创建ESC监控线程，错误码: " + std::to_string(err) + "，将无法使用ESC强制退出";
			coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(logPath, errMsg);
		} else {
			logMessage(logPath, "ESC监控线程创建成功");
		}
		
		logMessage(logPath, "开始等待子进程结束");
		DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);
		if (waitResult == WAIT_FAILED) {
			DWORD err = GetLastError();
			std::string errMsg = "等待进程结束时出错，错误码: " + std::to_string(err);
			coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(logPath, errMsg);
		} else {
			logMessage(logPath, "子进程结束，等待返回");
		}
		
		if (hThread) {
			logMessage(logPath, "等待ESC监控线程结束");
			if (WaitForSingleObject(hThread, 5000) == WAIT_FAILED) {
				DWORD err = GetLastError();
				std::string errMsg = "等待ESC监控线程结束时出错，错误码: " + std::to_string(err);
				coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
				logMessage(logPath, errMsg);
			} else {
				logMessage(logPath, "ESC监控线程已结束");
			}
			CloseHandle(hThread);
		}
		
		DWORD exitCode = 0;
		if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
			DWORD err = GetLastError();
			std::string errMsg = "获取进程退出代码失败，错误码: " + std::to_string(err);
			coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(logPath, errMsg);
			exitCode = err;
		}
		
		std::string exitMsg = "程序 " + programName + " 退出，退出代码: " + std::to_string(exitCode) + "\n";
		coloredOutput(exitMsg, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		logMessage(logPath, "程序 " + programName + " 退出，代码: " + std::to_string(exitCode));
	} else {
		// 新窗口模式：不等待，直接返回
		std::string prompt = "程序 " + programName + " 已在新窗口中启动\n";
		coloredOutput(prompt, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		logMessage(logPath, "新窗口模式启动完成，不等待进程结束");
	}
	
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

// 工具子菜单
void toolSubMenu(bool debugMode, LaunchMode launchMode, const std::string& logPath) {
	int toolChoice;
	while (true) {
		std::cout << "\n===== 工具列表 =====\n";
		std::cout << "1. prime.exe (高精度质数判断与生成器)\n";
		std::cout << "2. High-Precision_Base_Converter.exe (高精度进制转换器)\n";
		std::cout << "3. High_Precision.exe (高精度计算器)\n";
		std::cout << "4. Fraction.exe (高精度分数小数转换器)\n";
		std::cout << "0. 返回主菜单\n";
		std::cout << "请选择工具: ";
		
		std::cin >> toolChoice;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			std::string warnMsg = "输入无效，请重新输入数字";
			std::cout << warnMsg << "\n";
			logMessage(logPath, "用户输入无效: 非数字");
			continue;
		}
		
		logMessage(logPath, "用户选择工具: " + std::to_string(toolChoice));
		
		switch (toolChoice) {
			case 1: runProgram("prime.exe", debugMode, launchMode, logPath); break;
			case 2: runProgram("High-Precision_Base_Converter.exe", debugMode, launchMode, logPath); break;
			case 3: runProgram("High_Precision.exe", debugMode, launchMode, logPath); break;
			case 4: runProgram("Fraction.exe", debugMode, launchMode, logPath); break;
			case 0: 
			logMessage(logPath, "用户选择返回主菜单");
			return;
			default: 
			std::cout << "无效选项，请重新选择\n";
			logMessage(logPath, "用户选择无效工具: " + std::to_string(toolChoice));
			break;
		}
	}
}

// 显示帮助信息
void showHelp() {
	std::cout << "\n========== 帮助信息 ==========\n";
	std::cout << "1 - 运行工具子菜单         : 进入工具列表，选择需要运行的程序\n";
	std::cout << "2 - 切换调试模式           : 开启/关闭子进程退出代码显示（仅同窗口模式有效）\n";
	std::cout << "3 - 切换启动模式           : 在同窗口（等待）与新窗口（不等待）之间切换\n";
	std::cout << "4 - 显示本帮助信息         : 列出所有可用选项\n";
	std::cout << "0 - 退出程序               : 结束启动器\n";
	std::cout << "在子程序运行时（同窗口模式），按下 ESC 键可强制终止该子程序\n";
	std::cout << "配置文件 config.txt 位于程序同目录，支持 # 注释，可设置初始调试模式和启动模式\n";
	std::cout << "日志文件位于 %TEMP%\\launcher_YYYYMMDD.log，每次启动覆盖旧日志\n";
	std::cout << "================================\n";
}

// 打印彩色猫咪图案和介绍（分行cout输出，第一行加空格）
void printBanner() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD originalColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	if (hConsole != INVALID_HANDLE_VALUE) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
			originalColor = csbi.wAttributes;
		}
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	}
	
	// 分行输出猫咪图案，第一行前加一个空格
	std::cout << " /\\_/\\\n";
	std::cout << "( o.o )\n";
	std::cout << " > ^ <\n";
	
	if (hConsole != INVALID_HANDLE_VALUE) {
		SetConsoleTextAttribute(hConsole, originalColor);
	}
	
	std::cout << "\n           高精度工具启动器\n";
	std::cout << "========================================\n";
	std::cout << "本启动器可以运行四个高精度计算工具，支持调试模式、ESC强制退出、\n";
	std::cout << "同窗口/新窗口启动等。详情请查看帮助（选项4）\n";
	std::cout << "========================================\n\n";
}

int main() {
	g_debugMode = false;
	
	std::string logPath;
	try {
		logPath = getLogFilePath();
	} catch (const std::exception& e) {
		std::cerr << "严重错误：无法获取日志文件路径 - " << e.what() << std::endl;
		logPath = "launcher_error.log";
	}
	
	// 清空日志文件（覆盖模式）
	try {
		std::ofstream log(logPath, std::ios::trunc);
		if (!log.is_open()) {
			std::string warnMsg = "无法清空日志文件 " + logPath + "，将尝试追加模式";
			std::cerr << warnMsg << std::endl;
			// 此时日志文件可能不可写，但仍尝试写入
			logMessage(logPath, warnMsg);
		} else {
			log.close();
			logMessage(logPath, "日志文件已清空");
		}
	} catch (const std::exception& e) {
		std::string errMsg = std::string("日志文件操作异常: ") + e.what();
		std::cerr << errMsg << std::endl;
		// 无法写入日志，只能输出到控制台
	}
	
	std::string exeDir = getExecutableDir();
	std::string configPath = exeDir + "config.txt";
	bool debugMode = false;
	LaunchMode launchMode = SINGLE_WINDOW;
	
	try {
		if (!readConfig(configPath, debugMode, launchMode)) {
			createDefaultConfig(configPath);
		}
	} catch (const std::exception& e) {
		std::string errMsg = std::string("配置文件处理异常: ") + e.what();
		coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
		logMessage(logPath, errMsg);
	}
	
	g_debugMode = debugMode;
	
	logMessage(logPath, "========== 启动器启动 ==========");
	logMessage(logPath, "调试模式: " + std::string(debugMode ? "开启" : "关闭"));
	logMessage(logPath, "启动模式: " + std::string(launchMode == NEW_WINDOW ? "新窗口" : "同窗口"));
	
	printBanner();
	
	int choice;
	while (true) {
		std::cout << "请选择操作：\n";
		std::cout << "1. 运行工具\n";
		std::cout << "2. 切换调试模式（当前: " << (debugMode ? "开启" : "关闭") << "）\n";
		std::cout << "3. 切换启动模式（当前: " << (launchMode == NEW_WINDOW ? "新窗口" : "同窗口") << "）\n";
		std::cout << "4. 帮助信息\n";
		std::cout << "0. 退出\n";
		std::cout << "请输入选项: ";
		
		std::cin >> choice;
		
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			std::string warnMsg = "输入无效，请重新输入数字";
			std::cout << warnMsg << "\n";
			logMessage(logPath, "主菜单输入无效: 非数字");
			continue;
		}
		
		logMessage(logPath, "主菜单选择: " + std::to_string(choice));
		g_debugMode = debugMode;
		
		try {
			switch (choice) {
			case 1:
				toolSubMenu(debugMode, launchMode, logPath);
				break;
			case 2:
				debugMode = !debugMode;
				std::cout << "调试模式已切换为: " << (debugMode ? "开启" : "关闭") << "\n";
				logMessage(logPath, "调试模式手动切换为: " + std::string(debugMode ? "开启" : "关闭"));
				saveConfig(configPath, debugMode, launchMode);
				break;
			case 3:
				launchMode = (launchMode == SINGLE_WINDOW) ? NEW_WINDOW : SINGLE_WINDOW;
				std::cout << "启动模式已切换为: " << (launchMode == NEW_WINDOW ? "新窗口" : "同窗口") << "\n";
				logMessage(logPath, "启动模式手动切换为: " + std::string(launchMode == NEW_WINDOW ? "新窗口" : "同窗口"));
				saveConfig(configPath, debugMode, launchMode);
				break;
			case 4:
				showHelp();
				logMessage(logPath, "显示帮助信息");
				break;
			case 0:
				std::cout << "程序退出\n";
				logMessage(logPath, "========== 启动器退出 ==========");
				return 0;
			default:
				std::cout << "无效选项，请重新选择\n";
				logMessage(logPath, "主菜单选择无效选项: " + std::to_string(choice));
				break;
			}
		} catch (const std::exception& e) {
			std::string errMsg = std::string("操作执行异常: ") + e.what();
			coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(logPath, errMsg);
		} catch (...) {
			std::string errMsg = "未知异常发生";
			coloredOutput(errMsg + "\n", FOREGROUND_RED | FOREGROUND_INTENSITY);
			logMessage(logPath, errMsg);
		}
	}
	
	return 0;
}
