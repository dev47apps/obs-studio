#include "obs-app.hpp"
#ifdef BROWSER_AVAILABLE
#include "window-dock-browser.hpp"
#endif
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

const char *DROIDCAM_OBS_ID = "droidcam_obs";

// TODO reduce logging post-beta
// TODO add "Toggle Controls" hotkey
void OBSBasicDroidCam::on_urlChanged(const QString &url) {
	blog(LOG_INFO, "Remote URL = %s", url.toUtf8().constData());
	last_remote_url = url.toStdString();
}

void OBSBasicDroidCam::DroidCam_Connect(OBSSource source) {
	blog(LOG_INFO, "DroidCam_Connect: %s", obs_source_get_name(source));
	if (allowEvent()) {
		SysTrayNotify(QTStr("Connected"), obs_source_get_name(source),
			QSystemTrayIcon::Information);
	}

	if (last_remote_url.rfind("http", 0) != 0) {
		DroidCam_Update_Remote(source);
		return;
	}

	obs_sceneitem_t *item = obs_scene_sceneitem_from_source(GetCurrentScene(), source);
	if (item) {
		if (obs_sceneitem_selected(item)) {
			blog(LOG_INFO, "DroidCam_Connect: source SELECTED => Update Remote");
			DroidCam_Update_Remote(source);
		}
		obs_sceneitem_release(item);
		return;
	}
}

void OBSBasicDroidCam::DroidCam_Disconnect(OBSSource source) {
	blog(LOG_INFO, "DroidCam_Disconnect: %p", source.Get());
	if (allowEvent()) {
		SysTrayNotify(QTStr("Disconnected"), obs_source_get_name(source),
			QSystemTrayIcon::Information);
	}

	if (!DroidCam_Cycle_Remote(source) && !obs_source_showing(source)) {
		blog(LOG_INFO, "DroidCam_Disconnect: CLEAR URL");
		QCefWidget *browser = (QCefWidget *)remoteDock->widget();
		if (browser) browser->setURL("about:blank");
		last_remote_url = "";
	}
}

bool OBSBasicDroidCam::DroidCam_Cycle_Remote(OBSSource source) {
	struct io {
		obs_source_t *source;
		OBSBasicDroidCam *thiz;
		bool changed;
	} io { source.Get(), this, false };

	obs_scene_enum_items(GetCurrentScene(),
		[](obs_scene_t*, obs_sceneitem_t *item, void *data) {
		auto io = (struct io*) data;
		obs_source_t* source = obs_sceneitem_get_source(item);
		if (!source || source == io->source)
			return true;

		const char *id = obs_source_get_id(source);
		if (strcmp(id, DROIDCAM_OBS_ID) == 0 && io->thiz->DroidCam_Update_Remote(source)) {
			io->changed = true;
			return false; // ok, stop enumeration
		}

		return true;
	}, &io);

	return io.changed;
}

bool OBSBasicDroidCam::DroidCam_Update_Remote(OBSSource source) {
	std::string remote_url = "";
	obs_data_t *settings = obs_source_get_settings(source);
	if (settings) {
		const char *url = obs_data_get_string(settings, "remote_url");
		if (url && strlen(url) > 4) remote_url = url;
		obs_data_release(settings);
	}

	blog(LOG_INFO, "DroidCam_Update_Remote: %s url=%s // last_remote_url=%s",
		obs_source_get_name(source),
		remote_url.c_str(), last_remote_url.c_str());

	if (remote_url.empty())
		return false;

	#ifdef BROWSER_AVAILABLE
	QCefWidget *browser = (QCefWidget *)remoteDock->widget();
	if (browser && obs_source_showing(source))
	{
		if (last_remote_url.rfind(remote_url, 0) == 0) {
			blog(LOG_INFO, "DroidCam_Update_Remote: REFRESH %s", remote_url.c_str());
			browser->reloadPage();
		}
		else {
			blog(LOG_INFO, "DroidCam_Update_Remote: SET => %s", remote_url.c_str());
			browser->setURL(remote_url);
			last_remote_url = remote_url;
		}
		return true;
	}
	#endif
	return false;
}

OBSBasicDroidCam::~OBSBasicDroidCam() {
}

