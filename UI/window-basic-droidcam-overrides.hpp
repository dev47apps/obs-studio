#pragma once
#include "window-basic-main.hpp"
#include <QWidget>

struct OBSBasicDroidCam : public OBSBasic {
	void OBSInit() override;

#ifdef _WIN32
	bool nativeEvent(const QByteArray&, void*, long*) override;
#endif

private slots:
	void on_actionHelpPortal_triggered() override;
	void on_actionWebsite_triggered() override;
	void on_actionDiscord_triggered() override { };
	void on_customContextMenuRequested(const QPoint &) override { };
	void on_scenes_customContextMenuRequested(const QPoint &) override { };
	void ProgramViewContextMenuRequested(const QPoint &) override { };
};
