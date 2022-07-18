#pragma once
#include "window-basic-main.hpp"
#include <QWidget>

struct OBSBasicDroidCam : public OBSBasic {
	void OBSInit() override;

#ifdef _WIN32
	bool nativeEvent(const QByteArray&, void*, long*) override;
#endif

private slots:
};
