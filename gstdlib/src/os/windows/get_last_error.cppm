module;
#include <windows.h>
export module gs:get_last_error;
import std;
import :string;

export string get_last_error() {
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL) != 0) {
		string error_msg((char*)lpMsgBuf);
		LocalFree(lpMsgBuf);
		return error_msg;
	}

	return "Unknown error";
}

export std::string get_last_std_error() {
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

	if (FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &lpMsgBuf,
		0, NULL) != 0) {
		std::string error_msg((char*)lpMsgBuf);
		LocalFree(lpMsgBuf);
		return error_msg;
	}

	return "Unknown error";
}
