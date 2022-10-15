#pragma once
#include "window-basic-main.hpp"
#include <QWidget>

class BrowserDock;

struct OBSBasicDroidCam : public OBSBasic {
	QSharedPointer<BrowserDock> remoteDock;
	QSharedPointer<QAction> remoteMenuEntry;
	std::string last_remote_url;
	uint64_t last_alert = 0;

	void OBSInit() override;
	~OBSBasicDroidCam();

	inline bool allowEvent(void) {
		auto now = os_gettime_ns();
		if ((now - last_alert) < 10000000000) {
			blog(LOG_INFO, "Rate Limiting Notifications");
			return false;
		}
		last_alert = now;
		return true;
	}

#ifdef _WIN32
	bool nativeEvent(const QByteArray&, void*, long*) override;
#endif

	Q_OBJECT

private slots:
	void DroidCam_Connect(OBSSource source);
	void DroidCam_Disconnect(OBSSource source);
	bool DroidCam_Update_Remote(OBSSource source);
	void on_actionHelpPortal_triggered() override;
	void on_actionWebsite_triggered() override;
	void on_actionDiscord_triggered() override { };
	void on_customContextMenuRequested(const QPoint &) override { };
	void on_scenes_customContextMenuRequested(const QPoint &) override { };
	void ProgramViewContextMenuRequested(const QPoint &) override { };
};
