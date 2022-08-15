#include "obs-app.hpp"
#include "window-basic-settings.hpp"
#include "window-basic-droidcam-overrides.hpp"
#include <QMenu>
#include <QDesktopServices>

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

void OBSBasicDroidCam::on_actionHelpPortal_triggered()
{
	QUrl url = QUrl("https://droidcam.app/help", QUrl::TolerantMode);
	QDesktopServices::openUrl(url);
}

void OBSBasicDroidCam::on_actionWebsite_triggered()
{
	QUrl url = QUrl("https://droidcam.app", QUrl::TolerantMode);
	QDesktopServices::openUrl(url);
}

void CleanMenuItems(QMenu *menu, bool recursive) {
	QAction *prevSeperator = nullptr;
	foreach(QAction *action, menu->actions()) {
		if (action->isSeparator()) {
			prevSeperator = action;
			action->setVisible(false);
			continue;
		}

		if (action->menu()) {
			QString name = action->menu()->objectName();
			if (name == QString::fromLocal8Bit("menuLogFiles"))
				prevSeperator->setVisible(true);

			if (recursive) {
				CleanMenuItems(action->menu(), recursive);
			}
			else {
				action->setVisible(false);
			}
			continue;
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
		if (name == QString::fromLocal8Bit("actionShowLogs"))
			continue;
		if (name == QString::fromLocal8Bit("actionShowCrashLogs"))
			continue;

		if (name == QString::fromLocal8Bit("actionCheckForUpdates"))
			continue;
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

inline bool OBSBasicSettings::IsCustomService() const { return true; }
void OBSBasicSettings::InitStreamPage() {}
void OBSBasicSettings::LoadStream1Settings() {}
void OBSBasicSettings::SaveStream1Settings() {}
void OBSBasicSettings::UpdateMoreInfoLink() {}
void OBSBasicSettings::UpdateKeyLink() {}
void OBSBasicSettings::LoadServices(bool) {}
void OBSBasicSettings::UseStreamKeyAdvClicked() {}
void OBSBasicSettings::on_service_currentIndexChanged(int) {}
void OBSBasicSettings::UpdateServerList() {}
void OBSBasicSettings::on_show_clicked() {}
void OBSBasicSettings::on_authPwShow_clicked() {}
OBSService OBSBasicSettings::SpawnTempService() { return nullptr; }
void OBSBasicSettings::OnOAuthStreamKeyConnected() {}
void OBSBasicSettings::OnAuthConnected() {}
void OBSBasicSettings::on_connectAccount_clicked() {}
void OBSBasicSettings::on_disconnectAccount_clicked() {}
void OBSBasicSettings::on_useStreamKey_clicked() {}
void OBSBasicSettings::on_useAuth_toggled() {}
void OBSBasicSettings::UpdateVodTrackSetting() {}
OBSService OBSBasicSettings::GetStream1Service() { return nullptr; }
void OBSBasicSettings::UpdateServiceRecommendations() {}
void OBSBasicSettings::DisplayEnforceWarning(bool) {}
bool OBSBasicSettings::ResFPSValid(obs_service_resolution*,
	size_t, int) { return true; }
void OBSBasicSettings::UpdateResFPSLimits() {}
