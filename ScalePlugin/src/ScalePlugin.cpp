/*******************************************************************************************************
ReadModules are plugins for nomacs developed at CVL/TU Wien for the EU project READ. 

Copyright (C) 2016 Markus Diem <diem@caa.tuwien.ac.at>
Copyright (C) 2016 Stefan Fiel <fiel@caa.tuwien.ac.at>
Copyright (C) 2016 Florian Kleber <kleber@caa.tuwien.ac.at>

This file is part of ReadModules.

ReadFramework is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ReadFramework is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

The READ project  has  received  funding  from  the European  Union’s  Horizon  2020  
research  and innovation programme under grant agreement No 674943

related links:
[1] http://www.caa.tuwien.ac.at/cvl/
[2] https://transkribus.eu/Transkribus/
[3] https://github.com/TUWien/
[4] http://nomacs.org
*******************************************************************************************************/

#include "ScalePlugin.h"

// nomacs
#include "DkImageStorage.h"
#include "DkSettings.h"
#include "DkTimer.h"

// w2x dependencies
#include "picojson.h"
#include "modelHandler.hpp"
#include "convertRoutine.hpp"


#pragma warning(push, 0)	// no warnings from includes - begin
#include <QAction>
#include <QSettings>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#pragma warning(pop)		// no warnings from includes - end

namespace rdm {


/**
*	Constructor
**/
ScalePlugin::ScalePlugin(QObject* parent) : QObject(parent) {

	// create run IDs
	QVector<QString> runIds;
	runIds.resize(id_end);

	runIds[id_scale] = "af450a0a05f14184aef91fe26ab0061f";
	mRunIDs = runIds.toList();

	// create menu actions
	QVector<QString> menuNames;
	menuNames.resize(id_end);

	menuNames[id_scale] = tr("Scale Image");
	mMenuNames = menuNames.toList();

	// create menu status tips
	QVector<QString> statusTips;
	statusTips.resize(id_end);

	statusTips[id_scale] = tr("Doubles the image size");
	mMenuStatusTips = statusTips.toList();

	init();
}
/**
*	Destructor
**/
ScalePlugin::~ScalePlugin() {

	qDebug() << "destroying scale plugin...";
}


/**
* Returns unique ID for the generated dll
**/
QString ScalePlugin::id() const {

	return PLUGIN_ID;
};

/**
* Returns descriptive iamge for every ID
* @param plugin ID
**/
QImage ScalePlugin::image() const {

	return QImage(":/ScalePlugin/img/read.png");
};

QList<QAction*> ScalePlugin::createActions(QWidget* parent) {

	if (mActions.empty()) {

		for (int idx = 0; idx < id_end; idx++) {
			QAction* ca = new QAction(mMenuNames[idx], parent);
			ca->setObjectName(mMenuNames[idx]);
			ca->setStatusTip(mMenuStatusTips[idx]);
			ca->setData(mRunIDs[idx]);	// runID needed for calling function runPlugin()
			mActions.append(ca);
		}
	}

	return mActions;
}



QList<QAction*> ScalePlugin::pluginActions() const {
	return mActions;
}

/**
* Main function: runs plugin based on its ID
* @param plugin ID
* @param image to be processed
**/
QSharedPointer<nmc::DkImageContainer> ScalePlugin::runPlugin(
	const QString &runID, 
	QSharedPointer<nmc::DkImageContainer> imgC, 
	const nmc::DkSaveInfo& saveInfo,
	QSharedPointer<nmc::DkBatchInfo>& info) const {

	qDebug() << "running scale plugin...";

	if (!imgC)
		return imgC;

	QString modelName;

	if(runID == mRunIDs[id_scale]) {

		modelName = "C:/VSProjects/nomacs/build2015-x64/RelWithDebInfo/plugins";
		qDebug() << "scaling image...";
	}

	nmc::DkTimer dt;
	//std::vector<std::unique_ptr<w2xc::Model> > models;
	//w2xc::modelUtility::generateModelFromJSON(modelName.toStdString(), models);
	//qDebug() << "model loaded in" << dt;

	cv::Mat image = nmc::DkImage::qImage2Mat(imgC->image());
	cv::cvtColor(image, image, CV_RGB2GRAY);

	W2XConv* converter = w2xconv_init(W2XCONV_GPU_DISABLE, 1, true);
	w2xconv_load_models(converter, modelName.toStdString().c_str());
	
	cv::Mat dst;
	w2xconv_convert(converter, image, dst, 0, 2.0, 0);

	cv::cvtColor(dst, dst, CV_GRAY2RGB);

	imgC->setImage(nmc::DkImage::mat2QImage(dst), tr("Rescaled"));

	// wrong runID? - do nothing
	return imgC;
}
	
void ScalePlugin::preLoadPlugin() const {

	qDebug() << "[PRE LOADING] Batch Test";
}

void ScalePlugin::postLoadPlugin(const QVector<QSharedPointer<nmc::DkBatchInfo>>& batchInfo) const {
	
}


void ScalePlugin::init() {
	loadSettings(nmc::Settings::instance().getSettings());
}

void ScalePlugin::loadSettings(QSettings & settings) {
	//settings.beginGroup("SkewEstimation");
	//mFilePath = settings.value("skewEvalPath", mFilePath).toString();
	//settings.endGroup();
}

void ScalePlugin::saveSettings(QSettings & settings) const {
	//settings.beginGroup("SkewEstimation");
	//settings.setValue("skewEvalPath", mFilePath);
	//settings.endGroup();
}

};