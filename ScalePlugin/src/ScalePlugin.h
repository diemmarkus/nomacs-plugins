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

The READ project  has  received  funding  from  the European  Union�s  Horizon  2020  
research  and innovation programme under grant agreement No 674943

related links:
[1] http://www.caa.tuwien.ac.at/cvl/
[2] https://transkribus.eu/Transkribus/
[3] https://github.com/TUWien/
[4] http://nomacs.org
*******************************************************************************************************/

#pragma once

#include "DkPluginInterface.h"
#include "DkBatchInfo.h"

class QSettings;


// opencv defines
namespace cv {
	class Mat;
}

namespace rdm {


class SkewInfo : public nmc::DkBatchInfo {

public:
	SkewInfo(const QString& id = QString(), const QString& filePath = QString());

	void setProperty(const QString& p);
	QString property() const;

	void setSkew(const double skew);
	double skew() const;

	void setSkewGt(const double skew);
	double skewGt() const;

private:
	QString mProp;
	double mSkew;
	double mSkewGt;

};

class ScalePlugin : public QObject, nmc::DkBatchPluginInterface {
	Q_OBJECT
		Q_INTERFACES(nmc::DkBatchPluginInterface)
		Q_PLUGIN_METADATA(IID "com.nomacs.ImageLounge.ScalePlugin/5.1" FILE "ScalePlugin.json")

public:
	ScalePlugin(QObject* parent = 0);
	~ScalePlugin();

	QString id() const override;
	QImage image() const override;

	QList<QAction*> createActions(QWidget* parent) override;
	QList<QAction*> pluginActions() const override;
	QSharedPointer<nmc::DkImageContainer> runPlugin(
		const QString &runID, 
		QSharedPointer<nmc::DkImageContainer> imgC,
		const nmc::DkSaveInfo& saveInfo,
		QSharedPointer<nmc::DkBatchInfo>& info) const override;

	void preLoadPlugin() const override;
	void postLoadPlugin(const QVector<QSharedPointer<nmc::DkBatchInfo> >& batchInfo) const override;

	enum {
		// add actions here
		id_scale,

		id_end
	};

protected:
	QList<QAction*> mActions;
	QStringList mRunIDs;
	QStringList mMenuNames;
	QStringList mMenuStatusTips;

	QString mFilePath;

private:
	void init();
	void loadSettings(QSettings& settings);
	void saveSettings(QSettings& settings) const;

};
};
