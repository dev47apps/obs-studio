#include "obs-app.hpp"

#include <QMenu>

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
