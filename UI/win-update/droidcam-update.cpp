#include "win-update-helpers.hpp"
#include "update-window.hpp"
#include "remote-text.hpp"
#include "qt-wrappers.hpp"
#include "win-update.hpp"
#include "obs-app.hpp"

#include <QMessageBox>

#include <string>
# include <sstream>

#include <util/windows/WinHandle.hpp>
#include <util/util.hpp>
#include <strsafe.h>
#include <shellapi.h>

#define xlog(log_level, format, ...) \
        blog(log_level, "[DroidcamUpdate] " format, ##__VA_ARGS__)

#define ilog(format, ...) xlog(LOG_INFO, format, ##__VA_ARGS__)
#define elog(format, ...) xlog(LOG_ERROR, format, ##__VA_ARGS__)

#define UPDATES_URL "https://update.dev47apps.net/"
#define UPDATE_FILE "droidcam-obs-client.1.txt"

const BYTE DEV47APPS_SERIAL[] = {
    0x01, 0x61, 0x6e, 0x5e, 0x84, 0x49, 0x4c, 0x77,
    0x7a, 0x0c, 0xe5, 0xd8, 0xd2, 0xea, 0x30, 0x93,
    0 };

int VerifySignature(const char *filename, const BYTE *serial);

static inline void HexToBytes(const char* hex, BYTE* bytes, size_t size) {
	int c = 0;
	char ptr[3] = { 0 };
	for (size_t i = 0; i < size; i += 2) {
		ptr[0] = hex[i];
		ptr[1] = hex[i + 1];
		bytes[c++] = (BYTE)strtoul(ptr, nullptr, 16);
	}
}

static inline int GetCurrentVersion() {
	int version = 0;
	char *v = OBS_VERSION;
	char *p = v;
	while (*p) {
		char c = *p++;
		if (c < '0' || c > '9')
			continue;

		int num = c - '0';
		version *= 10;
		version += num;
	}
	return version;
}

static bool QuickWriteFile(const char *filename, std::string &data) {
	BPtr<wchar_t> w_file;
	if (os_utf8_to_wcs_ptr(filename, 0, &w_file) == 0)
		return false;

	WinHandle handle = CreateFileW(w_file, GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH,
		nullptr);

	if (handle == INVALID_HANDLE_VALUE) {
		elog("Failed to open file for write '%s': %lu", filename, GetLastError());
		return false;
	}

	DWORD written;
	DWORD size = (DWORD)data.size();
	bool rc = WriteFile(handle, data.data(), size, &written, nullptr);
	if (!rc || written != size) {
		elog("Failed to write file '%s': %lu", filename, GetLastError());
		return false;
	}

	return true;
}

static bool QuickReadFile(const char *filename, std::string &data) {
	BPtr<wchar_t> w_file;
	if (os_utf8_to_wcs_ptr(filename, 0, &w_file) == 0)
		return false;

	WinHandle handle = CreateFileW(w_file, GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, 0, nullptr);

	if (handle == INVALID_HANDLE_VALUE) {
		elog("Failed to open file for read '%s': %lu", filename, GetLastError());
		return false;
	}

	DWORD read;
	DWORD size = GetFileSize(handle, nullptr);
	data.resize(size);

	bool rc = ReadFile(handle, &data[0], size, &read, nullptr);
	if (!rc || read != size) {
		elog("Failed to read file '%s': %lu", filename, GetLastError());
		return false;
	}

	return true;

}

static bool GetLastWriteTime(HANDLE handle, char* buffer, int size) {
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC;
	const char* dow[] = {
	    "Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat",
	};

	const char* month[] = {
	    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	};

	if (!GetFileTime(handle, &ftCreate, &ftAccess, &ftWrite))
		return false;

	FileTimeToSystemTime(&ftWrite, &stUTC);

	// Thu, 21 Apr 2022 20:36:02 GMT
	return size > snprintf(buffer, size,
		"%s, %02d %s %d %02d:%02d:%02d GMT",
		dow[stUTC.wDayOfWeek], stUTC.wDay, month[stUTC.wMonth - 1],
		stUTC.wYear, stUTC.wHour, stUTC.wMinute, stUTC.wSecond);

}

static bool Download(const char *filename, const char* updatesDir,
	std::string &data, BPtr<wchar_t> &w_file_path,
	bool verify = true, bool wantData = false)
{
	std::vector<std::string> headers;

	std::string file_path = updatesDir;
	file_path += "/";
	file_path += filename;
	if (os_utf8_to_wcs_ptr(file_path.data(), 0, &w_file_path) == 0)
		return false;

	HANDLE handle = CreateFileW(w_file_path, GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, 0, nullptr);

	if (handle != INVALID_HANDLE_VALUE) {
		char buffer[256];
		if (GetLastWriteTime(handle, buffer, 256)) {
			std::string header = "If-Modified-Since: ";
			header += buffer;
			headers.push_back(std::move(header));
		}

		CloseHandle(handle);
	}

	long responseCode;
	std::string error;
	std::string signature;
	std::string file_url(UPDATES_URL);
	file_url += filename;
	// ilog("GetRemoteFile: %s", file_url.data());
	data.clear();

	if (GetRemoteFile(file_url.data(), data, error, &responseCode,
		nullptr, "", nullptr, headers, &signature, 30))
	{
		BYTE serial[256];
		ilog("GetRemoteFile: signature=%s", signature.c_str());
		ilog("GetRemoteFile: responseCode=%ld", responseCode);

		if (responseCode == 304 && signature.size()) {
			HexToBytes(signature.data(), serial, signature.size());
			serial[sizeof(DEV47APPS_SERIAL) - 1] = 0;

			if (verify && !VerifySignature(file_path.data(), serial)) {
				elog("Invalid signature: %s", file_path.data());
				DeleteFileW(w_file_path);
				return false;
			}

			goto EARLY_OUT;
		}

		if (!(responseCode == 200 && signature.size() && data.size()))
			return false;

		std::string file_path_tmp(file_path + ".tmp");
		if (!QuickWriteFile(file_path_tmp.data(), data))
			return false;

		HexToBytes(signature.data(), serial, signature.size());
		serial[sizeof(DEV47APPS_SERIAL) - 1] = 0;

		if (verify && !VerifySignature(file_path_tmp.data(), serial)) {
			elog("Invalid signature: %s", file_url.data());
			DeleteFileA(file_path_tmp.data());
			return false;
		}

		BPtr<wchar_t> w_file_path_tmp;
		if (os_utf8_to_wcs_ptr(file_path_tmp.data(), 0, &w_file_path_tmp) == 0)
			return false;

		if (!MoveFileW(w_file_path_tmp, w_file_path)) {
			elog("error renaming %s -> %s", file_path_tmp.data(), file_path.data());
				return false;
		}
	}
	else {
		elog("Update check failed: %s", error.c_str());
		return false;
	}

EARLY_OUT:
	return wantData ? QuickReadFile(file_path.data(), data) : true;
}

void AutoUpdateThread::infoMsg(const QString &title, const QString &text)
{
	OBSMessageBox::information(App()->GetMainWindow(), title, text);
}

void AutoUpdateThread::info(const QString &title, const QString &text)
{
	QMetaObject::invokeMethod(this, "infoMsg", Qt::BlockingQueuedConnection,
				  Q_ARG(QString, title), Q_ARG(QString, text));
}

int AutoUpdateThread::queryUpdateSlot(bool localManualUpdate,
				      const QString &text)
{
	OBSUpdate updateDlg(App()->GetMainWindow(), localManualUpdate, text);
	return updateDlg.exec();
}

int AutoUpdateThread::queryUpdate(bool localManualUpdate, const char *text_utf8)
{
	int ret = OBSUpdate::No;
	QString text = text_utf8;
	QMetaObject::invokeMethod(this, "queryUpdateSlot",
				  Qt::BlockingQueuedConnection,
				  Q_RETURN_ARG(int, ret),
				  Q_ARG(bool, localManualUpdate),
				  Q_ARG(QString, text));
	return ret;
}

static int ParseUpdateFile(std::string &data, std::string &notes,
	std::string &updater, int &version)
{
	std::string line;
	std::istringstream iss(data);

	#define GET_LINE \
	if (!std::getline(iss, line)) \
		return OBSUpdate::No

	GET_LINE;

	if (line == "droidcam-obs-client-update-v1") {
		GET_LINE;
		int new_version = 0;
		try { new_version = std::stoi(line); }
		catch (...) {}

		if (new_version > version) {
			version = new_version;
		}
		else {
			return OBSUpdate::Skip;
		}

		GET_LINE;
		updater = line;

		while (std::getline(iss, line)) {
			notes += line;
			notes += "<br/>";
		}

		return OBSUpdate::Yes;
	}

	return OBSUpdate::No;
}

void AutoUpdateThread::run()
{
	struct FinishedTrigger {
		inline ~FinishedTrigger()
		{
			QMetaObject::invokeMethod(App()->GetMainWindow(),
				"updateCheckFinished");
		}
	} finishedTrigger;

	if (!VerifySignature(nullptr, DEV47APPS_SERIAL)) {
		elog("executable signature invalid, skipping update check");
		return;
	}

	BPtr<wchar_t> wUpdaterPath;
	BPtr<wchar_t> wManifestPath;
	char updatesDir[512];
	if (GetConfigPath(updatesDir, sizeof(updatesDir), "obs-studio/updates") <= 0)
		return;

	int version = GetCurrentVersion();
	int result = OBSUpdate::No;
	std::string data;
	std::string notes;
	std::string updater;

	// TODO use a cab file and require + verify signature
	if (Download(UPDATE_FILE, updatesDir, data, wManifestPath, false, true)) {
		result = ParseUpdateFile(data, notes, updater, version);
	}

	if (result == OBSUpdate::Skip) {
		if (manualUpdate)
			info("", QTStr("Updater.NoUpdatesAvailable.Text"));

		return;
	}

	if (result == OBSUpdate::No || updater.size() == 0)
		return;

	// result == OBSUpdate::Yes
	// new update avialble

	int skip_version = config_get_int(GetGlobalConfig(), "General",
		"SkipUpdateVersion");

	if (!manualUpdate && version == skip_version) {
		ilog("Skip update version %d", version);
		return;
	}

	result = queryUpdate(manualUpdate, notes.c_str());
	if (result == OBSUpdate::No) {
		if (!manualUpdate) {
			long long t = (long long)time(nullptr);
			config_set_int(GetGlobalConfig(), "General",
				"LastUpdateCheck", t);
		}
		return;

	}

	if (result == OBSUpdate::Skip) {
		config_set_int(GetGlobalConfig(), "General",
			"SkipUpdateVersion", version);
		return;
	}

	// result == OBSUpdate::Yes
	// get the update

	if (!Download(updater.c_str(), updatesDir, data, wUpdaterPath)) {
		elog("error fetching %s", updater.c_str());
		QString msg = QTStr("Updater.FailedToLaunch");
		info("", msg);
		return;
	}

	SHELLEXECUTEINFO execInfo = {};
	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = wUpdaterPath;
	execInfo.lpDirectory = nullptr;
	execInfo.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteEx(&execInfo)) {
		elog("Can't launch updater '%s': %d", updater.c_str(), GetLastError());
		QString msg = QTStr("Updater.FailedToLaunch");
		info("", msg);
		return;
	}

	config_set_int(GetGlobalConfig(), "General", "LastUpdateCheck", 0);
	config_set_int(GetGlobalConfig(), "General", "SkipUpdateVersion", 0);
	QMetaObject::invokeMethod(App()->GetMainWindow(), "close");
}

void WhatsNewInfoThread::run() {}

void WhatsNewBrowserInitThread::run() {}
