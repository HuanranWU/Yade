/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2005 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "GLViewer.hpp"
#include "OpenGLManager.hpp"

#include <lib/base/TimedLogging.hpp>
#include <lib/high-precision/Constants.hpp>
#include <lib/opengl/OpenGLWrapper.hpp>
#include <lib/pyutil/gil.hpp>
#include <lib/serialization/ObjectIO.hpp>
#include <core/Body.hpp>
#include <core/DisplayParameters.hpp>
#include <core/Interaction.hpp>
#include <core/Scene.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iomanip>
#include <sstream>


#include <QtGui/qevent.h>

#ifdef YADE_GL2PS
#include <gl2ps.h>
#endif

namespace yade { // Cannot have #include directive inside.

void GLViewer::useDisplayParameters(size_t n)
{
	LOG_DEBUG("Loading display parameters from #" << n);
	vector<shared_ptr<DisplayParameters>>& dispParams = Omega::instance().getScene()->dispParams;
	if (dispParams.size() <= (size_t)n) {
		throw std::invalid_argument(("Display parameters #" + boost::lexical_cast<string>(n) + " don't exist (number of entries "
		                             + boost::lexical_cast<string>(dispParams.size()) + ")")
		                                    .c_str());
		;
		return;
	}
	const shared_ptr<DisplayParameters>& dp = dispParams[n];
	string                               val;
	if (dp->getValue("OpenGLRenderer", val)) {
		std::istringstream oglre(val);
		yade::ObjectIO::load<decltype(renderer), boost::archive::xml_iarchive>(oglre, "renderer", renderer);
	} else {
		LOG_WARN("OpenGLRenderer configuration not found in display parameters, skipped.");
	}
	if (dp->getValue("GLViewer", val)) {
		GLViewer::setState(val);
		displayMessage("Loaded view configuration #" + boost::lexical_cast<string>(n));
	} else {
		LOG_WARN("GLViewer configuration not found in display parameters, skipped.");
	}
}

void GLViewer::saveDisplayParameters(size_t n)
{
	LOG_DEBUG("Saving display parameters to #" << n);
	vector<shared_ptr<DisplayParameters>>& dispParams = Omega::instance().getScene()->dispParams;
	if (dispParams.size() <= n) {
		while (dispParams.size() <= n)
			dispParams.push_back(shared_ptr<DisplayParameters>(new DisplayParameters));
	}
	assert(n < dispParams.size());
	shared_ptr<DisplayParameters>& dp = dispParams[n];
	std::ostringstream             oglre;
	yade::ObjectIO::save<decltype(renderer), boost::archive::xml_oarchive>(oglre, "renderer", renderer);
	dp->setValue("OpenGLRenderer", oglre.str());
	dp->setValue("GLViewer", GLViewer::getState());
	displayMessage("Saved view configuration ot #" + boost::lexical_cast<string>(n));
}

void GLViewer::draw()
{
#ifdef YADE_GL2PS
	if (!nextFrameSnapshotFilename.empty() && boost::algorithm::ends_with(nextFrameSnapshotFilename, ".pdf")) {
		gl2psStream = fopen(nextFrameSnapshotFilename.c_str(), "wb");
		if (!gl2psStream) {
			int err = errno;
			throw runtime_error(string("Error opening file ") + nextFrameSnapshotFilename + ": " + strerror(err));
		}
		LOG_DEBUG("Start saving snapshot to " << nextFrameSnapshotFilename);
		size_t nBodies  = Omega::instance().getScene()->bodies->size();
		int    sortAlgo = (nBodies < 100 ? GL2PS_BSP_SORT : GL2PS_SIMPLE_SORT);
		gl2psBeginPage(
		        /*const char *title*/ "Some title",
		        /*const char *producer*/ "Yade",
		        /*GLint viewport[4]*/ NULL,
		        /*GLint format*/ GL2PS_PDF,
		        /*GLint sort*/ sortAlgo,
		        /*GLint options*/ GL2PS_SIMPLE_LINE_OFFSET | GL2PS_USE_CURRENT_VIEWPORT | GL2PS_TIGHT_BOUNDING_BOX | GL2PS_COMPRESS
		                | GL2PS_OCCLUSION_CULL | GL2PS_NO_BLENDING,
		        /*GLint colormode*/ GL_RGBA,
		        /*GLint colorsize*/ 0,
		        /*GL2PSrgba *colortable*/ NULL,
		        /*GLint nr*/ 0,
		        /*GLint ng*/ 0,
		        /*GLint nb*/ 0,
		        /*GLint buffersize*/ 4096 * 4096 /* 16MB */,
		        /*FILE *stream*/ gl2psStream,
		        /*const char *filename*/ NULL);
	}
#endif

	qglviewer::Vec vd       = camera()->viewDirection();
	renderer->viewDirection = Vector3r(vd[0], vd[1], vd[2]);
	if (Omega::instance().getScene()) {
		const shared_ptr<Scene>& scene     = Omega::instance().getScene();
		int                      selection = selectedName();
		if (selection != -1 && (*(Omega::instance().getScene()->bodies)).exists(selection) && isMoving) {
			static Real lastTimeMoved(0);
#if QGLVIEWER_VERSION >= 0x020603
			qreal v0, v1, v2;
			manipulatedFrame()->getPosition(v0, v1, v2);
#else
			float v0, v1, v2;
			manipulatedFrame()->getPosition(v0, v1, v2);
#endif
			if (last == selection) // delay by one redraw, so the body will not jump into 0,0,0 coords
			{
				Quaternionr& q      = (*(Omega::instance().getScene()->bodies))[selection]->state->ori;
				Vector3r&    v      = (*(Omega::instance().getScene()->bodies))[selection]->state->pos;
				Vector3r&    vel    = (*(Omega::instance().getScene()->bodies))[selection]->state->vel;
				Vector3r&    angVel = (*(Omega::instance().getScene()->bodies))[selection]->state->angVel;
				angVel              = Vector3r::Zero();
				Real dt             = (scene->time - lastTimeMoved);
				lastTimeMoved       = scene->time;
				if (dt != 0) {
					vel[0] = -(v[0] - v0) / dt;
					vel[1] = -(v[1] - v1) / dt;
					vel[2] = -(v[2] - v2) / dt;
				} else
					vel[0] = vel[1] = vel[2] = 0;
				//FIXME: should update spin like velocity above, when the body is rotated:
				double q0, q1, q2, q3;
				manipulatedFrame()->getOrientation(q0, q1, q2, q3);
				q.x() = q0;
				q.y() = q1;
				q.z() = q2;
				q.w() = q3;
			}
			(*(Omega::instance().getScene()->bodies))[selection]->userForcedDisplacementRedrawHook();
		}
		if (manipulatedClipPlane >= 0) {
			assert(manipulatedClipPlane < renderer->numClipPlanes);
#if QGLVIEWER_VERSION >= 0x020603
			qreal v0, v1, v2;
			manipulatedFrame()->getPosition(v0, v1, v2);
#else
			float v0, v1, v2;
			manipulatedFrame()->getPosition(v0, v1, v2);
#endif
			double q0, q1, q2, q3;
			manipulatedFrame()->getOrientation(q0, q1, q2, q3);
			Se3r newSe3(Vector3r(v0, v1, v2), Quaternionr(q0, q1, q2, q3));
			newSe3.orientation.normalize();
			const Se3r& oldSe3 = renderer->clipPlaneSe3[manipulatedClipPlane];
			for (const auto& planeId : boundClipPlanes) {
				if (planeId >= renderer->numClipPlanes || !renderer->clipPlaneActive[planeId] || planeId == manipulatedClipPlane)
					continue;
				Se3r&       boundSe3  = renderer->clipPlaneSe3[planeId];
				Quaternionr relOrient = oldSe3.orientation.conjugate() * boundSe3.orientation;
				relOrient.normalize();
				Vector3r relPos      = oldSe3.orientation.conjugate() * (boundSe3.position - oldSe3.position);
				boundSe3.position    = newSe3.position + newSe3.orientation * relPos;
				boundSe3.orientation = newSe3.orientation * relOrient;
				boundSe3.orientation.normalize();
			}
			renderer->clipPlaneSe3[manipulatedClipPlane] = newSe3;
		}
		scene->renderer = renderer;
		renderer->render(scene, selectedName());
	}
}

void GLViewer::drawWithNames()
{
	qglviewer::Vec vd       = camera()->viewDirection();
	renderer->viewDirection = Vector3r(vd[0], vd[1], vd[2]);
	const shared_ptr<Scene> scene(Omega::instance().getScene());
	scene->renderer = renderer;
	renderer->scene = scene;
	renderer->renderShape();
}

std::pair<double, qglviewer::Vec> GLViewer::displayedSceneRadiusCenter()
{
	const auto w2 = width() / 2;
	const auto h2 = height() / 2;
	return { (camera()->unprojectedCoordinatesOf(qglviewer::Vec(w2, h2, 0.5)) - camera()->unprojectedCoordinatesOf(qglviewer::Vec(0, 0, 0.5))).norm(),
		 camera()->unprojectedCoordinatesOf(qglviewer::Vec(w2 /* pixels */, h2 /* pixels */, /* middle between near plane and far plane */ 0.5)) };
}

void GLViewer::drawReadableNum(const Real& n, const Vector3r& pos, unsigned precision, const Vector3r& color)
{
	// Also XOR-ing is possible with these commands: glEnable(GL_COLOR_LOGIC_OP); glLogicOp(GL_XOR); ………… glDisable(GL_COLOR_LOGIC_OP);
	std::ostringstream oss;
	oss << std::setprecision(precision) << n;
	drawReadableText(oss.str(), pos, color);
}

void GLViewer::drawReadableText(const std::string& txt, const Vector3r& pos, const Vector3r& color)
{
	auto opposite = Vector3r::Ones() - color;
	// draw shifted text
	drawTextWithPixelShift(txt, pos, Vector2i(1, 0), opposite);
	// draw the real text
	drawTextWithPixelShift(txt, pos, Vector2i(0, 0), color);
}

void GLViewer::drawTextWithPixelShift(const std::string& txt, const Vector3r& pos, const Vector2i& pixelShift, const Vector3r& color)
{
	glColor3v(color);
	qglviewer::Vec p = QGLViewer::camera()->projectedCoordinatesOf(
	        qglviewer::Vec((static_cast<double>(pos[0])), (static_cast<double>(pos[1])), (static_cast<double>(pos[2]))));
	if (p[0] > 0 and p[0] < this->width() and p[1] > 0 and p[1] < this->height()) // don't waste time to draw outside window frame.
		QGLViewer::drawText(static_cast<int>(p[0] + pixelShift[0]), static_cast<int>(p[1] + pixelShift[1]), txt.c_str());
}

void GLViewer::postDraw()
{
	using math::max;
	using math::min; // when used inside function it does not leak
	Real wholeDiameter = QGLViewer::camera()->sceneRadius() * 2;

	renderer->viewInfo.sceneRadius = QGLViewer::camera()->sceneRadius();
	qglviewer::Vec c               = QGLViewer::camera()->sceneCenter();
	renderer->viewInfo.sceneCenter = Vector3r(c[0], c[1], c[2]);

	const auto radiusCenter = displayedSceneRadiusCenter();

	const Real dispDiameter = min(
	        wholeDiameter, max(static_cast<Real>(radiusCenter.first * 2.), wholeDiameter / 1e3)); // limit to avoid drawing 1e5 lines with big zoom level
	//qglviewer::Vec center=QGLViewer::camera()->sceneCenter();
	Real gridStep(requestedGridStep);
	if (autoGrid)
		gridStep = pow(10, (floor(0.5 + log10(dispDiameter))));
	glPushMatrix();

	auto nHalfSegments = ((int)(wholeDiameter / gridStep)) + 1;
	auto nSegments     = static_cast<int>(2 * nHalfSegments);
	if (nSegments > 500) {
		LOG_TIMED_WARN(
		        10s,
		        "More than 500 grid segments (currently: "
		                << nSegments << ") take too long to draw, using previous value: " << prevSegments
		                << ". If you need denser grid try calling: yade.qt.center(suggestedRadius,gridOrigin,suggestedCenter,gridDecimalPlaces); (each "
		                   "parameter is optional) to reduce scene grid radius. Current values are: yade.qt.center(suggestedRadius="
		                << QGLViewer::camera()->sceneRadius() << ",gridOrigin=(" << gridOrigin[0] << "," << gridOrigin[1] << "," << gridOrigin[2]
		                << "),suggestedCenter=(" << QGLViewer::camera()->sceneCenter()[0] << "," << QGLViewer::camera()->sceneCenter()[1] << ","
		                << QGLViewer::camera()->sceneCenter()[2] << "),gridDecimalPlaces=" << gridDecimalPlaces
		                << ")\nPress '-' (decrease grid density) in View window to remove this warning.\n");
		nSegments = prevSegments;
		gridStep  = prevGridStep;
	}
	prevGridStep = gridStep;
	if (autoGrid)
		requestedGridStep = gridStep;
	nHalfSegments       = ((int)(wholeDiameter / gridStep)) + 1;
	const auto realSize = nHalfSegments * gridStep;
	//LOG_TRACE("nHalfSegments="<<nHalfSegments<<",gridStep="<<gridStep<<",realSize="<<realSize);
	prevSegments = nSegments;
	// round requested gridOrigin to nearest nicely-readable value
	auto gridCen = Vector3r(0, 0, 0);
	if (gridOrigin != Vector3r(0, 0, 0)) {
		gridCen = Vector3r(
		        gridOrigin[0] - math::remainder(gridOrigin[0], gridStep),
		        gridOrigin[1] - math::remainder(gridOrigin[1], gridStep),
		        gridOrigin[2] - math::remainder(gridOrigin[2], gridStep));
	}
	// XYZ grids
	glLineWidth(.5);
	if (drawGrid & 1) {
		glColor3(0.6, 0.3, 0.3);
		glPushMatrix();
		glTranslatev(gridCen);
		glRotated(90., 0., 1., 0.);
		QGLViewer::drawGrid(static_cast<double>(realSize), nSegments);
		glPopMatrix();
	}
	if (drawGrid & 2) {
		glColor3(0.3, 0.6, 0.3);
		glPushMatrix();
		glTranslatev(gridCen);
		glRotated(90., 1., 0., 0.);
		QGLViewer::drawGrid(static_cast<double>(realSize), nSegments);
		glPopMatrix();
	}
	if (drawGrid & 4) {
		glColor3(0.3, 0.3, 0.6);
		glPushMatrix();
		glTranslatev(gridCen); /*glRotated(90.,0.,1.,0.);*/
		QGLViewer::drawGrid(static_cast<double>(realSize), nSegments);
		glPopMatrix();
	}
	if (gridSubdivide) {
		if (drawGrid & 1) {
			glColor3(0.4, 0.1, 0.1);
			glPushMatrix();
			glTranslatev(gridCen);
			glRotated(90., 0., 1., 0.);
			QGLViewer::drawGrid(static_cast<double>(realSize), nSegments * 10);
			glPopMatrix();
		}
		if (drawGrid & 2) {
			glColor3(0.1, 0.4, 0.1);
			glPushMatrix();
			glTranslatev(gridCen);
			glRotated(90., 1., 0., 0.);
			QGLViewer::drawGrid(static_cast<double>(realSize), nSegments * 10);
			glPopMatrix();
		}
		if (drawGrid & 4) {
			glColor3(0.1, 0.1, 0.4);
			glPushMatrix();
			glTranslatev(gridCen); /*glRotated(90.,0.,1.,0.);*/
			QGLViewer::drawGrid(static_cast<double>(realSize), nSegments * 10);
			glPopMatrix();
		}
	}
	if (displayGridNumbers and drawGrid) {
		// disabling lighting & depth test makes sure that text is always above everything, readable.
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		for (int xyz(-nHalfSegments); xyz <= nHalfSegments; xyz++) { // write text - coordinate numbers on grid
			Real pos = xyz * gridStep;
			if ((drawGrid & 2) or (drawGrid & 4))
				drawReadableNum(pos + gridCen[0], Vector3r(pos, 0, 0) + gridCen, gridDecimalPlaces);
			if ((drawGrid & 1) or (drawGrid & 4))
				drawReadableNum(pos + gridCen[1], Vector3r(0, pos, 0) + gridCen, gridDecimalPlaces);
			if ((drawGrid & 1) or (drawGrid & 2))
				drawReadableNum(pos + gridCen[2], Vector3r(0, 0, pos) + gridCen, gridDecimalPlaces);
		}
		Real pos = nHalfSegments * gridStep + gridStep * 0.1;
		if ((drawGrid & 2) or (drawGrid & 4))
			drawReadableText("X", Vector3r(pos, 0, 0) + gridCen);
		if ((drawGrid & 1) or (drawGrid & 4))
			drawReadableText("Y", Vector3r(0, pos, 0) + gridCen);
		if ((drawGrid & 1) or (drawGrid & 2))
			drawReadableText("Z", Vector3r(0, 0, pos) + gridCen);
		if ((drawGrid & 2) or (drawGrid & 4))
			drawReadableText("-X", Vector3r(-pos, 0, 0) + gridCen);
		if ((drawGrid & 1) or (drawGrid & 4))
			drawReadableText("-Y", Vector3r(0, -pos, 0) + gridCen);
		if ((drawGrid & 1) or (drawGrid & 2))
			drawReadableText("-Z", Vector3r(0, 0, -pos) + gridCen);
		// enable back lighting & depth test
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	}

	// scale
	if (drawScale) {
		const Real     segmentSize = pow(10, (floor(log10(radiusCenter.first * 2) - .7))); // unconstrained
		qglviewer::Vec screenDxDy[3];                                                      // dx,dy for x,y,z scale segments
		int            extremalDxDy[2] = { 0, 0 };
		for (int axis = 0; axis < 3; axis++) {
			qglviewer::Vec delta(0, 0, 0);
			delta[axis]           = static_cast<double>(segmentSize);
			qglviewer::Vec center = radiusCenter.second;
			screenDxDy[axis]      = camera()->projectedCoordinatesOf(center + delta) - camera()->projectedCoordinatesOf(center);
			for (int xy = 0; xy < 2; xy++)
				extremalDxDy[xy]
				        = int(std::round(axis > 0 ? std::min(extremalDxDy[xy], int(std::round(screenDxDy[axis][xy]))) : screenDxDy[axis][xy]));
		}
		//LOG_DEBUG("Screen offsets for axes: "<<" x("<<screenDxDy[0][0]<<","<<screenDxDy[0][1]<<") y("<<screenDxDy[1][0]<<","<<screenDxDy[1][1]<<") z("<<screenDxDy[2][0]<<","<<screenDxDy[2][1]<<")");
		int margin = 10; // screen pixels
		int scaleCenter[2];
		scaleCenter[0] = std::abs(extremalDxDy[0]) + margin;
		scaleCenter[1] = std::abs(extremalDxDy[1]) + margin;
		//LOG_DEBUG("Center of scale "<<scaleCenter[0]<<","<<scaleCenter[1]);
		//displayMessage(QString().sprintf("displayed scene radius %g",radiusCenter.first));
		startScreenCoordinatesSystem();
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glLineWidth(3.0);
		for (int axis = 0; axis < 3; axis++) {
			Vector3r color(.4, .4, .4);
			color[axis] = .9;
			glColor3v(color);
			glBegin(GL_LINES)
				;
				glVertex2f(scaleCenter[0], scaleCenter[1]);
				glVertex2(scaleCenter[0] + screenDxDy[axis][0], scaleCenter[1] + screenDxDy[axis][1]);
			glEnd();
		}
		glLineWidth(1.);
		glEnable(GL_DEPTH_TEST);
		std::ostringstream oss {};
		oss << std::setprecision(3) << segmentSize;
		QGLViewer::drawText(scaleCenter[0], scaleCenter[1], oss.str().c_str());
		stopScreenCoordinatesSystem();
	}

	// cutting planes (should be moved to OpenGLRenderer perhaps?)
	// only painted if one of those is being manipulated
	if (manipulatedClipPlane >= 0) {
		for (int planeId = 0; planeId < renderer->numClipPlanes; planeId++) {
			if (!renderer->clipPlaneActive[planeId] && planeId != manipulatedClipPlane)
				continue;
			glPushMatrix();
			const Se3r& se3 = renderer->clipPlaneSe3[planeId];
			AngleAxisr  aa(se3.orientation);
			glTranslate(se3.position[0], se3.position[1], se3.position[2]);
			glRotate(aa.angle() * Mathr::RAD_TO_DEG, aa.axis()[0], aa.axis()[1], aa.axis()[2]);
			Real cff = 1;
			if (!renderer->clipPlaneActive[planeId])
				cff = .4;
			glColor3(
			        max((Real)0., cff * cos(planeId)), max((Real)0., cff * sin(planeId)), Real(planeId == manipulatedClipPlane)); // variable colors
			QGLViewer::drawGrid(static_cast<double>(realSize), 2 * nSegments);
			drawArrow(static_cast<double>(wholeDiameter / 6));
			glPopMatrix();
		}
	}

	Scene* rb = Omega::instance().getScene().get();
#define _W3 setw(3) << setfill('0')
#define _W2 setw(2) << setfill('0')
	if (timeDispMask != 0) {
		const int lineHt = 13;
		unsigned  x = 10, y = height() - 3 - lineHt * 2;
		glColor3v(Vector3r(1, 1, 1));
		if (timeDispMask & GLViewer::TIME_VIRT) {
			std::ostringstream oss;
			const Real&        t   = Omega::instance().getScene()->time;
			unsigned           min = ((unsigned)t / 60), sec = (((unsigned)t) % 60), msec = ((unsigned)(1e3 * t)) % 1000,
			         usec = ((unsigned long)(1e6 * t)) % 1000, nsec = ((unsigned long)(1e9 * t)) % 1000;
			if (min > 0)
				oss << _W2 << min << ":" << _W2 << sec << "." << _W3 << msec << "m" << _W3 << usec << "u" << _W3 << nsec << "n";
			else if (sec > 0)
				oss << _W2 << sec << "." << _W3 << msec << "m" << _W3 << usec << "u" << _W3 << nsec << "n";
			else if (msec > 0)
				oss << _W3 << msec << "m" << _W3 << usec << "u" << _W3 << nsec << "n";
			else if (usec > 0)
				oss << _W3 << usec << "u" << _W3 << nsec << "n";
			else
				oss << _W3 << nsec << "ns";
			QGLViewer::drawText(x, y, oss.str().c_str());
			y -= lineHt;
		}
		glColor3v(Vector3r(0, .5, .5));
		if (timeDispMask & GLViewer::TIME_REAL) {
			QGLViewer::drawText(x, y, getRealTimeString().c_str() /* virtual, since player gets that from db */);
			y -= lineHt;
		}
		if (timeDispMask & GLViewer::TIME_ITER) {
			std::ostringstream oss;
			oss << "#" << rb->iter;
			if (rb->stopAtIter > rb->iter)
				oss << " (" << setiosflags(ios::fixed) << setw(3) << setprecision(1) << setfill('0') << (100. * rb->iter) / rb->stopAtIter
				    << "%)";
			QGLViewer::drawText(x, y, oss.str().c_str());
			y -= lineHt;
		}
		if (drawGrid) {
			glColor3v(Vector3r(1, 1, 0));
			std::ostringstream oss;
			oss << "grid: " << setprecision(4) << gridStep;
			if (gridSubdivide)
				oss << " (minor " << setprecision(3) << gridStep * .1 << ")";
			QGLViewer::drawText(x, y, oss.str().c_str());
			y -= lineHt;
		}
	}
	QGLViewer::postDraw();
	if (!nextFrameSnapshotFilename.empty()) {
#ifdef YADE_GL2PS
		if (boost::algorithm::ends_with(nextFrameSnapshotFilename, ".pdf")) {
			gl2psEndPage();
			LOG_DEBUG("Finished saving snapshot to " << nextFrameSnapshotFilename);
			fclose(gl2psStream);
		} else
#endif
		{
			// save the snapshot
			saveSnapshot(QString(nextFrameSnapshotFilename.c_str()), /*overwrite*/ true);
		}
		// notify the caller that it is done already (probably not an atomic op :-|, though)
		nextFrameSnapshotFilename.clear();
	}
}

} // namespace yade
