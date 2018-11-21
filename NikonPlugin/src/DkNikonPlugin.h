/*******************************************************************************************************
 DkPaintPlugin.h
 Created on:	14.07.2013

 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances

 Copyright (C) 2011-2013 Markus Diem <markus@nomacs.org>
 Copyright (C) 2011-2013 Stefan Fiel <stefan@nomacs.org>
 Copyright (C) 2011-2013 Florian Kleber <florian@nomacs.org>

 This file is part of nomacs.

 nomacs is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 nomacs is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *******************************************************************************************************/

#pragma once

#include <QObject>
#include <QtPlugin>
#include <QImage>
#include <QStringList>
#include <QString>
#include <QMessageBox>
#include <QAction>
#include <QGraphicsPathItem>
#include <QGraphicsSceneMouseEvent>
#include <QToolbar>
#include <QMainWindow>
#include <QColorDialog>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QListWidgetItem>
#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QApplication>
#include <QInputDialog>

#include <QDockWidget>

#include <functional>
#include <memory>
#include <set>

#include "DkPluginInterface.h"
#include "DkSettings.h"
#include "DkUtils.h"
#include "DkBaseViewport.h"
#include "MaidFacade.h"
#include "DkCamControls.h"

namespace nmp {

class DkNikonViewPort;

class DkNikonPlugin : public QObject, nmc::DkViewPortInterface {
    Q_OBJECT
    Q_INTERFACES(nmc::DkViewPortInterface)
	Q_PLUGIN_METADATA(IID "com.nomacs.ImageLounge.DkNikonPlugin/3.3" FILE "DkNikonPlugin.json")


public:
    
	DkNikonPlugin();
	~DkNikonPlugin();

    QImage image() const override;

	QSharedPointer<nmc::DkImageContainer> runPlugin(const QString &runID = QString(), QSharedPointer<nmc::DkImageContainer> image = QSharedPointer<nmc::DkImageContainer>()) const override;
	nmc::DkPluginViewPort* getViewPort() override;
	void deleteViewPort();
	virtual bool closesOnImageChange() {return false;};

	void setVisible(bool visible) override;

	bool createViewPort(QWidget* parent) override;

protected:
	nmc::DkPluginViewPort* viewport;
	DkCamControls* camControls;
	MaidFacade* maidFacade;
};

class DkNikonViewPort : public nmc::DkPluginViewPort {
	Q_OBJECT

public:
	DkNikonViewPort(QWidget* parent = 0, Qt::WindowFlags flags = 0);
	~DkNikonViewPort();

protected:

};

};