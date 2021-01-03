/*************************************************************************
* Copyright (C) 2004 by Olivier Galizzi         *
* olivier.galizzi@imag.fr            *
* Copyright (C) 2004 by Janek Kozicki         *
* cosurgi@berlios.de             *
*                  *
* This program is free software; it is licensed under the terms of the *
* GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

// XXX never do #include<Python.h>, see https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/building/include_issues.html
#include <boost/python/detail/wrap_python.hpp>
#include <fstream>
#include <iostream>
#include <mutex>
#include <time.h>

#include <lib/base/Math.hpp>
#include <lib/factory/ClassFactory.hpp>

#include "SimulationFlow.hpp"
#include <lib/base/Singleton.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>
#include <set>

namespace yade { // Cannot have #include directive inside.

class Scene;
class ThreadRunner;

struct DynlibDescriptor {
	std::set<string> baseClasses;
	bool             isSerializable;
};

class Omega : public Singleton<Omega> {
	shared_ptr<ThreadRunner>           simulationLoop;
	SimulationFlow                     simulationFlow_;
	std::map<string, DynlibDescriptor> dynlibs;                                                // FIXME : should store that in ClassFactory ?
	void                               buildDynlibDatabase(const vector<string>& dynlibsList); // FIXME - maybe in ClassFactory ?

	vector<shared_ptr<Scene>> scenes;
	int                       currentSceneNb;
	shared_ptr<Scene>         sceneAnother; // used for temporarily running different simulation, in Omega().switchscene()

	boost::posix_time::ptime startupLocalTime;

	std::map<string, string> memSavedSimulations;

	// to avoid accessing simulation when it is being loaded (should avoid crashes with the UI)
	std::mutex  tmpFileCounterMutex;
	long        tmpFileCounter;
	std::string tmpFileDir;

public:
	// management, not generally useful
	void                                      init();
	void                                      reset();
	void                                      timeInit();
	void                                      initTemps();
	void                                      cleanupTemps();
	const std::map<string, DynlibDescriptor>& getDynlibsDescriptor();
	void                                      loadPlugins(vector<string> pluginFiles);
	bool                                      isInheritingFrom(const string& className, const string& baseClassName);
	bool                                      isInheritingFrom_recursive(const string& className, const string& baseClassName);
	void                                      createSimulationLoop();
	bool                                      hasSimulationLoop() { return (bool)(simulationLoop); }
	string                                    gdbCrashBatch;
	char**                                    origArgv;
	int                                       origArgc;
	// do not change by hand
	/* Mutex for:
		* 1. GLViewer::paintGL (deffered lock: if fails, no GL painting is done)
		* 2. other threads that wish to manipulate GL
		* 3. Omega when substantial changes to the scene are being made (bodies being deleted, simulation loaded etc) so that GL doesn't access those and crash */
	std::mutex renderMutex;

	void        run();
	void        pause();
	void        step();
	void        stop(); // resets the simulationLoop
	bool        isRunning();
	std::string sceneFile; // updated at load/save automatically
	void        loadSimulation(const string& name, bool quiet = false);
	void        saveSimulation(const string& name, bool quiet = false);

	void                     resetScene();
	void                     resetCurrentScene();
	void                     resetAllScenes();
	const shared_ptr<Scene>& getScene();
	void                     setScene(const shared_ptr<Scene>& source) { scenes[currentSceneNb] = source; }
	int                      addScene();
	void                     switchToScene(int i);
	//! Return unique temporary filename. May be deleted by the user; if not, will be deleted at shutdown.
	string                           tmpFilename();
	Real                             getRealTime();
	boost::posix_time::time_duration getRealTime_duration();

	// configuration directory used for logging config and possibly other things
	std::string confDir;

	DECLARE_LOGGER;

	Omega() { LOG_DEBUG("Constructing Omega."); }
	~Omega() { }

	FRIEND_SINGLETON(Omega);
	friend class pyOmega;
};

} // namespace yade
