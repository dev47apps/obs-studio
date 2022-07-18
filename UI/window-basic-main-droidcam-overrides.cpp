#include "obs-app.hpp"
#include "window-basic-main-droidcam-overrides.hpp"
#include <QMenu>

#ifdef _WIN32
#include <windows.h>

bool OBSBasicDroidCam::nativeEvent(const QByteArray &eventType, void *message, long *result) {
	(void)eventType;
	(void)result;

	MSG* msg = reinterpret_cast<MSG*>(message);
	if (msg->message == WM_SHOWWINDOW) {

		DWORD sw = LOWORD(msg->wParam);
		if (sw == SW_SHOWNORMAL || sw == SW_RESTORE) {
			if (!isVisible()) SetShowing(true);
		}
	}

	return false;
}
#endif

void OBSBasicDroidCam::DroidCam_Connected(OBSSource source) {
	blog(LOG_INFO, "DroidCam_Connected: source=%p / %s", source.Get(),
		obs_source_get_name(source));
}

void OBSBasicDroidCam::DroidCam_Disconnected(OBSSource source) {
	blog(LOG_INFO, "DroidCam_Disconnected: source=%p / %s", source.Get(),
		obs_source_get_name(source));
}


void OBSBasicDroidCam::OBSInit() {
	OBSBasic::OBSInit();

	signalHandlers.emplace_back(obs_get_signal_handler(), "droidcam_connect",
	[](void *data, calldata_t *cd) {
		obs_source_t *source = (obs_source_t *)calldata_ptr(cd, "source");
		if (source && QMetaObject::invokeMethod(static_cast<OBSBasicDroidCam *>(data),
			"DroidCam_Connected", Q_ARG(OBSSource, source))) {
		} else {
			blog(LOG_WARNING, "droidcam_connect event not handled! this=%p source=%p", data, source);
		}
	}, this);

	signalHandlers.emplace_back(obs_get_signal_handler(), "droidcam_disconnect",
	[](void *data, calldata_t *cd) {
		obs_source_t *source = (obs_source_t *)calldata_ptr(cd, "source");
		if (source && QMetaObject::invokeMethod(static_cast<OBSBasicDroidCam *>(data),
			"DroidCam_Disconnected", Q_ARG(OBSSource, source))) {
		} else {
			blog(LOG_WARNING, "droidcam_disconnect event not handled! this=%p source=%p", data, source);
		}
	}, this);
}

void CleanMenuItems(QMenu *menu, bool recursive) {
	QAction *prevSeperator = nullptr;
	foreach(QAction *action, menu->actions()) {
		if (action->isSeparator()) {
			prevSeperator = action;
		}
		else if (recursive && action->menu()) {
			CleanMenuItems(action->menu(), false);
		}

		QString name = action->objectName();

		// File Menu
		if (name == QString::fromLocal8Bit("action_Settings"))
			continue;

		if (name == QString::fromLocal8Bit("actionAlwaysOnTop")) {
			prevSeperator->setVisible(true);
			continue;
		}

		if (name == QString::fromLocal8Bit("actionE_xit")) {
			prevSeperator->setVisible(true);
			continue;
		}

		// View Menu
		if (name == QString::fromLocal8Bit("actionFullscreenInterface"))
			continue;

		if (name == QString::fromLocal8Bit("toggleStatusBar")) {
			prevSeperator->setVisible(true);
			continue;
		}

		// Help Menu
		if (name == QString::fromLocal8Bit("actionHelpPortal"))
			continue;
		if (name == QString::fromLocal8Bit("actionWebsite"))
			continue;
		if (name == QString::fromLocal8Bit("actionShowAbout")) {
			prevSeperator->setVisible(true);
			continue;
		}

		action->setVisible(false);
	}
}