void OBSBasicDroidCam::OBSInit() {
	OBSBasic::OBSInit();

	signalHandlers.emplace_back(obs_get_signal_handler(), "droidcam_connect",
		[](void *data, calldata_t *cd) {
			obs_source_t *source = (obs_source_t *)calldata_ptr(cd, "source");
			QMetaObject::invokeMethod(static_cast<OBSBasicDroidCam *>(data),
				"DroidCam_Connect", Q_ARG(OBSSource, source));
		}, this);

	signalHandlers.emplace_back(obs_get_signal_handler(), "droidcam_disconnect",
		[](void *data, calldata_t *cd) {
			obs_source_t *source = (obs_source_t *)calldata_ptr(cd, "source");
			QMetaObject::invokeMethod(static_cast<OBSBasicDroidCam *>(data),
				"DroidCam_Disconnect", Q_ARG(OBSSource, source));
		}, this);

	OBSScene curScene = GetCurrentScene();
	if (curScene) {
		obs_source_t *sceneSource = obs_scene_get_source(curScene);
		signal_handler_t *signal = obs_source_get_signal_handler(sceneSource);

		signalHandlers.emplace_back(signal, "item_select",
		[](void *data, calldata_t *cd) {
			obs_source_t* source = obs_sceneitem_get_source(
				(obs_sceneitem_t *)calldata_ptr(cd, "item"));

			blog(LOG_INFO, "Source Selected: %s", obs_source_get_name(source));
			const char *id = obs_source_get_id(source);
			if (strcmp(id, DROIDCAM_OBS_ID) == 0)
				QMetaObject::invokeMethod(static_cast<OBSBasicDroidCam *>(data),
					"DroidCam_Update_Remote", Q_ARG(OBSSource, source));
		}, this);

		signalHandlers.emplace_back(signal, "item_remove",
		[](void *data, calldata_t *cd) {
			obs_source_t* source = obs_sceneitem_get_source(
				(obs_sceneitem_t *)calldata_ptr(cd, "item"));

			if (source) {
				blog(LOG_INFO, "Source Removed: %s", obs_source_get_name(source));
				const char *id = obs_source_get_id(source);
				if (strcmp(id, DROIDCAM_OBS_ID) == 0)
					QMetaObject::invokeMethod(static_cast<OBSBasicDroidCam *>(data),
						"DroidCam_Cycle_Remote", Q_ARG(OBSSource, source));
			}
		}, this);
	}

	#ifdef BROWSER_AVAILABLE
	if (cef) {
		OBSBasic::InitBrowserPanelSafeBlock();
		std::string script = "document.body.style.backgroundColor='#000';";
		last_remote_url = "about:blank";

		remoteDock.reset(new BrowserDock());
		remoteDock->setObjectName("droidcamRemote");
		remoteDock->setFloating(true);
		remoteDock->setVisible(false);
		remoteDock->setMinimumSize(200, 200);
		remoteDock->setWindowTitle(QTStr("Remote.Title"));
		remoteDock->setAllowedAreas(Qt::AllDockWidgetAreas);

		QCefWidget *browser = cef->create_widget(nullptr, last_remote_url);
		browser->setStartupScript(script);
		remoteDock->SetWidget(browser);

		// note addDockWidget != AddDockWidget
		addDockWidget(Qt::RightDockWidgetArea, remoteDock.data());

		QDockWidget *dock = remoteDock.data();
		dock->setVisible(false);
		remoteMenuEntry.reset(AddDockWidget(dock));

		dock->connect(dock, &QDockWidget::visibilityChanged,
			[=](bool visible) {
				blog(LOG_INFO, "Remote Visibility = %d", visible);
				if (visible) {
					QCefWidget *browser = (QCefWidget *)remoteDock->widget();
					if (browser) browser->reloadPage();
				}
			});

		connect(browser, SIGNAL(urlChanged(const QString &)), this,
			SLOT(on_urlChanged(const QString &)));

		// re-run restoreState() from  OBSBasic::OBSInit()
		const char *dockStateStr = config_get_string(
			App()->GlobalConfig(), "BasicWindow", "DockState");

		if (dockStateStr) {
			restoreState(QByteArray::fromBase64(QByteArray(dockStateStr)));
		}
	}
	#endif // BROWSER_AVAILABLE
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
bool OBSBasicSettings::ResFPSValid(obs_service_resolution*, size_t, int) { return true; }
void OBSBasicSettings::UpdateResFPSLimits() {}
