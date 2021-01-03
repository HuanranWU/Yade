#include "GLViewer.hpp"
#include "OpenGLManager.hpp"
#include <lib/base/AliasNamespaces.hpp>
#include <lib/base/Logging.hpp>
#include <lib/pyutil/doc_opts.hpp>
#include <pkg/common/OpenGLRenderer.hpp>
#include <QApplication>
#include <QCloseEvent>

CREATE_CPP_LOCAL_LOGGER("qt5/_GLViewer.cpp");

namespace yade { // Cannot have #include directive inside.

qglviewer::Vec tuple2vec(py::tuple t)
{
	qglviewer::Vec ret;
	for (int i = 0; i < 3; i++) {
		py::extract<Real> e(t[i]);
		if (!e.check())
			throw invalid_argument("Element #" + boost::lexical_cast<string>(i) + " is not a number");
		ret[i] = static_cast<qreal>(e());
	}
	return ret;
};
py::tuple vec2tuple(qglviewer::Vec v) { return py::make_tuple(v[0], v[1], v[2]); };

class pyGLViewer {
	const size_t viewNo;

public:
#define GLV                                                                                                                                                    \
	if ((OpenGLManager::self->views.size() <= viewNo) || !(OpenGLManager::self->views[viewNo]))                                                            \
		throw runtime_error("No view #" + boost::lexical_cast<string>(viewNo));                                                                        \
	GLViewer* glv = OpenGLManager::self->views[viewNo].get();
	pyGLViewer(size_t _viewNo = 0)
	        : viewNo(_viewNo)
	{
	}
	void close()
	{
		GLV;
		QCloseEvent* e(new QCloseEvent);
		QApplication::postEvent(glv, e);
	}
	py::tuple get_grid()
	{
		GLV;
		return py::make_tuple(bool(glv->drawGrid & 1), bool(glv->drawGrid & 2), bool(glv->drawGrid & 4));
	}
	void set_grid(py::tuple t)
	{
		GLV;
		glv->drawGrid = 0;
		for (int i = 0; i < 3; i++)
			if (py::extract<bool>(t[i])())
				glv->drawGrid += 1 << i;
	}
#define VEC_GET_SET(property, getter, setter)                                                                                                                  \
	Vector3r get_##property()                                                                                                                              \
	{                                                                                                                                                      \
		GLV;                                                                                                                                           \
		qglviewer::Vec v = getter();                                                                                                                   \
		return Vector3r(v[0], v[1], v[2]);                                                                                                             \
	}                                                                                                                                                      \
	void set_##property(const Vector3r& t)                                                                                                                 \
	{                                                                                                                                                      \
		GLV;                                                                                                                                           \
		setter(qglviewer::Vec((static_cast<double>(t[0])), (static_cast<double>(t[1])), (static_cast<double>(t[2]))));                                 \
	}
	VEC_GET_SET(upVector, glv->camera()->upVector, glv->camera()->setUpVector);
	VEC_GET_SET(lookAt, glv->camera()->position() + glv->camera()->viewDirection, glv->camera()->lookAt);
	VEC_GET_SET(viewDir, glv->camera()->viewDirection, glv->camera()->setViewDirection);
	VEC_GET_SET(eyePosition, glv->camera()->position, glv->camera()->setPosition);
#define BOOL_GET_SET(property, getter, setter)                                                                                                                 \
	void set_##property(bool b)                                                                                                                            \
	{                                                                                                                                                      \
		GLV;                                                                                                                                           \
		setter(b);                                                                                                                                     \
	}                                                                                                                                                      \
	bool get_##property()                                                                                                                                  \
	{                                                                                                                                                      \
		GLV;                                                                                                                                           \
		return getter();                                                                                                                               \
	}
	BOOL_GET_SET(axes, glv->axisIsDrawn, glv->setAxisIsDrawn);
	BOOL_GET_SET(fps, glv->FPSIsDisplayed, glv->setFPSIsDisplayed);
	bool get_scale()
	{
		GLV;
		return glv->drawScale;
	}
	void set_scale(bool b)
	{
		GLV;
		glv->drawScale = b;
	}
	bool get_orthographic()
	{
		GLV;
		return glv->camera()->type() == qglviewer::Camera::ORTHOGRAPHIC;
	}
	void set_orthographic(bool b)
	{
		GLV;
		return glv->camera()->setType(b ? qglviewer::Camera::ORTHOGRAPHIC : qglviewer::Camera::PERSPECTIVE);
	}
	int get_selection(void)
	{
		GLV;
		return glv->selectedName();
	}
	void set_selection(int s)
	{
		GLV;
		glv->setSelectedName(s);
	}
#define FLOAT_GET_SET(property, getter, setter)                                                                                                                \
	void set_##property(Real r)                                                                                                                            \
	{                                                                                                                                                      \
		GLV;                                                                                                                                           \
		setter(static_cast<qreal>(r));                                                                                                                 \
	}                                                                                                                                                      \
	Real get_##property()                                                                                                                                  \
	{                                                                                                                                                      \
		GLV;                                                                                                                                           \
		return getter();                                                                                                                               \
	}
	FLOAT_GET_SET(sceneRadius, glv->sceneRadius, glv->setSceneRadius);
	void fitAABB(const Vector3r& min, const Vector3r& max)
	{
		GLV;
		glv->camera()->fitBoundingBox(
		        qglviewer::Vec((static_cast<double>(min[0])), (static_cast<double>(min[1])), (static_cast<double>(min[2]))),
		        qglviewer::Vec((static_cast<double>(max[0])), (static_cast<double>(max[1])), (static_cast<double>(max[2]))));
	}
	void fitSphere(const Vector3r& center, Real radius)
	{
		GLV;
		glv->camera()->fitSphere(
		        qglviewer::Vec((static_cast<double>(center[0])), (static_cast<double>(center[1])), (static_cast<double>(center[2]))),
		        static_cast<double>(radius));
	}
	void showEntireScene()
	{
		GLV;
		glv->camera()->showEntireScene();
	}
	void center(bool median, Real suggestedRadius)
	{
		GLV;
		if (median)
			glv->centerMedianQuartile();
		else
			glv->centerScene(suggestedRadius);
	}
	Vector2i get_screenSize()
	{
		GLV;
		return Vector2i(glv->width(), glv->height());
	}
	void set_screenSize(Vector2i t)
	{ /*GLV;*/
		OpenGLManager::self->emitResizeView(viewNo, t[0], t[1]);
	}
	string pyStr() { return string("<GLViewer for view #") + boost::lexical_cast<string>(viewNo) + ">"; }
	void   saveDisplayParameters(size_t n)
	{
		GLV;
		glv->saveDisplayParameters(n);
	}
	void useDisplayParameters(size_t n)
	{
		GLV;
		glv->useDisplayParameters(n);
	}
	void loadState(string filename)
	{
		GLV;
		QString origStateFileName = glv->stateFileName();
		glv->setStateFileName(QString(filename.c_str()));
		glv->restoreStateFromFile();
		glv->saveStateToFile();
		glv->setStateFileName(origStateFileName);
	}
	void saveState(string filename)
	{
		GLV;
		QString origStateFileName = glv->stateFileName();
		glv->setStateFileName(QString(filename.c_str()));
		glv->saveStateToFile();
		glv->setStateFileName(origStateFileName);
	}
	string get_timeDisp()
	{
		GLV;
		const int& m(glv->timeDispMask);
		string     ret;
		if (m & GLViewer::TIME_REAL)
			ret += 'r';
		if (m & GLViewer::TIME_VIRT)
			ret += "v";
		if (m & GLViewer::TIME_ITER)
			ret += "i";
		return ret;
	}
	void set_timeDisp(string s)
	{
		GLV;
		int& m(glv->timeDispMask);
		m = 0;
		FOREACH(char c, s)
		{
			switch (c) {
				case 'r': m |= GLViewer::TIME_REAL; break;
				case 'v': m |= GLViewer::TIME_VIRT; break;
				case 'i': m |= GLViewer::TIME_ITER; break;
				default: throw invalid_argument(string("Invalid flag for timeDisp: `") + c + "'");
			}
		}
	}
	void set_bgColor(const Vector3r& c)
	{
		QColor cc(int(math::round(255 * c[0])), int(math::round(255 * c[1])), int(math::round(255 * c[2])));
		GLV;
		glv->setBackgroundColor(cc);
	}
	Vector3r get_bgColor()
	{
		GLV;
		QColor c(glv->backgroundColor());
		return Vector3r(c.red() / 255., c.green() / 255., c.blue() / 255.);
	}
	void saveSnapshot(string filename)
	{
		GLV;
		glv->nextFrameSnapshotFilename = filename;
	}
#undef GLV
#undef VEC_GET_SET
#undef BOOL_GET_SET
#undef FLOAT_GET_SET
};

// ask to create a new view and wait till it exists
pyGLViewer createView()
{
	int id = OpenGLManager::self->waitForNewView();
	if (id < 0)
		throw std::runtime_error("Unable to open new 3d view.");
	return pyGLViewer((*OpenGLManager::self->views.rbegin())->viewId);
}

py::list getAllViews()
{
	py::list ret;
	FOREACH(const shared_ptr<GLViewer>& v, OpenGLManager::self->views)
	{
		if (v)
			ret.append(pyGLViewer(v->viewId));
	}
	return ret;
};
void centerViews(const Real& suggestedRadius, const Vector3r& gridOrigin, const Vector3r& suggestedCenter, int gridDecimalPlaces)
{
	OpenGLManager::self->centerAllViews(suggestedRadius, gridOrigin, suggestedCenter, gridDecimalPlaces);
}
py::dict centerValues()
{
	py::dict ret;
	ret["suggestedRadius"]   = OpenGLManager::self->getSuggestedRadius();
	ret["gridOrigin"]        = OpenGLManager::self->getGridOrigin();
	ret["suggestedCenter"]   = OpenGLManager::self->getSuggestedCenter();
	ret["gridDecimalPlaces"] = OpenGLManager::self->getGridDecimalPlaces();
	return ret;
}

shared_ptr<OpenGLRenderer> getRenderer() { return OpenGLManager::self->renderer; }

} // namespace yade

