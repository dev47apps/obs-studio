#pragma once
#include <memory>
#include <QDialog>
#include "ui-config.h"

#if DROIDCAM_OVERRIDE
#include "ui_DroidCamAbout.h"
#else
#include "ui_OBSAbout.h"
#endif

class OBSAbout : public QDialog {
	Q_OBJECT

public:
	explicit OBSAbout(QWidget *parent = 0);

	std::unique_ptr<Ui::OBSAbout> ui;

private slots:
	void ShowAbout();
	void ShowAuthors();
	void ShowLicense();
};
