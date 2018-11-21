/*******************************************************************************************************
 DkPatchMatchingPlugin.cpp
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

#include "DkPatchMatchingPlugin.h"

#include <QDebug>
#include <QMouseEvent>
#include <QActionGroup>
#include "DkPolyTimeline.h"
#include <QjsonDocument>
#include <Qjsonarray>
#include <QCombobox>
#include <QMessageBox>
namespace nmp {

	/*-----------------------------------DkPatchMatchingPlugin ---------------------------------------------*/

	/**
	*	Constructor
	**/
	DkPatchMatchingPlugin::DkPatchMatchingPlugin()
		:mViewport(nullptr) {
	}

	/**
	* Returns descriptive image
	**/
	QImage DkPatchMatchingPlugin::image() const {

		return QImage(":/nomacsPluginPaint/img/description.png");
	};

	/**
	* Main function: runs plugin based on its ID
	* @param run ID
	* @param current image in the Nomacs viewport
	**/
	QSharedPointer<nmc::DkImageContainer> DkPatchMatchingPlugin::runPlugin(const QString &runID, QSharedPointer<nmc::DkImageContainer> image) const {

		qDebug() << "Run PatchMatchinPlugin";

		return image;
	};

	/**
	* returns paintViewPort
	**/
	nmc::DkPluginViewPort* DkPatchMatchingPlugin::getViewPort() {

		qDebug() << "Get viewport";


		return mViewport;
	}

	bool DkPatchMatchingPlugin::createViewPort(QWidget * parent)
	{
		if (!mViewport) {
			mViewport = new DkPatchMatchingViewPort;
		}
		return mViewport != 0;
	}

	bool DkPatchMatchingPlugin::closesOnImageChange() const
	{
		return false;
	}

	void DkPatchMatchingPlugin::setVisible(bool visible)
	{
		mViewport->setVisible(visible);
	}

	/*-----------------------------------DkPatchMatchingViewPort ---------------------------------------------*/

	DkPatchMatchingViewPort::DkPatchMatchingViewPort(QWidget* parent, Qt::WindowFlags flags)
		: DkPluginViewPort(parent, flags),
		panning(false),
		mCurrentPolygon(0),
		mPolygonList(QVector < QSharedPointer<DkSyncedPolygon>>{QSharedPointer<DkSyncedPolygon>::create()}),
		mtoolbar(new DkPatchMatchingToolBar(tr("PatchMatching Toolbar)"),this), &QObject::deleteLater),
		mDock(new QDockWidget(this), &QObject::deleteLater),
		mTimeline(new DkPolyTimeline(currentPolygon(), mDock.data()), &QObject::deleteLater),
		defaultCursor(Qt::CrossCursor),
		mColorIndex(0)
	{
		setObjectName("DkPatchMatchingViewPort");
		setMouseTracking(true);
		setAttribute(Qt::WA_MouseTracking);

		setCursor(defaultCursor);

		loadSettings();

		// handler to clone the polygon
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::clonePolyTriggered, this, &DkPatchMatchingViewPort::clonePolygon);
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::saveTriggered, this, &DkPatchMatchingViewPort::saveToFile);
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::addPolyTriggerd, this, &DkPatchMatchingViewPort::addPolygon);
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::closeTriggerd, this, &DkPatchMatchingViewPort::discardChangesAndClose);
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::currentPolyChanged, this, &DkPatchMatchingViewPort::changeCurrentPolygon);
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::removePolyTriggered, this, &DkPatchMatchingViewPort::removePolygon);
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::showTimelineTriggerd, mDock.data(), &QWidget::show);
		
		// timeline stuff
		//mTimeline->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		mTimeline->setStepSize(mtoolbar->getStepSize());
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::stepSizeChanged, mTimeline.data(), &DkPolyTimeline::setStepSize);
		connect(mtoolbar.data(), &DkPatchMatchingToolBar::patchSizeChanged, mTimeline.data(), &DkPolyTimeline::setPatchSize);

		// add the neccesarry dock
		mDock->setStyleSheet("QDockWidget{background-color: #f00;}");
		mDock->setWidget(mTimeline.data());
		dynamic_cast<QMainWindow*>(qApp->activeWindow())->addDockWidget(Qt::BottomDockWidgetArea, mDock.data());
	}

	void DkPatchMatchingViewPort::updateImageContainer(QSharedPointer<nmc::DkImageContainerT> imgC)
	{
		if (!imgC) {
			return;
		}

		//auto empty = mCurrentFile.isEmpty();
		//auto same = createCurrentJson() != mCurrentFile;
		if (createCurrentJson() != mCurrentFile && !mCurrentFile.isEmpty()) {
			auto res = QMessageBox::question(this, tr("Patchmatching Plugin"),
				tr("The workspace has been modified.\n Do you want to save your changes?"),
				QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save);
			if (res == QMessageBox::Save) {
				saveToFile();
			}
		}

		mCurrentFile.clear();

		mImage = imgC;
		mTimeline->setImage(mImage);
		loadFromFile();
	}

	void DkPatchMatchingViewPort::loadFromFile()
	{
		clear();

		QFile file{ getJsonFilePath() };
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "[PatchMatchingPlugin] No json file found for this image: " << getJsonFilePath();
			addPolygon();
			mCurrentFile = createCurrentJson();
		}
		else {
			mCurrentFile = file.readAll();
			auto doc = QJsonDocument::fromJson(mCurrentFile);
			qDebug() << "[PatchMatchingPlugin] Json file found and successfully read.";

			auto list = doc.array();
			auto first = list.at(0).toObject();
			
			for (auto p : list) {
				auto pobj = p.toObject();
				auto poly = QSharedPointer<DkSyncedPolygon>::create();
				// read synced polygon
				poly->read(p.toObject()["polygon"].toObject());

				// read clones
				auto array = pobj["clones"].toArray();
				for (auto obj : array) {
					auto clone = addClone(poly);
					clone->read(obj.toObject());
					//mRenderer.push_back(clone);
				}

				auto size = array.size();
				if (array.isEmpty()) {
					addClone(poly);
					size = 1;
				}
				mPolygonList << poly;
				mtoolbar->addPolygon(mRenderer[mRenderer.size()-size]->getColor(false), first == p.toObject());
			}
		}
	}

	QString DkPatchMatchingViewPort::getJsonFilePath() const
	{
		if (!mImage) {
			return "";
		}

		auto file = QFileInfo{ mImage->filePath() };
		return QDir{ file.path() }.filePath(file.completeBaseName() + ".patches.json");
	}

	DkPatchMatchingViewPort::~DkPatchMatchingViewPort() {
		qDebug() << "[VIEWPORT] deleted...";

		saveSettings();
	}


	void DkPatchMatchingViewPort::saveSettings() const {

		nmc::DefaultSettings settings;

		settings.beginGroup(objectName());
		settings.setValue("StepSize", mtoolbar->getStepSize());
		settings.setValue("PatchSize",mtoolbar->getPatchSize());
		settings.endGroup();
	}

	QColor DkPatchMatchingViewPort::getNextColor()
	{
		const auto div = 60 / 360.;
		return QColor::fromHsvF(fmod(static_cast<double>(mColorIndex++)*div, 1), 1, 1);
	}

	void DkPatchMatchingViewPort::resetColorIndex()
	{
		mColorIndex = 0;
	}

	void DkPatchMatchingViewPort::loadSettings() {

		nmc::DefaultSettings settings;

		settings.beginGroup(objectName());
		mtoolbar->setStepSize(settings.value("StepSize", 50).toInt());
		mtoolbar->setPatchSize(settings.value("PatchSize", 40).toInt());
		settings.endGroup();
	}

	void DkPatchMatchingViewPort::clonePolygon()
	{
		auto poly = addClone(currentPolygon());
		poly->translate(400, 0);

		update();
		updateInactive();

		emit polygonAdded();
	}

	void DkPatchMatchingViewPort::addPolygon()
	{
		auto poly = QSharedPointer<DkSyncedPolygon>::create();
		mPolygonList.push_back(poly);
		addClone(poly);

		// just to make sure check that an polygon is actually selected
		mtoolbar->addPolygon(mRenderer.last()->getColor(false), true);
		qDebug() << "[PatchMatchingPlugin] add polygon triggerd";
	}

	void DkPatchMatchingViewPort::removePolygon()
	{
		auto current = currentPolygon();
		decltype(mRenderer) list;
		
		for (auto r : mRenderer) {
			if (r->getPolygon() == current) {
				list.push_back(r);
			}
		}
	
		for (auto r : list) {
			r->disconnect();
			r->clear();
			mRenderer.removeAll(r);
		}

		mPolygonList.removeAll(current);
		
		if (mPolygonList.empty()) {
			clear();
			addPolygon();
		} else {
			mtoolbar->removePoly(mCurrentPolygon);
		}
	}

	void DkPatchMatchingViewPort::saveToFile()
	{
		// write to file
		QFile saveFile(getJsonFilePath());

		if (!saveFile.open(QIODevice::WriteOnly)) {
			qWarning("Couldn't open savePolygon file.");
			return;
		}

		mCurrentFile = createCurrentJson();
		saveFile.write(mCurrentFile);

		qDebug() << "[PatchMatchingPlugin] Saving file : Success!!!";
	}

	auto unmapToViewPort(QSharedPointer<nmp::DkPolygonRenderer> poly, QPointF point) {
		return (poly->getTransform()*poly->getWorldMatrix()).map(point);
	}

	auto DkPatchMatchingViewPort::getNearestPolygon(QPointF point) {
		// First Point is default polygon
		auto polygon = firstPoly();

		if (!currentPolygon()->points().isEmpty()) {
			auto startDiff = QLineF(currentPolygon()->points().first()->getPos(), point).length();	// Set Start Difference from first to point of the current Polygon
			for (auto r : mRenderer) {
				// Only on active polygon
				if (!r->isInactive()) {
					auto poly_points = r->getPolygon()->points();
					auto first = unmapToViewPort(r, poly_points.first()->getPos());
					auto last = unmapToViewPort(r, poly_points.last()->getPos());

					auto first_diff = QLineF(first, point).length();
					auto last_diff = QLineF(last, point).length();

					if (first_diff < startDiff) {
						polygon = r;
						startDiff = first_diff;
					}
					if (last_diff < startDiff){
						polygon = r;
						startDiff = last_diff;
					}
				}
			}
		}
		return polygon;
	}

	void DkPatchMatchingViewPort::mousePressEvent(QMouseEvent *event) {
		// panning -> redirect to viewport
		if (event->buttons() == Qt::LeftButton &&
			(event->modifiers() == nmc::DkSettingsManager::param().global().altMod || panning)) {
			setCursor(Qt::ClosedHandCursor);
			event->setModifiers(Qt::NoModifier);	// we want a 'normal' action in the viewport
			event->ignore();
			return;
		}

		if (event->buttons() == Qt::LeftButton && parent()) {
			QPointF point = event->pos();

			// Get nearest polygon to point
			auto nearest_poly = getNearestPolygon(point);
			nearest_poly->addPointMouseCoords(point);
		}
	}

	void DkPatchMatchingViewPort::mouseMoveEvent(QMouseEvent *event) {
		// panning -> redirect to viewport
		if (event->modifiers() == nmc::DkSettingsManager::param().global().altMod ||
			panning) {

			event->setModifiers(Qt::NoModifier);
			event->ignore();
			update();
		}
	}

	void DkPatchMatchingViewPort::mouseReleaseEvent(QMouseEvent *event) {
		// panning -> redirect to viewport
		if (event->modifiers() == nmc::DkSettingsManager::param().global().altMod || panning) {
			setCursor(defaultCursor);
			event->setModifiers(Qt::NoModifier);
			event->ignore();
		}
	}

	void DkPatchMatchingViewPort::paintEvent(QPaintEvent *event) {
		checkWorldMatrixChanged();
		DkPluginViewPort::paintEvent(event);
	}

	void DkPatchMatchingViewPort::checkWorldMatrixChanged()
	{
		QTransform mat;
		if (mImgMatrix) {
			mat = *mImgMatrix;
		}
		if (mWorldMatrix) {
			mat = mat*(*mWorldMatrix);
		}
		if (mat != mWorldMatrixCache) {
			mWorldMatrixCache = mat;
			emit worldMatrixChanged(mWorldMatrixCache);
		}
	}

	QByteArray DkPatchMatchingViewPort::createCurrentJson()
	{
		QJsonArray root;
		for (auto poly : mPolygonList) {
			root << createJson(poly);
		}

		QJsonDocument doc{ root };
		return doc.toJson();
	}

	QSharedPointer<DkPolygonRenderer> DkPatchMatchingViewPort::firstPoly()
	{
		// return first active renderer
		for (auto r : mRenderer) {
			if (r->getPolygon() == currentPolygon()) {
				return r;
			}
		}

		// this should not happen
		return addClone(currentPolygon());
	}

	QSharedPointer<DkPolygonRenderer> DkPatchMatchingViewPort::addClone(QSharedPointer<DkSyncedPolygon> poly)
	{
		
		auto render = QSharedPointer<DkPolygonRenderer>::create(this, poly, mWorldMatrixCache);

		render->setColor(getNextColor());
		render->setImageRect(mImage->image().rect());

		connect(this, &DkPatchMatchingViewPort::worldMatrixChanged, render.data(), &DkPolygonRenderer::setWorldMatrix);

		mRenderer.append(render);

		// this is our cleanup slot
		connect(render.data(), &DkPolygonRenderer::removed, this,

			[this, render]() {
			// disconnect the polygon and clear it (delete all QWidgets which have viewport as parent)
			render.data()->disconnect();
			render->clear();

			auto poly = render->getPolygon();

			// remove the polygon from the renderer list
			mRenderer.removeAll(render);
			
			auto cnt = 0;
			for (auto r : mRenderer) {
				if (r->getPolygon() == poly) {
					++cnt;
				}
			}

			if (cnt == 0) {
				removePolygon();
			}
			updateInactive();
		});

		// remove renderer if the whole viewport is reset
		connect(this, &DkPatchMatchingViewPort::reset, render.data(), &DkPolygonRenderer::removed);

		return render;
	}


	void DkPatchMatchingViewPort::setPanning(bool checked) {

		this->panning = checked;
		if (checked) defaultCursor = Qt::OpenHandCursor;
		else defaultCursor = Qt::CrossCursor;
		setCursor(defaultCursor);
	}

	QJsonObject DkPatchMatchingViewPort::createJson(QSharedPointer<DkSyncedPolygon> poly) {

		QJsonObject json;

		// save the synced polygon
		QJsonObject syncedPoly;
		poly->write(syncedPoly);
		json["polygon"] = syncedPoly;

		// save all clones
		QJsonArray array;
		for (auto p : mRenderer) {
			if (p->getPolygon() == poly) {
				QJsonObject obj;
				p->write(obj);
				array.append(obj);
			}
		}
		json["clones"] = array;

		return json;
	}

	void DkPatchMatchingViewPort::changeCurrentPolygon(int idx)
	{
		if (idx == -1) {
			return;
		}
		mCurrentPolygon = idx;
		updateInactive();
	}

	void DkPatchMatchingViewPort::updateInactive()
	{
		for (auto i = 0; i < mPolygonList.size(); ++i) {
			mPolygonList[i]->setInactive(true);
		}
		mPolygonList[mCurrentPolygon]->setInactive(false);

		mTimeline->reset();
		mTimeline->setSyncedPolygon(mPolygonList[mCurrentPolygon]);

		for (auto r : mRenderer) {
			if (!r->isInactive()) {
				mTimeline->addTimeline(r);
			}
			r->update();
		}
	}

	QSharedPointer<DkSyncedPolygon> DkPatchMatchingViewPort::currentPolygon()
	{
		return mPolygonList[mCurrentPolygon];
	}

	void DkPatchMatchingViewPort::clear()
	{
		for (auto r : mRenderer) {
			r->disconnect();
			r->clear();
		}
		mRenderer.clear();
		mPolygonList.clear();
		mtoolbar->clearPolygons();
		resetColorIndex();
	}

	void DkPatchMatchingViewPort::discardChangesAndClose() {

		emit DkPluginViewPort::closePlugin();
	}

	void DkPatchMatchingViewPort::setVisible(bool visible) {

		if (mtoolbar)
			emit DkPluginViewPort::showToolBar(mtoolbar.data(), visible);

		DkPluginViewPort::setVisible(visible);
	}
	/*-----------------------------------DkPatchMatchingToolBar ---------------------------------------------*/
	DkPatchMatchingToolBar::DkPatchMatchingToolBar(const QString & title, QWidget * parent /* = 0 */) : QToolBar(title, parent) {

		createLayout();

		setStyleSheet("QToolBar{spacing: 3px; padding: 3px;}");

		qDebug() << "[PAINT TOOLBAR] created...";
	}

	void DkPatchMatchingToolBar::createLayout() {

		// step size for timeline
		mStepSizeSpinner = new QSpinBox(this);
		mStepSizeSpinner->setObjectName("mStepSizeSpinner");
		mStepSizeSpinner->setSuffix("px");
		mStepSizeSpinner->setMinimum(10);
		mStepSizeSpinner->setMaximum(500);

		// add polygon
		auto timeline = new QAction{ tr("Timeline"), this };
		connect(timeline, &QAction::triggered, this, &DkPatchMatchingToolBar::showTimelineTriggerd);
		addAction(timeline);

		//addWidget(new QLabel{ "Resolution:",this });
		connect(mStepSizeSpinner, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, &DkPatchMatchingToolBar::stepSizeChanged);
		addWidget(new QLabel{ tr("Step"), this });
		addWidget(mStepSizeSpinner);

		mPatchSizeSpinner = new QSpinBox(this);
		mPatchSizeSpinner->setObjectName("mPatchSizeSpinner");
		mPatchSizeSpinner->setSuffix("px");
		mPatchSizeSpinner->setMinimum(10);
		mPatchSizeSpinner->setMaximum(500);
		connect(mPatchSizeSpinner, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, &DkPatchMatchingToolBar::patchSizeChanged);

		addWidget(new QLabel{ tr("Size"), this });
		addWidget(mPatchSizeSpinner);
		addSeparator();
	
		// select polygon
		mPolygonCombobox = new QComboBox(this);
		connect(mPolygonCombobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged)
											, this, &DkPatchMatchingToolBar::changeCurrentPoly);
		connect(mPolygonCombobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::highlighted)
			, this, &DkPatchMatchingToolBar::highlightedPoly);
		addWidget(mPolygonCombobox);

		// add polygon
		auto addPolygonWidget = new QAction{ tr("Add"), this };
		connect(addPolygonWidget, &QAction::triggered, this, &DkPatchMatchingToolBar::addPolyTriggerd);
		addAction(addPolygonWidget);

		// clone
		// setup clone action
		auto clone = new QAction(tr("Clone"), this);
		clone->setShortcuts(QList<QKeySequence>{ QKeySequence(Qt::Key_Enter), QKeySequence(Qt::Key_Return) });
		connect(clone, &QAction::triggered, this, &DkPatchMatchingToolBar::clonePolyTriggered);
		addAction(clone);

		// remove
		auto removePolygonWidget = new QAction{ tr("Remove"), this };
		addAction(removePolygonWidget);
		connect(removePolygonWidget, &QAction::triggered, this, &DkPatchMatchingToolBar::removePolyTriggered);

		addSeparator();

		// save
		QAction* saveAction = new QAction{ tr("Save"), this };
		connect(saveAction, &QAction::triggered, this, &DkPatchMatchingToolBar::saveTriggered);
		addAction(saveAction);

		// close
		QAction* close = new QAction{ tr("Close"), this };
		connect(close, &QAction::triggered, this, &DkPatchMatchingToolBar::closeTriggerd);
		addAction(close);
	}

	void DkPatchMatchingToolBar::setVisible(bool visible) {

		qDebug() << "[PAINT TOOLBAR] set visible: " << visible;

		QToolBar::setVisible(visible);
	}

	int DkPatchMatchingToolBar::getStepSize()
	{
		return mStepSizeSpinner->value();
	}

	void DkPatchMatchingToolBar::setStepSize(int size)
	{
		mStepSizeSpinner->setValue(size);
	}
	int DkPatchMatchingToolBar::getPatchSize()
	{
		return mPatchSizeSpinner->value();
	}
	void DkPatchMatchingToolBar::setPatchSize(int size)
	{
		mPatchSizeSpinner->setValue(size);
	}
	int DkPatchMatchingToolBar::getCurrentPolygon()
	{
		return mPolygonCombobox->currentIndex();
	}
	void DkPatchMatchingToolBar::addPolygon(QColor color, bool select)
	{
		auto text = "Polygon";
		mPolygonCombobox->addItem(text);
		qDebug() << "Count = " << mPolygonCombobox->count();
		mPolygonCombobox->setItemData(mPolygonCombobox->count() - 1, color, Qt::BackgroundRole);
		if (select) {
			mPolygonCombobox->setCurrentIndex(mPolygonCombobox->count() - 1);
			changeCurrentPoly(mPolygonCombobox->count() - 1);
		}
	}
	void DkPatchMatchingToolBar::clearPolygons()
	{
		mPolygonCombobox->clear();
	}
	void DkPatchMatchingToolBar::removePoly(int idx)
	{
		mPolygonCombobox->removeItem(idx);
		changeCurrentPoly(std::max(0,idx-1));
	}
	void DkPatchMatchingToolBar::changeCurrentPoly(int newindex)
	{
		auto color = mPolygonCombobox->itemData(newindex, Qt::BackgroundRole).value<QColor>();
		auto pal = mPolygonCombobox->palette();
		//pal.setColor(QPalette::Active, QPalette::Button, color);
		//pal.setColor(QPalette::Inactive, QPalette::Button, color);
		pal.setColor(QPalette::Highlight, color);
		pal.setColor(QPalette::Text, color);
		//pal.setColor(QPalette::Button, color);
		//pal.setColor(QPalette::Inactive, color);

		//QString style = "selection-background-color: rgb(255,255,255);";
		//style = style.arg(color.red()).arg(color.green()).arg(color.blue());
		//qDebug() << "Style: " << style;
		//mPolygonCombobox->setStyleSheet(style);
		mPolygonCombobox->setPalette(pal);
		emit currentPolyChanged(newindex);
	}
	void DkPatchMatchingToolBar::highlightedPoly(int idx)
	{
		auto color = mPolygonCombobox->itemData(idx, Qt::BackgroundRole).value<QColor>();

		QPalette palette = mPolygonCombobox->view()->palette();
		palette.setColor(QPalette::Highlight, color);
		mPolygonCombobox->view()->setPalette(palette);
	}
};