// BOOST_PYTHON_MODULE cannot be inside yade namespace, it has 'extern "C"' keyword, which strips it out of any namespaces.
BOOST_PYTHON_MODULE(_GLViewer)
try {
	namespace y  = ::yade;
	namespace py = ::boost::python;

	YADE_SET_DOCSTRING_OPTS;

	y::OpenGLManager* glm = new y::OpenGLManager(); // keep this singleton object forever
	glm->emitStartTimer();

	py::def("View", y::createView, "Create a new 3d view.");
	py::def("centerValues",
	        y::centerValues,
	        ":return: a dictionary with all parameters currently used by ``yade.qt.center(…)``, see :yref:`yade.qt.center<yade.qt._GLViewer.center>` or "
	        "type ``yade.qt.center?`` for details. Returns zeros if view is closed.");
	py::def("center",
	        y::centerViews,
	        (py::arg("suggestedRadius")   = -1.0,
	         py::arg("gridOrigin")        = y::Vector3r(0, 0, 0),
	         py::arg("suggestedCenter")   = y::Vector3r(0, 0, 0),
	         py::arg("gridDecimalPlaces") = 4),
	        R"""(
Center all views.

:param suggestedRadius:   optional parameter, if provided and positive then it will be used instead of automatic radius detection. This parameter affects the (1) size of grid being drawn (2) the Z-clipping distance in OpenGL, it means that if clipping is too large and some of your scene is not being drawn but is "cut" or "sliced" then this parameter needs to be bigger.
:param gridOrigin:        optional parameter, if provided it will be used as the origin for drawing the grid. Meaning the intersection of all three grids will not be at 0,0,0; but at the provided coordinate rounded to the nearest gridStep.
:param suggestedCenter:   optional parameter, if provided other than (0,0,0) then it will be used instead of automatic calculation of scene center using bounding boxes. This parameter affects the drawn rotation-center. If you try to rotate the view, and the rotation is around some strange point, then this parameter needs to be changed.
:param gridDecimalPlaces: default value=4, determines the number of decimal places to be shown on grid labels using stringstream (extra zeros are not shown).

.. note:: You can get the current values of all these four arguments by invoking command: :yref:`yade.qt.centerValues()<yade.qt._GLViewer.centerValues>`

)""");
	py::def("views", y::getAllViews, R"""(

:return: a list of all open :yref:`yade.qt.GLViewer` objects

If one needs to exactly copy camera position and settings between two different yade sessions, the following commands can be used:

.. code-block:: python

  v=yade.qt.views()[0]                           ## to obtain a handle of currently opened view.
  v.lookAt, v.viewDir, v.eyePosition, v.upVector ## to print the current camera parameters of the view.

  ## Then copy the output of this command into the second yade session to reposition the camera.
  v.lookAt, v.viewDir, v.eyePosition, v.upVector = (Vector3(-0.5,1.6,0.47),Vector3(-0.5,0.6,0.4),Vector3(0.015,0.98,-0.012),Vector3(0.84,0.46,0.27))
  ## Since these parameters depend on each other it might be necessary to execute this command twice.

Also one can call :yref:`yade.qt.centerValues()<yade.qt._GLViewer.centerValues>` to obtain current settings of axis and scene radius (if defaults are not used) and apply them via call to :yref:`yade.qt.center<yade.qt._GLViewer.center>` in the second yade session.

This cumbersome method above may be improved in the future.

)""");

	py::def("Renderer", &y::getRenderer, "Return the active :yref:`OpenGLRenderer` object.");

	using pyGLViewer = yade::pyGLViewer;
	py::class_<pyGLViewer>("GLViewer", py::no_init)
	        .add_property("upVector", &pyGLViewer::get_upVector, &pyGLViewer::set_upVector, "Vector that will be shown oriented up on the screen.")
	        .add_property("lookAt", &pyGLViewer::get_lookAt, &pyGLViewer::set_lookAt, "Point at which camera is directed.")
	        .add_property("viewDir", &pyGLViewer::get_viewDir, &pyGLViewer::set_viewDir, "Camera orientation (as vector).")
	        .add_property("eyePosition", &pyGLViewer::get_eyePosition, &pyGLViewer::set_eyePosition, "Camera position.")
	        .add_property(
	                "grid", &pyGLViewer::get_grid, &pyGLViewer::set_grid, "Display square grid in zero planes, as 3-tuple of bools for yz, xz, xy planes.")
	        .add_property("fps", &pyGLViewer::get_fps, &pyGLViewer::set_fps, "Show frames per second indicator.")
	        .add_property("axes", &pyGLViewer::get_axes, &pyGLViewer::set_axes, "Show arrows for axes.")
	        .add_property("scale", &pyGLViewer::get_scale, &pyGLViewer::set_scale, "Scale of the view (?)")
	        .add_property("sceneRadius", &pyGLViewer::get_sceneRadius, &pyGLViewer::set_sceneRadius, "Visible scene radius.")
	        .add_property(
	                "ortho",
	                &pyGLViewer::get_orthographic,
	                &pyGLViewer::set_orthographic,
	                "Whether orthographic projection is used; if false, use perspective projection.")
	        .add_property("screenSize", &pyGLViewer::get_screenSize, &pyGLViewer::set_screenSize, "Size of the viewer's window, in screen pixels")
	        .add_property(
	                "timeDisp",
	                &pyGLViewer::get_timeDisp,
	                &pyGLViewer::set_timeDisp,
	                "Time displayed on in the vindow; is a string composed of characters *r*, *v*, *i* standing respectively for real time, virtual time, "
	                "iteration number.")
	        // .add_property("bgColor",&pyGLViewer::get_bgColor,&pyGLViewer::set_bgColor) // useless: OpenGLRenderer::Background_color is used via openGL directly, bypassing QGLViewer background property
	        .def("fitAABB",
	             &pyGLViewer::fitAABB,
	             (py::arg("mn"), py::arg("mx")),
	             "Adjust scene bounds so that Axis-aligned bounding box given by its lower and upper corners *mn*, *mx* fits in.")
	        .def("fitSphere",
	             &pyGLViewer::fitSphere,
	             (py::arg("center"), py::arg("radius")),
	             "Adjust scene bounds so that sphere given by *center* and *radius* fits in.")
	        .def("showEntireScene", &pyGLViewer::showEntireScene)
	        .def("center",
	             &pyGLViewer::center,
	             (py::arg("median") = true, py::arg("suggestedRadius") = -1.0f),
	             "Center view. View is centered either so that all bodies fit inside (*median* = False), or so that 75\% of bodies fit inside (*median* = "
	             "True). If radius cannot be determined automatically then suggestedRadius is used.")
	        .def("saveState",
	             &pyGLViewer::saveState,
	             (py::arg("stateFilename") = ".qglviewer.xml"),
	             "Save display parameters into a file. Saves state for both :yref:`GLViewer<yade._qt.GLViewer>` and associated :yref:`OpenGLRenderer`.")
	        .def("loadState",
	             &pyGLViewer::loadState,
	             (py::arg("stateFilename") = ".qglviewer.xml"),
	             "Load display parameters from file saved previously into.")
	        .def("__repr__", &pyGLViewer::pyStr)
	        .def("__str__", &pyGLViewer::pyStr)
	        .def("close", &pyGLViewer::close)
	        .def("saveSnapshot", &pyGLViewer::saveSnapshot, (py::arg("filename")), "Save the current view to image file")
	        .add_property("selection", &pyGLViewer::get_selection, &pyGLViewer::set_selection);

} catch (...) {
	LOG_FATAL("Importing this module caused an exception and this module is in an inconsistent state now.");
	PyErr_Print();
	PyErr_SetString(PyExc_SystemError, __FILE__);
	boost::python::handle_exception();
	throw;
}
