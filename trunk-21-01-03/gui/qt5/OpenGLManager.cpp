#include "OpenGLManager.hpp"

namespace yade { // Cannot have #include directive inside.

CREATE_LOGGER(OpenGLManager);

OpenGLManager* OpenGLManager::self = NULL;

OpenGLManager::OpenGLManager(QObject* parent)
        : QObject(parent)
{
	if (self)
		throw runtime_error("OpenGLManager instance already exists, uses OpenGLManager::self to retrieve it.");
	self     = this;
	renderer = shared_ptr<OpenGLRenderer>(new OpenGLRenderer);
	renderer->init();
	connect(this, SIGNAL(createView()), this, SLOT(createViewSlot()));
	connect(this, SIGNAL(resizeView(int, int, int)), this, SLOT(resizeViewSlot(int, int, int)));
	connect(this, SIGNAL(closeView(int)), this, SLOT(closeViewSlot(int)));
	connect(this, SIGNAL(startTimerSignal()), this, SLOT(startTimerSlot()), Qt::QueuedConnection);
}

void OpenGLManager::timerEvent(QTimerEvent* /*event*/)
{
	//cerr<<".";
	const std::lock_guard<std::mutex> lock(viewsMutex);
// when sharing the 0th view widget, it should be enough to update the primary view only
//if(views.size()>0) views[0]->updateGLViewer();
#if 1
	FOREACH(const shared_ptr<GLViewer>& view, views)
	{
		if (view)
			view->updateGLViewer();
	}
#endif
}

void OpenGLManager::cleanupViewsSlot()
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	LOG_DEBUG("mutex locked, now calling: if(not viewsLater.empty()) { ... }");
	if (not viewsLater.empty()) {
		// cannot use viewsLater.clear(), because after removing each one of the views the QT event loop needs to call deleteLater(), see https://wiki.qt.io/Threads_Events_QObjects
		viewsLater.resize(viewsLater.size() - 1);
		if (not viewsLater.empty()) {
			// whatever amount of time (e.g. 250ms) is OK, because this event is added at the end of the event queue.
			QTimer::singleShot(250, this, SLOT(cleanupViewsSlot()));
		}
	}
}

void OpenGLManager::createViewSlot()
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	if (views.size() == 0) {
		views.push_back(shared_ptr<GLViewer>(boost::make_shared<GLViewer>(0, renderer, /*shareWidget*/ (QGLWidget*)0)));
	} else {
		throw runtime_error("Secondary views not supported");
		//views.push_back(shared_ptr<GLViewer>(new GLViewer(views.size(),renderer,views[0].get())));
	}
}

void OpenGLManager::resizeViewSlot(int id, int wd, int ht) { views[id]->resize(wd, ht); }

void OpenGLManager::closeViewSlot(int id)
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	for (size_t i = views.size() - 1; (!views[i]); i--) {
		views.resize(i);
	}                                       // delete empty views from the end
	if (id < 0) {                           // close the last one existing
		assert(*views.rbegin());        // this should have been sanitized by the loop above
		views.resize(views.size() - 1); // releases the pointer as well
	}
	if (id == 0) {
		LOG_DEBUG("Closing primary view.");
		if (views.size() == 1) {
			LOG_DEBUG("The currently closed view will be destroyed later. (QWindow insists on accessing the about to be destroyed instance)");
			viewsLater.push_back(views[0]);
			views.clear();
			// let QObject::deleteLater() do its job during these 250ms, see https://wiki.qt.io/Threads_Events_QObjects
			// whatever amount of time (e.g. 250ms) is OK, because this event is added at the end of the event queue.
			QTimer::singleShot(250, this, SLOT(cleanupViewsSlot()));
		} else {
			LOG_INFO("Cannot close primary view, secondary views still exist.");
		}
	}
}
void OpenGLManager::centerAllViews(const Real& suggestedRadius, const Vector3r& gridOrigin, const Vector3r& suggestedCenter, int gridDecimalPlaces)
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	for (const auto& g : views) {
		if (!g)
			continue;
		g->centerScene(suggestedRadius, gridOrigin, suggestedCenter, gridDecimalPlaces);
	}
}

Real OpenGLManager::getSuggestedRadius() const
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	for (const auto& g : views) {
		if (!g)
			continue;
		return g->getSuggestedRadius();
	};
	return -1;
}
Vector3r OpenGLManager::getGridOrigin() const
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	for (const auto& g : views) {
		if (!g)
			continue;
		return g->getGridOrigin();
	};
	return Vector3r(0, 0, 0);
}
Vector3r OpenGLManager::getSuggestedCenter() const
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	for (const auto& g : views) {
		if (!g)
			continue;
		return g->getSuggestedCenter();
	};
	return Vector3r(0, 0, 0);
}
int OpenGLManager::getGridDecimalPlaces() const
{
	const std::lock_guard<std::mutex> lock(viewsMutex);
	for (const auto& g : views) {
		if (!g)
			continue;
		return g->getGridDecimalPlaces();
	};
	return 4;
}

void OpenGLManager::startTimerSlot() { startTimer(50); }

int OpenGLManager::waitForNewView(double timeout, bool center)
{
	size_t origViewCount = views.size();
	emitCreateView();
	double t = 0;
	while (views.size() != origViewCount + 1) {
		usleep(50000);
		t += .05;
		// wait at most 5 secs
		if (t >= timeout) {
			LOG_ERROR("Timeout waiting for the new view to open, giving up.");
			return -1;
		}
	}
	if (center)
		(*views.rbegin())->centerScene();
	return (*views.rbegin())->viewId;
}

} // namespace yade
