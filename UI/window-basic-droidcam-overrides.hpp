#pragma once
#include "window-basic-main.hpp"
#include <QWidget>

class BrowserDock;

struct OBSBasicDroidCam : public OBSBasic {
	QSharedPointer<BrowserDock> remoteDock;
	QSharedPointer<QAction> remoteMenuEntry;
	std::string last_remote_url;
	std::map<void*, uint64_t> event_log;

	void OBSInit() override;
	~OBSBasicDroidCam();

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
