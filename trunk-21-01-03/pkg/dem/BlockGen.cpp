/*CWBoon 2016 */
/* Please cite: */
/* CW Boon, GT Houlsby, S Utili (2015).  A new rock slicing method based on linear programming.  Computers and Geotechnics 65, 12-29. */
/* The numerical library is changed from CPLEX to CLP because subscription to the academic initiative is required to use CPLEX for free */
#ifdef YADE_POTENTIAL_BLOCKS

#include <lib/high-precision/Constants.hpp>
#include <core/Clump.hpp>
#include <pkg/dem/KnKsPBLaw.hpp>

#include <core/Body.hpp>
#include <core/Interaction.hpp>
#include <core/Scene.hpp>
#include <pkg/common/Aabb.hpp>
#include <pkg/common/ForceResetter.hpp>
#include <pkg/common/InsertionSortCollider.hpp>
#include <pkg/common/InteractionLoop.hpp>
#include <pkg/dem/GlobalStiffnessTimeStepper.hpp>
#include <pkg/dem/NewtonIntegrator.hpp>
//#include <pkg/dem/Shop.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/limits.hpp>
#include <boost/numeric/conversion/bounds.hpp>
// random
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include "BlockGen.hpp"
#include <lib/pyutil/gil.hpp>
#include <pkg/dem/Ig2_PB_PB_ScGeom.hpp>
#include <pkg/dem/PotentialBlock2AABB.hpp>

/* IpOpt */
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>


// Note about using high precisision Real:
// for start create a file for coinor compatibility lib/compatibility/CoinorCompatibility.hpp,
// and wrap there all the calls to external coinor library.
// This would be similar to wrapper/converter how lib/compatibility/LapackCompatibility.cpp is done
// Then try to replace these functions with native Eigen high-precision Real routines of linear programming.
//
// Also all memset(…) calls should be replaced with std::fill(…) of std::vector like it is done in pkg/dem/Ig2_PP_PP_ScGeom.cpp

namespace yade { // Cannot have #include directive inside.

using math::max;
using math::min; // using inside .cpp file is ok.

CREATE_LOGGER(BlockGen);
YADE_PLUGIN((BlockGen));

//using namespace boost;
//using namespace std;

BlockGen::~BlockGen() { }
//std::ofstream BlockGen::output("BlockGenFindExtreme.txt", fstream::trunc); // it was always creating files "BlkGen" "BlockGenFindExtreme.txt", but they are not used in the code, so I commented this out, Janek
bool BlockGen::generate(string& /*message*/)
{
	if (saveBlockGenData == true) { //at first, open an empty file
		if (!outputFile.empty()) {
			myfile.open(outputFile.c_str(), std::ofstream::out);
			myfile << "Block Generation data" << endl;
			myfile.close();
		}
	}

	scene = shared_ptr<Scene>(new Scene);
	shared_ptr<Body> body;
	//	positionRootBody(scene);
	createActors(scene);

	/* Create domain: start with one big block */
	vector<Block> blk;
	Vector3r      firstBlockCentre(0, 0, 0);
	firstBlockCentre.x() = 0.0; //0.5*(boundarySizeXmax + boundarySizeXmin);
	firstBlockCentre.y() = 0.0; //0.5*(boundarySizeYmax + boundarySizeYmin);
	firstBlockCentre.z() = 0.0; //0.5*(boundarySizeZmax + boundarySizeZmin);
	blk.push_back(Block(firstBlockCentre, kForPP, rForPP, RForPP));

	Real xmin = fabs(firstBlockCentre.x() - boundarySizeXmin);
	Real xmax = fabs(-firstBlockCentre.x() + boundarySizeXmax);
	Real ymin = fabs(firstBlockCentre.y() - boundarySizeYmin);
	Real ymax = fabs(-firstBlockCentre.y() + boundarySizeYmax);
	Real zmin = fabs(firstBlockCentre.z() - boundarySizeZmin);
	Real zmax = fabs(-firstBlockCentre.z() + boundarySizeZmax);

	// Geometric definition of the initial cuboidal block
	blk[0].a = { 1.0, -1.0, 0.0, 0.0, 0.0, 0.0 };
	blk[0].b = { 0.0, 0.0, 1.0, -1.0, 0.0, 0.0 };
	blk[0].c = { 0.0, 0.0, 0.0, 0.0, 1.0, -1.0 };
	blk[0].d = { xmax - rForPP, xmin - rForPP, ymax - rForPP, ymin - rForPP, zmax - rForPP, zmin - rForPP };
	//	blk[0].d = { xmax, xmin, ymax, ymin, zmax, zmin };

	for (unsigned int i = 0; i < blk[0].a.size(); i++) {
		blk[0].redundant.push_back(false);
		blk[0].JRC.push_back(15.0);
		blk[0].JCS.push_back(pow(10, 6));
		blk[0].sigmaC.push_back(pow(10, 6));
		blk[0].phi_r.push_back(40.0);
		blk[0].phi_b.push_back(40.0);
		blk[0].asperity.push_back(5.0);
		blk[0].cohesion.push_back(0.0);
		blk[0].tension.push_back(0.0);
		blk[0].isBoundaryPlane.push_back(false);
		blk[0].lambda0.push_back(0.0);
		blk[0].heatCapacity.push_back(0.0);
		blk[0].hwater.push_back(-1.0);
		blk[0].intactRock.push_back(false);
		blk[0].jointType.push_back(0);
	}

	/* List of Discontinuities */
	vector<Discontinuity> joint;

	/* Read boundary size */
	Real boundarySize
	        = max(max(fabs(boundarySizeXmax - boundarySizeXmin), fabs(boundarySizeYmax - boundarySizeYmin)), fabs(boundarySizeZmax - boundarySizeZmin));
	Real dip = 0.0, dipdir = 0.0;

	/*Python input for discontinuities */
	for (unsigned int i = 0; i < joint_a.size(); i++) {
		joint.push_back(Discontinuity(globalOrigin));
		joint[i].a = joint_a[i];
		joint[i].b = joint_b[i];
		joint[i].c = joint_c[i];
		joint[i].d = joint_d[i];
		//std::cout<<"joint i: "<<i<<", a: "<<joint_a[i]<<", b: "<<joint_b[i]<<", c: "<<joint_c[i]<<", d: "<<joint_d[i]<<endl;
	}

	//TODO: Need to output a message if the expected file does not exist in all the the below branches: if(boundaries) - if(sliceBoundaries) - if(persistentPlanes) - if(jointProbabilistic) - if(slopeFace) - if(opening)

	if (boundaries) {
		/* Read csv file for info on discontinuities */
		const char*   filenameChar = filenameBoundaries.c_str();
		std::ifstream file(filenameChar); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
		string        value;
		/* skip first line */
		getline(file, value);
		int      count = 0;
		Vector3r jointCentre(0, 0, 0);
		//		const Real PI = std::atan(1.0)*4;
		/* int abdCount=0; */ Vector3r planeNormal(0, 0, 0);
		int                            jointNo = 0; //Real persistenceA=0; Real persistenceB=0; Real spacing = 0;
		while (file.good()) {
			count++;
			getline(file, value, ';'); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
			//std::cout<<"count: "<<count<<", value: "<<value<<endl;
			const char* valueChar = value.c_str();
			Real        valueReal = atof(valueChar);
			if (count == 1)
				dip = valueReal;
			if (count == 2) {
				dipdir       = valueReal;
				Real dipdirN = 0.0;
				Real dipN    = 90.0 - dip;

				if (dipdir > 180.0) {
					dipdirN = dipdir - 180.0;
				} else {
					dipdirN = dipdir + 180.0;
				}
				Real dipRad    = dipN / 180.0 * Mathr::PI;
				Real dipdirRad = dipdirN / 180.0 * Mathr::PI;
				Real a         = cos(dipdirRad) * cos(dipRad);
				Real b         = sin(dipdirRad) * cos(dipRad);
				Real c         = sin(dipRad);
				Real l         = sqrt(a * a + b * b + c * c);
				joint.push_back(Discontinuity(globalOrigin));
				int i       = joint.size() - 1;
				jointNo     = i;
				joint[i].a  = a / l;
				joint[i].b  = b / l;
				joint[i].c  = c / l;
				joint[i].d  = 0.0;
				planeNormal = Vector3r(a / l, b / l, c / l);
				//std::cout<<"planeNormal: "<<planeNormal<<endl;
			}
			if (count == 3)
				jointCentre.x() = valueReal;
			if (count == 4)
				jointCentre.y() = valueReal;
			if (count == 5) {
				jointCentre.z()           = valueReal;
				joint[jointNo].centre     = jointCentre;
				joint[jointNo].isBoundary = true;
			}
			if (count == 6) {
				joint[jointNo].phi_b = valueReal;
			}
			if (count == 7) {
				joint[jointNo].phi_r = valueReal;
			}
			if (count == 8) {
				joint[jointNo].JRC = valueReal;
			}
			if (count == 9) {
				joint[jointNo].JCS = valueReal;
			}
			if (count == 10) {
				joint[jointNo].asperity = valueReal;
			}
			if (count == 11) {
				joint[jointNo].sigmaC = valueReal;
			}
			if (count == 12) {
				joint[jointNo].cohesion = valueReal;
			}
			if (count == 13) {
				joint[jointNo].tension = valueReal;
			}
			if (count == 14) {
				joint[jointNo].lambda0 = valueReal;
			}
			if (count == 15) {
				joint[jointNo].heatCapacity = valueReal;
			}
			if (count == 16) {
				joint[jointNo].hwater       = valueReal;
				joint[jointNo].throughGoing = true;
				count                       = 0;
			}
		}
	}


	if (sliceBoundaries) {
		/* Read csv file for info on discontinuities */
		const char*   filenameChar = filenameSliceBoundaries.c_str();
		std::ifstream file(filenameChar); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
		string        value;
		/* skip first line */
		getline(file, value);
		int      count = 0;
		Vector3r jointCentre(0, 0, 0);
		//		const Real PI = std::atan(1.0)*4;
		Vector3r planeNormal(0, 0, 0);
		int      jointNo = 0;
		while (file.good()) {
			count++;
			getline(file, value, ';'); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
			//std::cout<<"count: "<<count<<", value: "<<value<<endl;
			const char* valueChar = value.c_str();
			Real        valueReal = atof(valueChar);
			if (count == 1)
				dip = valueReal;
			if (count == 2) {
				dipdir       = valueReal;
				Real dipdirN = 0.0;
				Real dipN    = 90.0 - dip;
				if (dipdir > 180.0) {
					dipdirN = dipdir - 180.0;
				} else {
					dipdirN = dipdir + 180.0;
				}
				Real dipRad    = dipN / 180.0 * Mathr::PI;
				Real dipdirRad = dipdirN / 180.0 * Mathr::PI;
				Real a         = cos(dipdirRad) * cos(dipRad);
				Real b         = sin(dipdirRad) * cos(dipRad);
				Real c         = sin(dipRad);
				Real l         = sqrt(a * a + b * b + c * c);
				joint.push_back(Discontinuity(globalOrigin));
				int i       = joint.size() - 1;
				jointNo     = i;
				joint[i].a  = a / l;
				joint[i].b  = b / l;
				joint[i].c  = c / l;
				joint[i].d  = 0.0;
				planeNormal = Vector3r(a / l, b / l, c / l);
				//std::cout<<"planeNormal: "<<planeNormal<<endl;
			}
			if (count == 3)
				jointCentre.x() = valueReal;
			if (count == 4)
				jointCentre.y() = valueReal;
			if (count == 5) {
				jointCentre.z()                = valueReal;
				joint[jointNo].centre          = jointCentre;
				joint[jointNo].sliceBoundaries = true;
			}
			if (count == 6) {
				joint[jointNo].phi_b = valueReal;
			}
			if (count == 7) {
				joint[jointNo].phi_r = valueReal;
			}
			if (count == 8) {
				joint[jointNo].JRC = valueReal;
			}
			if (count == 9) {
				joint[jointNo].JCS = valueReal;
			}
			if (count == 10) {
				joint[jointNo].asperity = valueReal;
			}
			if (count == 11) {
				joint[jointNo].sigmaC = valueReal;
			}
			if (count == 12) {
				joint[jointNo].cohesion = valueReal;
			}
			if (count == 13) {
				joint[jointNo].tension      = valueReal;
				joint[jointNo].throughGoing = true;
				count                       = 0;
			}
		}
	}


	if (persistentPlanes) {
		/* Read csv file for info on discontinuities */
		const char*   filenameChar = filenamePersistentPlanes.c_str();
		std::ifstream file(filenameChar); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
		string        value;
		/* skip first line */
		getline(file, value);
		int count = 0;
		//		const Real PI = std::atan(1.0)*4;
		/* int boundaryNo = 0; int boundaryCount = 0; int abdCount=0; */ Vector3r planeNormal(
		        0, 0, 0); /* int jointNo = 0; Real persistenceA=0; Real persistenceB=0; */
		Real spacing = 0;
		Real a = 0, b = 0, c = 0 /* d */, l = 0.0;
		while (file.good()) {
			count++;
			getline(file, value, ';'); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
			//std::cout<<"count: "<<count<<", value: "<<value<<endl;
			const char* valueChar = value.c_str();
			Real        valueReal = atof(valueChar);

			Real phi_b = 0, phi_r = 0, JRC = 0, JCS = 0, asperity = 0, sigmaC = 0, cohesion = 0, tension = 0.0;
			if (count == 1)
				dip = valueReal;
			if (count == 2)
				dipdir = valueReal;
			if (count == 3) {
				spacing = valueReal;
				//std::cout<<"dip: "<<dip<<", dipdir: "<<dipdir<<", spacing: "<<spacing<<endl;
				Real dipdirN = 0.0;
				Real dipN    = 90.0 - dip;
				if (dipdir > 180.0) {
					dipdirN = dipdir - 180.0;
				} else {
					dipdirN = dipdir + 180.0;
				}
				Real dipRad    = dipN / 180.0 * Mathr::PI;
				Real dipdirRad = dipdirN / 180.0 * Mathr::PI;
				a              = cos(dipdirRad) * cos(dipRad);
				b              = sin(dipdirRad) * cos(dipRad);
				c              = sin(dipRad);
				l              = sqrt(a * a + b * b + c * c);
				planeNormal    = Vector3r(a / l, b / l, c / l); //save for later
				                                                //std::cout<<"planeNormal1: "<<planeNormal<<endl;
			}
			if (count == 4) {
				phi_b = valueReal;
			}
			if (count == 5) {
				phi_r = valueReal;
			}
			if (count == 6) {
				JRC = valueReal;
			}
			if (count == 7) {
				JCS = valueReal;
			}
			if (count == 8) {
				asperity = valueReal;
			}
			if (count == 9) {
				sigmaC = valueReal;
			}
			if (count == 10) {
				cohesion = valueReal;
			}
			if (count == 11) {
				tension = valueReal;
				joint.push_back(Discontinuity(firstBlockCentre));
				{
					int i             = joint.size() - 1;
					joint[i].a        = a / l;
					joint[i].b        = b / l;
					joint[i].c        = c / l;
					joint[i].d        = 0.0;
					joint[i].phi_b    = phi_b;
					joint[i].phi_r    = phi_r;
					joint[i].JRC      = JRC;
					joint[i].JCS      = JCS;
					joint[i].asperity = asperity;
					joint[i].sigmaC   = sigmaC;
					joint[i].cohesion = cohesion;
					joint[i].tension  = tension;
				}
				planeNormal = Vector3r(a / l, b / l, c / l); //save for later
				//std::cout<<"planeNormal: "<<planeNormal<<endl;
				int number = int(math::round(boundarySize / spacing));
				for (int j = 1; j <= number / 2.0 /* -1 */; j++) {
					joint.push_back(Discontinuity(firstBlockCentre));
					int i             = joint.size() - 1;
					joint[i].a        = a / l;
					joint[i].b        = b / l;
					joint[i].c        = c / l;
					joint[i].d        = spacing * static_cast<Real>(j);
					joint[i].phi_b    = phi_b;
					joint[i].phi_r    = phi_r;
					joint[i].JRC      = JRC;
					joint[i].JCS      = JCS;
					joint[i].asperity = asperity;
					joint[i].sigmaC   = sigmaC;
					joint[i].cohesion = cohesion;
					joint[i].tension  = tension;
					joint.push_back(Discontinuity(firstBlockCentre));
					i                 = joint.size() - 1;
					joint[i].a        = -a / l;
					joint[i].b        = -b / l;
					joint[i].c        = -c / l;
					joint[i].d        = spacing * static_cast<Real>(j);
					joint[i].phi_b    = phi_b;
					joint[i].phi_r    = phi_r;
					joint[i].JRC      = JRC;
					joint[i].JCS      = JCS;
					joint[i].asperity = asperity;
					joint[i].sigmaC   = sigmaC;
					joint[i].cohesion = cohesion;
					joint[i].tension  = tension;
				}
				count = 0;
			}
		}
	}


	if (jointProbabilistic) {
		/* Read csv file for info on discontinuities */
		const char*   filenameChar = filenameProbabilistic.c_str();
		std::ifstream file(filenameChar); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
		string        value;
		/* skip first line */
		getline(file, value);
		int      count   = 0; //int linecount = 0;
		Real     dip2    = 0.0;
		Real     dipdir2 = 0.0;
		Vector3r jointCentre(0, 0, 0);
		//		const Real PI = std::atan(1.0)*4;
		/* int boundaryNo = 0; int boundaryCount = 0; int abdCount=0; */ Vector3r planeNormal(0, 0, 0);
		int                          jointNo = 0; //Real persistenceA=0; Real persistenceB=0; Real spacing = 0;
		boost::normal_distribution<> nd(0.0, 1.0);
		boost::variate_generator<boost::mt19937, boost::normal_distribution<>> generator(boost::mt19937(time(0)), nd);
		while (file.good()) {
			//std::cout<<"reading file, count"<<count<<endl;
			count++;
			getline(file, value, ';'); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
			//std::cout<<"count: "<<count<<", value: "<<value<<endl;
			const char* valueChar = value.c_str();
			Real        valueReal = atof(valueChar);
			if (count == 1) {
				dip2 = valueReal;
			}
			if (count == 2) {
				dipdir2      = valueReal;
				Real dipdirN = 0.0;
				Real dipN    = 90.0 - dip2;
				if (dipdir2 > 180.0) {
					dipdirN = dipdir2 - 180.0;
				} else {
					dipdirN = dipdir2 + 180.0;
				}
				if (probabilisticOrientation == true && dip2 < 120.0 && dip2 > 60.0) {
					Real perturb = gen_normal_3(generator);
					dipN         = dipN + perturb;

					if (saveBlockGenData == true) { //output to file
						if (!outputFile.empty()) {
							myfile.open(outputFile.c_str(), std::ofstream::app);
							myfile << "perturb: " << perturb << endl;
							myfile.close();
						}
					} else { //output to terminal
						std::cout << "perturb: " << perturb << endl;
					}
				}
				Real dipRad    = dipN / 180.0 * Mathr::PI;
				Real dipdirRad = dipdirN / 180.0 * Mathr::PI;
				Real a         = cos(dipdirRad) * cos(dipRad);
				Real b         = sin(dipdirRad) * cos(dipRad);
				Real c         = sin(dipRad);
				Real l         = sqrt(a * a + b * b + c * c);
				joint.push_back(Discontinuity(globalOrigin));
				int i       = joint.size() - 1;
				jointNo     = i;
				joint[i].a  = a / l;
				joint[i].b  = b / l;
				joint[i].c  = c / l;
				joint[i].d  = 0.0;
				planeNormal = Vector3r(a / l, b / l, c / l);
			}
			if (count == 3) {
				jointCentre.x() = valueReal;
			}
			if (count == 4) {
				jointCentre.y() = valueReal;
			}
			if (count == 5) {
				jointCentre.z()       = valueReal;
				joint[jointNo].centre = jointCentre;
			}
			if (count == 6) {
				Real     strike    = dipdir2 - 90.0;
				Real     strikeRad = strike / 180.0 * Mathr::PI;
				Vector3r Nstrike   = Vector3r(cos(strikeRad), sin(strikeRad), 0.0);
				Vector3r Ndip      = planeNormal.cross(Nstrike);
				Ndip.normalize();
				Matrix3r Qp = Matrix3r::Zero();
				Qp(0, 0)    = Nstrike.x();
				Qp(0, 1)    = Ndip.x();
				Qp(0, 2)    = planeNormal.x();
				Qp(1, 0)    = Nstrike.y();
				Qp(1, 1)    = Ndip.y();
				Qp(1, 2)    = planeNormal.y();
				Qp(2, 0)    = Nstrike.z();
				Qp(2, 1)    = Ndip.z();
				Qp(2, 2)    = planeNormal.z();

				Vector3r rotatedPersistence(1.0, 0.0, 0);
#if 0
				Vector3r oriNormal(0,0,1); //normal vector of x-y plane
				Vector3r crossProd = oriNormal.cross(planeNormal);
				Quaternionr Qp;
				Qp.w() = sqrt(oriNormal.squaredNorm() * planeNormal.squaredNorm()) + oriNormal.dot(planeNormal);
				Qp.x() = crossProd.x(); Qp.y() = crossProd.y();  Qp.z() = crossProd.z();
				if(crossProd.norm() < pow(10,-7)){Qp = Quaternionr::Identity();}
				Qp.normalize();
#endif
				rotatedPersistence = Qp * rotatedPersistence;

				//std::cout<<"planeNormal : "<<planeNormal<<", rotatedPersistence: "<<rotatedPersistence<<endl;
				joint[jointNo].persistence_a.push_back(rotatedPersistence.x());
				joint[jointNo].persistence_a.push_back(-rotatedPersistence.x());
				joint[jointNo].persistence_b.push_back(rotatedPersistence.y());
				joint[jointNo].persistence_b.push_back(-rotatedPersistence.y());
				joint[jointNo].persistence_c.push_back(rotatedPersistence.z());
				joint[jointNo].persistence_c.push_back(-rotatedPersistence.z());
				joint[jointNo].persistence_d.push_back(valueReal);
				joint[jointNo].persistence_d.push_back(valueReal);
			}
			if (count == 7) {
				Real     strike    = dipdir2 - 90.0;
				Real     strikeRad = strike / 180.0 * Mathr::PI;
				Vector3r Nstrike   = Vector3r(cos(strikeRad), sin(strikeRad), 0.0);

				if (saveBlockGenData == true) { //output to file
					if (!outputFile.empty()) {
						myfile.open(outputFile.c_str(), std::ofstream::app);
						myfile << "Nstrike: " << Nstrike << endl;
						myfile.close();
					}
				} else { //output to terminal
					std::cout << "Nstrike: " << Nstrike << endl;
				}

				Vector3r Ndip = planeNormal.cross(Nstrike);
				Ndip.normalize();

				if (saveBlockGenData == true) { //output to file
					if (!outputFile.empty()) {
						myfile.open(outputFile.c_str(), std::ofstream::app);
						myfile << "Ndip: " << Ndip << endl;
						myfile.close();
					}
				} else { //output to terminal
					std::cout << "Ndip: " << Ndip << endl;
				}

				Matrix3r Qp = Matrix3r::Zero();
				Qp(0, 0)    = Nstrike.x();
				Qp(0, 1)    = Ndip.x();
				Qp(0, 2)    = planeNormal.x();
				Qp(1, 0)    = Nstrike.y();
				Qp(1, 1)    = Ndip.y();
				Qp(1, 2)    = planeNormal.y();
				Qp(2, 0)    = Nstrike.z();
				Qp(2, 1)    = Ndip.z();
				Qp(2, 2)    = planeNormal.z();

				Vector3r rotatedPersistence(0.0, 1.0, 0.0);
#if 0
				Vector3r oriNormal(0,0,1); //normal vector of x-y plane
				Vector3r crossProd = oriNormal.cross(planeNormal);
				Quaternionr Qp;
				Qp.w() = sqrt(oriNormal.squaredNorm() * planeNormal.squaredNorm()) + oriNormal.dot(planeNormal);
				Qp.x() = crossProd.x(); Qp.y() = crossProd.y();  Qp.z() = crossProd.z();
				if(crossProd.norm() < pow(10,-7)){Qp = Quaternionr::Identity();}
				Qp.normalize();
#endif
				rotatedPersistence = Qp * rotatedPersistence;
				//std::cout<<"planeNormal : "<<planeNormal<<", rotatedPersistence: "<<rotatedPersistence<<endl;
				joint[jointNo].persistence_a.push_back(rotatedPersistence.x());
				joint[jointNo].persistence_a.push_back(-rotatedPersistence.x());
				joint[jointNo].persistence_b.push_back(rotatedPersistence.y());
				joint[jointNo].persistence_b.push_back(-rotatedPersistence.y());
				joint[jointNo].persistence_c.push_back(rotatedPersistence.z());
				joint[jointNo].persistence_c.push_back(-rotatedPersistence.z());
				joint[jointNo].persistence_d.push_back(valueReal);
				joint[jointNo].persistence_d.push_back(valueReal);
			}
			if (count == 8) {
				joint[jointNo].phi_b = valueReal;
			}
			if (count == 9) {
				joint[jointNo].phi_r = valueReal;
			}
			if (count == 10) {
				joint[jointNo].JRC = valueReal;
			}
			if (count == 11) {
				joint[jointNo].JCS = valueReal;
			}
			if (count == 12) {
				joint[jointNo].asperity = valueReal;
			}
			if (count == 13) {
				joint[jointNo].sigmaC = valueReal;
			}
			if (count == 14) {
				joint[jointNo].cohesion = valueReal;
			}
			if (count == 15) {
				joint[jointNo].tension = valueReal;
			}
			if (count == 16) {
				joint[jointNo].jointType = static_cast<int>(valueReal);
			}
			if (count == 17) {
				count = 0; /* to include comments */
				if (intactRockDegradation == true) {
					joint[jointNo].intactRock = true;
				}
			}
		}
	}


	if (slopeFace) {
		/* Read csv file for info on discontinuities */
		const char*   filenameChar = filenameSlopeFace.c_str();
		std::ifstream file(filenameChar); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
		string        value;
		/* skip first line */
		getline(file, value);
		int      count = 0;
		Vector3r jointCentre(0, 0, 0);
		//		const Real PI = std::atan(1.0)*4;
		Vector3r planeNormal(0, 0, 0);
		int      jointNo = 0;
		while (file.good()) {
			count++;
			getline(file, value, ';'); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
			//std::cout<<"count: "<<count<<", value: "<<value<<endl;
			const char* valueChar = value.c_str();
			Real        valueReal = atof(valueChar);
			if (count == 1)
				dip = valueReal;
			if (count == 2) {
				dipdir       = valueReal;
				Real dipdirN = 0.0;
				Real dipN    = 90.0 - dip;
				if (dipdir > 180.0) {
					dipdirN = dipdir - 180.0;
				} else {
					dipdirN = dipdir + 180.0;
				}
				Real dipRad    = dipN / 180.0 * Mathr::PI;
				Real dipdirRad = dipdirN / 180.0 * Mathr::PI;
				Real a         = cos(dipdirRad) * cos(dipRad);
				Real b         = sin(dipdirRad) * cos(dipRad);
				Real c         = sin(dipRad);
				Real l         = sqrt(a * a + b * b + c * c);
				joint.push_back(Discontinuity(globalOrigin));
				int i       = joint.size() - 1;
				jointNo     = i;
				joint[i].a  = a / l;
				joint[i].b  = b / l;
				joint[i].c  = c / l;
				joint[i].d  = 0.0;
				planeNormal = Vector3r(a / l, b / l, c / l);
				//std::cout<<"planeNormal: "<<planeNormal<<endl;
			}
			if (count == 3)
				jointCentre.x() = valueReal;
			if (count == 4)
				jointCentre.y() = valueReal;
			if (count == 5) {
				jointCentre.z()       = valueReal;
				joint[jointNo].centre = jointCentre;
			}
			if (count == 6 or count == 7) {
				//				if(count == 6) {
				Vector3r rotatedPersistence(1, 0, 0); //if(count == 6)
				                                      //				} else {
				if (count == 7) {
					rotatedPersistence = Vector3r(0, 1, 0); //if(count == 7)
				}
				Vector3r    oriNormal(0, 0, 1); //normal vector of x-y plane
				Vector3r    crossProd = oriNormal.cross(planeNormal);
				Quaternionr Qp;
				Qp.w() = sqrt(oriNormal.squaredNorm() * planeNormal.squaredNorm()) + oriNormal.dot(planeNormal);
				Qp.x() = crossProd.x();
				Qp.y() = crossProd.y();
				Qp.z() = crossProd.z();
				if (crossProd.norm() < pow(10, -7)) {
					Qp = Quaternionr::Identity();
				}
				Qp.normalize();
				rotatedPersistence = Qp * rotatedPersistence;
				//std::cout<<"planeNormal : "<<planeNormal<<", rotatedPersistence: "<<rotatedPersistence<<endl;
				joint[jointNo].persistence_a.push_back(rotatedPersistence.x());
				joint[jointNo].persistence_a.push_back(-rotatedPersistence.x());
				joint[jointNo].persistence_b.push_back(rotatedPersistence.y());
				joint[jointNo].persistence_b.push_back(-rotatedPersistence.y());
				joint[jointNo].persistence_c.push_back(rotatedPersistence.z());
				joint[jointNo].persistence_c.push_back(-rotatedPersistence.z());
				joint[jointNo].persistence_d.push_back(valueReal);
				joint[jointNo].persistence_d.push_back(valueReal);
			}
			if (count == 8) {
				joint[jointNo].phi_b = valueReal;
			}
			if (count == 9) {
				joint[jointNo].phi_r = valueReal;
			}
			if (count == 10) {
				joint[jointNo].JRC = valueReal;
			}
			if (count == 11) {
				joint[jointNo].JCS = valueReal;
			}
			if (count == 12) {
				joint[jointNo].asperity = valueReal;
			}
			if (count == 13) {
				joint[jointNo].sigmaC = valueReal;
			}
			if (count == 14) {
				joint[jointNo].cohesion = valueReal;
			}
			if (count == 15) {
				joint[jointNo].tension            = valueReal;
				joint[jointNo].throughGoing       = true;
				joint[jointNo].constructionJoints = true;
				count                             = 0;
			}
			//if(intactRockDegradation==true){
			//	joint[jointNo].intactRock = true;
			//}
		}
	}


	if (opening) {
		/* Read csv file for info on discontinuities */
		const char*   filenameChar = filenameOpening.c_str();
		std::ifstream file(filenameChar); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
		string        value;
		/* skip first line */
		getline(file, value);
		int      count = 0; //int linecount = 0;
		Vector3r jointCentre(0, 0, 0);
		//const Real PI = std::atan(1.0)*4;
		/* int boundaryNo = 0; int boundaryCount = 0; int abdCount=0; */ Vector3r planeNormal(0, 0, 0);
		int jointNo = 0; //Real persistenceA=0; Real persistenceB=0; Real spacing = 0;
		while (file.good()) {
			count++;
			getline(file, value, ';'); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
			//std::cout<<"count: "<<count<<", value: "<<value<<endl;
			const char* valueChar = value.c_str();
			Real        valueReal = atof(valueChar);
			if (count == 1) {
				planeNormal.x() = valueReal;
			}
			if (count == 2) {
				planeNormal.y() = valueReal;
			}
			if (count == 3) {
				planeNormal.z() = valueReal;
				planeNormal.normalize();
			}
			if (count == 4) {
				joint.push_back(Discontinuity(jointCentre));
				int i      = joint.size() - 1;
				jointNo    = i; //std::cout<<"planeNormal: "<<planeNormal<<endl;
				joint[i].a = planeNormal.x();
				joint[i].b = planeNormal.y();
				joint[i].c = planeNormal.z();
				joint[i].d = valueReal;
			}
			if (count == 5 or count == 6) {
				//				if(count == 5){
				Vector3r rotatedPersistence(1, 0, 0); //if(count == 5)
				                                      //				} else {
				if (count == 6) {
					rotatedPersistence = Vector3r(0, 1, 0); //if(count == 6)
				}
				Vector3r    oriNormal(0, 0, 1); //normal vector of x-y plane
				Vector3r    crossProd = oriNormal.cross(planeNormal);
				Quaternionr Qp;
				Qp.w() = sqrt(oriNormal.squaredNorm() * planeNormal.squaredNorm()) + oriNormal.dot(planeNormal);
				Qp.x() = crossProd.x();
				Qp.y() = crossProd.y();
				Qp.z() = crossProd.z();
				if (crossProd.norm() < pow(10, -7)) {
					Qp = Quaternionr::Identity();
				}
				Qp.normalize();
				rotatedPersistence = Qp * rotatedPersistence;
				rotatedPersistence.normalize();
				//std::cout<<"planeNormal : "<<planeNormal<<", rotatedPersistence: "<<rotatedPersistence<<", check orthogonal: "<<planeNormal.dot(rotatedPersistence)<<endl;
				joint[jointNo].persistence_a.push_back(rotatedPersistence.x());
				joint[jointNo].persistence_a.push_back(-rotatedPersistence.x());
				joint[jointNo].persistence_b.push_back(rotatedPersistence.y());
				joint[jointNo].persistence_b.push_back(-rotatedPersistence.y());
				joint[jointNo].persistence_c.push_back(rotatedPersistence.z());
				joint[jointNo].persistence_c.push_back(-rotatedPersistence.z());
				joint[jointNo].persistence_d.push_back(valueReal);
				joint[jointNo].persistence_d.push_back(valueReal);
			}
			if (count == 7) {
				joint[jointNo].phi_b = valueReal;
			}
			if (count == 8) {
				joint[jointNo].phi_r = valueReal;
			}
			if (count == 9) {
				joint[jointNo].JRC = valueReal;
			}
			if (count == 10) {
				joint[jointNo].JCS = valueReal;
			}
			if (count == 11) {
				joint[jointNo].asperity = valueReal;
			}
			if (count == 12) {
				joint[jointNo].sigmaC = valueReal;
			}
			if (count == 13) {
				joint[jointNo].cohesion = valueReal;
			}
			if (count == 14) {
				joint[jointNo].tension            = valueReal;
				joint[jointNo].throughGoing       = true;
				joint[jointNo].constructionJoints = true;
				count                             = 0;
			}
		}
	}

	if (saveBlockGenData == true) { //output to file
		if (!outputFile.empty()) {
			myfile.open(outputFile.c_str(), std::ofstream::app);
			myfile << "joint_a.size(): " << joint_a.size() << ", joint size: " << joint.size() << endl;
			myfile.close();
		}
	} else { //output to terminal
		std::cout << "joint_a.size(): " << joint_a.size() << ", joint size: " << joint.size() << endl;
	}


	/* Perform contact detection between discontinuity and blocks.  For any blocks that touch the plane, split it into half */
	for (unsigned int j = 0; j < joint.size(); j++) {
		if (saveBlockGenData == true) { //output to file
			if (!outputFile.empty()) {
				myfile.open(outputFile.c_str(), std::ofstream::app);
				myfile << "Slicing progress.... joint no: " << j + 1 << "/" << joint.size() << endl;
				myfile.close();
			}
		} else { //output to terminal
			std::cout << "Slicing progress.... joint no: " << j + 1 << "/" << joint.size() << endl;
		}

		int blkOriCount = blk.size();
		for (int i = 0; i < blkOriCount; i++) {
			int   subMemberSize = blk[i].subMembers.size();
			int   subMemberIter = 0;
			Block presentBlock  = Block(blk[i].centre, kForPP, rForPP, RForPP);
			do {
				if (subMemberSize == 0) {
					presentBlock = blk[i];
				} else {
					presentBlock = blk[i].subMembers[subMemberIter];
				}
				/* Fast contact detection */
				Vector3r vertexFrJoint(0, 0, 0);
				if (joint[j].throughGoing == false) {
					Real twoRadiiDist
					        = 1.2 * (presentBlock.R + math::max(fabs(joint[j].persistence_d[0]), fabs(joint[j].persistence_d[2])));
					Real centroidDist = (joint[j].centre - presentBlock.tempCentre).norm();
					if (centroidDist > twoRadiiDist) {
						subMemberIter++;
						continue;
					}
					/* std::cout<<"centroidDist: "<<centroidDist<<", twoRadiiDist: "<<twoRadiiDist<<", presentBlock.R: "<<presentBlock.R<<", presentBlock.tempCentre: "<<presentBlock.tempCentre<<", joint.centre: "<<joint[j].centre<<endl; */
				}

				if (contactDetectionLPCLPglobal(joint[j], presentBlock, vertexFrJoint) && presentBlock.tooSmall == false) {
					if (saveBlockGenData == true) { //output to file
						if (!outputFile.empty()) {
							myfile.open(outputFile.c_str(), std::ofstream::app);
							myfile << "joint[" << j << "] sliced successfully" << endl;
							myfile.close();
						}
					} else { //output to terminal
						std::cout << "joint[" << j << "] sliced successfully" << endl;
					}

					if (presentBlock.isBoundary == true && joint[j].sliceBoundaries == false) {
						continue;
					}
					if (presentBlock.isBoundary == false && joint[j].sliceBoundaries == true) {
						continue;
					}

					//Real fns = evaluateFNoSphere(presentBlock, vertexFrJoint);
					//std::cout<<"fns: "<<fns<<endl;
					/* Split the block into two */ //shrink d

					/* Make a copy of parent block */
					int   blkNo = blk.size(); /*size = currentCount+1 */
					Block blkA  = Block(presentBlock.centre, kForPP, rForPP, RForPP);
					Block blkB  = Block(presentBlock.centre, kForPP, rForPP, RForPP);
					blkA        = presentBlock;
					blkB        = presentBlock;
					int planeNo = presentBlock.a.size();
					/* Add plane from joint to new block */
					blkB.a.push_back(-1.0 * joint[j].a);
					blkB.b.push_back(-1.0 * joint[j].b);
					blkB.c.push_back(-1.0 * joint[j].c);
					Real newNegD
					        = (1.0 * joint[j].a * (blkB.centre.x() - joint[j].centre.x())
					           + 1.0 * joint[j].b * (blkB.centre.y() - joint[j].centre.y())
					           + 1.0 * joint[j].c * (blkB.centre.z() - joint[j].centre.z()) - joint[j].d);
					blkB.d.push_back(newNegD - shrinkFactor * blkB.r); /*shrink */
					blkB.redundant.push_back(false);
					blkB.JRC.push_back(joint[j].JRC);
					blkB.JCS.push_back(joint[j].JCS);
					blkB.sigmaC.push_back(joint[j].sigmaC);
					blkB.phi_r.push_back(joint[j].phi_r);
					blkB.phi_b.push_back(joint[j].phi_b);
					blkB.asperity.push_back(joint[j].asperity);
					blkB.cohesion.push_back(joint[j].cohesion);
					blkB.tension.push_back(joint[j].tension);
					blkB.isBoundaryPlane.push_back(joint[j].isBoundary);
					blkB.lambda0.push_back(joint[j].lambda0);
					blkB.heatCapacity.push_back(joint[j].heatCapacity);
					blkB.hwater.push_back(joint[j].hwater);
					blkB.intactRock.push_back(joint[j].intactRock);
					blkB.jointType.push_back(joint[j].jointType);
					if (joint[j].isBoundary == true) {
						for (int k = 0; k < planeNo; k++) { /*planeNo is previous size before adding the new plane */
							blkB.isBoundaryPlane[k] = false;
						}
					}

					/* Add plane from joint to parent block */
					blkA.a.push_back(joint[j].a);
					blkA.b.push_back(joint[j].b);
					blkA.c.push_back(joint[j].c);
					Real newPosD = -1.0
					        * (joint[j].a * (blkA.centre.x() - joint[j].centre.x()) + joint[j].b * (blkA.centre.y() - joint[j].centre.y())
					           + joint[j].c * (blkA.centre.z() - joint[j].centre.z()) - joint[j].d);
					blkA.d.push_back(newPosD - shrinkFactor * blkA.r); /*shrink */
					blkA.redundant.push_back(false);
					blkA.JRC.push_back(joint[j].JRC);
					blkA.JCS.push_back(joint[j].JCS);
					blkA.sigmaC.push_back(joint[j].sigmaC);
					blkA.phi_r.push_back(joint[j].phi_r);
					blkA.phi_b.push_back(joint[j].phi_b);
					blkA.asperity.push_back(joint[j].asperity);
					blkA.cohesion.push_back(joint[j].cohesion);
					blkA.tension.push_back(joint[j].tension);
					blkA.isBoundaryPlane.push_back(joint[j].isBoundary);
					blkA.lambda0.push_back(joint[j].lambda0);
					blkA.heatCapacity.push_back(joint[j].heatCapacity);
					blkA.hwater.push_back(joint[j].hwater);
					blkA.intactRock.push_back(joint[j].intactRock);
					blkA.jointType.push_back(joint[j].jointType);
					if (joint[j].isBoundary == true) {
						for (int k = 0; k < planeNo; k++) { /*planeNo is previous size before adding the new plane */
							blkA.isBoundaryPlane[k] = false;
						}
					}
					//std::cout<<"detected"<<endl;
#if 0
					/* Adjust block centroid, after every discontinuity is introduced */
					Vector3r startingPt = Vector3r(0,0,0); //centroid
					bool converge = startingPointFeasibility(blkA, startingPt);
					Vector3r centroid = blkA.centre+startingPt;
					Vector3r prevCentre = blkA.centre;
					blkA.centre = centroid;
					Real maxd=0.0;
					for(unsigned int h=0; h<blkA.a.size(); h++){
						blkA.d[h]= -1.0*(blkA.a[h]*(centroid.x()-prevCentre.x()) + blkA.b[h]*(centroid.y()-prevCentre.y()) + blkA.c[h]*(centroid.z()-prevCentre.z())  - blkA.d[h]);
						if(blkA.d[h] < 0.0){
							std::cout<<"blkA.d[h]: "<<blkA.d[h]<<endl;
						}
						if(fabs(blkA.d[h] )> maxd){maxd= fabs(blkA.d[h]);}
					}
					if(converge== false){
						blkA.tooSmall = true;
						bool inside = checkCentroid(blkA,blkA.centre);
						std::cout<<"blki inside: "<<inside<<endl;
					}
					blkA.R = 1.2*maxd;

					/* Adjust block centroid, after every discontinuity is introduced */
					startingPt = Vector3r(0,0,0); maxd = 0.0;
					converge = startingPointFeasibility(blkB, startingPt);
					prevCentre = blkB.centre;
					centroid = blkB.centre+startingPt;
					blkB.centre = centroid;
					for(unsigned int h=0; h<blkB.a.size(); h++){
						blkB.d[h]= -1.0*(blkB.a[h]*(centroid.x()-prevCentre.x()) + blkB.b[h]*(centroid.y()-prevCentre.y()) + blkB.c[h]*(centroid.z()-prevCentre.z())  - blkB.d[h]);
						if(blkB.d[h] < 0.0){
							std::cout<<"blkB.d[h]: "<<blkB.d[h]<<endl;
						}
						if(fabs(blkB.d[h] )> maxd){maxd=fabs( blkB.d[h]);}
					}
					if(converge== false){
						blkB.tooSmall = true;
						bool inside = checkCentroid(blkB,blkB.centre);
						std::cout<<"blkNo inside: "<<inside<<endl;
					}
					blkB.R = 1.2*maxd;

					/* Identify redundant planes */
					for(int k=0; k<planeNo; k++){//The last plane is the new discontinuity.  It is definitely part of the new blocks.
						Discontinuity plane=Discontinuity(blkA.centre);
						plane.a=blkA.a[k];
						plane.b=blkA.b[k];
						plane.c=blkA.c[k];
						plane.d=blkA.d[k];
						Vector3r falseVertex (0.0,0.0,0.0);

						if (!checkRedundancyLP(plane, blkA,falseVertex) ){
							blkA.redundant[k] = true;
						}
						Discontinuity plane2=Discontinuity(blkB.centre);
						plane2.a=blkB.a[k];
						plane2.b=blkB.b[k];
						plane2.c=blkB.c[k];
						plane2.d=blkB.d[k];
						if (!checkRedundancyLP(plane2, blkB,falseVertex) ){
							blkB.redundant[k] = true;
						}
					}

					/* Remove redundant planes */
					unsigned int no = 0;
					while(no<blkA.a.size()){ //The last plane is the new discontinuity.  It is definitely part of the new blocks.
						//std::cout<<"no: "<<no<<", a[no]: "<<blkA.a[no]<<" redundant: "<<blkA.redundant[no]<<endl;
						if(blkA.redundant[no] == true){
							blkA.a.erase(blkA.a.begin()+no);
							blkA.b.erase(blkA.b.begin()+no);
							blkA.c.erase(blkA.c.begin()+no);
							blkA.d.erase(blkA.d.begin()+no);
							blkA.redundant.erase(blkA.redundant.begin()+no);
							blkA.JRC.erase(blkA.JRC.begin()+no);
							blkA.JCS.erase(blkA.JCS.begin()+no);
							blkA.sigmaC.erase(blkA.sigmaC.begin()+no);
							blkA.phi_r.erase(blkA.phi_r.begin()+no);
							blkA.phi_b.erase(blkA.phi_b.begin()+no);
							blkA.asperity.erase(blkA.asperity.begin()+no);
							blkA.cohesion.erase(blkA.cohesion.begin()+no);
							blkA.tension.erase(blkA.tension.begin()+no);
							blkA.isBoundaryPlane.erase(blkA.isBoundaryPlane.begin()+no);
							blkA.lambda0.erase(blkA.lambda0.begin()+no);
							blkA.heatCapacity.erase(blkA.heatCapacity.begin()+no);
							blkA.hwater.erase(blkA.hwater.begin()+no);
							blkA.intactRock.erase(blkA.intactRock.begin()+no);
							blkA.jointType.erase(blkA.jointType.begin()+no);
							no = 0;
						}else{
							no = no+1;
						}
					}

					no = 0;
					while(no<blkB.a.size()){ //The 1st plane is the new discontinuity.  It is definitely part of the new blocks.
						//std::cout<<"no: "<<no<<", a[no]: "<<blkB.a[no]<<" redundant: "<<blkB.redundant[no]<<endl;
						if(blkB.redundant[no] == true){
							blkB.a.erase(blkB.a.begin()+no);
							blkB.b.erase(blkB.b.begin()+no);
							blkB.c.erase(blkB.c.begin()+no);
							blkB.d.erase(blkB.d.begin()+no);
							blkB.redundant.erase(blkB.redundant.begin()+no);
							blkB.JRC.erase(blkB.JRC.begin()+no);
							blkB.JCS.erase(blkB.JCS.begin()+no);
							blkB.sigmaC.erase(blkB.sigmaC.begin()+no);
							blkB.phi_r.erase(blkB.phi_r.begin()+no);
							blkB.phi_b.erase(blkB.phi_b.begin()+no);
							blkB.asperity.erase(blkB.asperity.begin()+no);
							blkB.cohesion.erase(blkB.cohesion.begin()+no);
							blkB.tension.erase(blkB.tension.begin()+no);
							blkB.isBoundaryPlane.erase(blkB.isBoundaryPlane.begin()+no);
							blkB.lambda0.erase(blkB.lambda0.begin()+no);
							blkB.heatCapacity.erase(blkB.heatCapacity.begin()+no);
							blkB.hwater.erase(blkB.hwater.begin()+no);
							blkB.intactRock.erase(blkB.intactRock.begin()+no);
							blkB.jointType.erase(blkB.jointType.begin()+no);
							no = 0;
						}else{
							no = no+1;
						}
					}
#endif

					/* Identify boundary blocks */
					if (joint[j].isBoundary == true) {
						Vector3r startingPt = Vector3r(0, 0, 0); //centroid
						                                         //						//bool converge=startingPointFeasibility(blkA, startingPt);
						Real radius = inscribedSphereCLP(
						        blkA,
						        startingPt,
						        twoDimension); //Although we don't need the "radius" of the inscribed sphere here, we invoke "inscribedSphereCLP" in order to pass by reference the calculated value for "startingPt"
						bool converge = true;
						if (radius < 0.0) {
							converge = false;
						}
						if (converge == false) {
							radius += 0;
						}
						Vector3r centroidA = blkA.centre + startingPt;
						Real     fA        = joint[j].a * (centroidA.x() - joint[j].centre.x())
						        + joint[j].b * (centroidA.y() - joint[j].centre.y())
						        + joint[j].c * (centroidA.z() - joint[j].centre.z()) - joint[j].d;
						if (fA > 0.0) {
							blkA.isBoundary = true;
						} else {
							blkB.isBoundary = true;
						}
					}

					Real     RblkA = 0.0;
					Vector3r tempCentreA(0, 0, 0);
					Real     RblkB = 0.0;
					Vector3r tempCentreB(0, 0, 0);
					/* Prune blocks that are too small or too elongated */
					bool tooSmall = false;
					if (joint[j].throughGoing == false) {
						//Real conditioningFactor = 1.0;
						Real minX = 0.0;
						Real minY = 0.0;
						Real minZ = 0.0;
						Real maxX = 0.0;
						Real maxY = 0.0;
						Real maxZ = 0.0;

						Discontinuity plane = Discontinuity(Vector3r(0, 0, 0));
						plane.a             = directionA.x(); //1.0;
						plane.b             = directionA.y(); //0.0;
						plane.c             = directionA.z(); //0.0;
						plane.d             = 1.2 * boundarySizeXmax;
						Vector3r falseVertex(0.0, 0.0, 0.0);
						if (contactBoundaryLPCLP(plane, blkA, falseVertex)) {
							minX = directionA.dot(falseVertex); //falseVertex.x();
						} else {
							tooSmall = true;
						}
						tempCentreA = tempCentreA + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = directionB.x(); //0.0;
						plane.b = directionB.y(); //1.0;
						plane.c = directionB.z(); //0.0;
						plane.d = 1.2 * boundarySizeYmax;
						if (contactBoundaryLPCLP(plane, blkA, falseVertex)) {
							minY = directionB.dot(falseVertex); //falseVertex.y();
						} else {
							tooSmall = true;
						}
						tempCentreA = tempCentreA + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = directionC.x(); //0.0;
						plane.b = directionC.y(); //0.0;
						plane.c = directionC.z(); //1.0;
						plane.d = 1.2 * boundarySizeZmax;
						if (contactBoundaryLPCLP(plane, blkA, falseVertex)) {
							minZ = directionC.dot(falseVertex); //falseVertex.z();
						} else {
							tooSmall = true;
						}
						tempCentreA = tempCentreA + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = -directionA.x(); //-1.0;
						plane.b = -directionA.y(); //0.0;
						plane.c = -directionA.z(); //0.0;
						plane.d = 1.2 * boundarySizeXmin;
						if (contactBoundaryLPCLP(plane, blkA, falseVertex)) {
							maxX = directionA.dot(falseVertex); //falseVertex.x();
						} else {
							tooSmall = true;
						}
						tempCentreA = tempCentreA + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = -directionB.x(); //0.0;
						plane.b = -directionB.y(); //-1.0;
						plane.c = -directionB.z(); //0.0;
						plane.d = 1.2 * boundarySizeYmin;
						if (contactBoundaryLPCLP(plane, blkA, falseVertex)) {
							maxY = directionB.dot(falseVertex); //falseVertex.y();
						} else {
							tooSmall = true;
						}
						tempCentreA = tempCentreA + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = -directionC.x(); // 0.0;
						plane.b = -directionC.y(); //0.0;
						plane.c = -directionC.z(); //-1.0;
						plane.d = 1.2 * boundarySizeZmin;
						if (contactBoundaryLPCLP(plane, blkA, falseVertex)) {
							maxZ = directionC.dot(falseVertex); //falseVertex.z();
						} else {
							tooSmall = true;
						}
						tempCentreA = tempCentreA + falseVertex;

						Real maxXoverall = fabs(maxX - minX);
						Real maxYoverall = fabs(maxY - minY);
						Real maxZoverall = fabs(maxZ - minZ);
						tempCentreA      = 1.0 / 6.0 * tempCentreA;
						Vector3r tempA(0, 0, 0);
						Real     chebyshevRa = inscribedSphereCLP(blkA, tempA, twoDimension);
						tempCentreA          = tempA; //std::cout<<"chebyshevRa: "<<chebyshevRa<<endl;
						RblkA                = sqrt(maxXoverall * maxXoverall + maxYoverall * maxYoverall + maxZoverall * maxZoverall);

						if (saveBlockGenData == true) { //output to file
							if (!outputFile.empty()) {
								myfile.open(outputFile.c_str(), std::ofstream::app);
								myfile << "blockA, minX: " << minX << ", maxX: " << maxX << ", minY: " << minY
								       << ", maxY: " << maxY << ", minZ: " << minZ << ", maxZ: " << maxZ << ", RblkA: " << RblkA
								       << ", tempCentreA: " << tempCentreA << endl;
								myfile.close();
							}
						} else { //output to terminal
							std::cout << "blockA, minX: " << minX << ", maxX: " << maxX << ", minY: " << minY << ", maxY: " << maxY
							          << ", minZ: " << minZ << ", maxZ: " << maxZ << ", RblkA: " << RblkA
							          << ", tempCentreA: " << tempCentreA << endl;
						}

						if (2.0 * chebyshevRa < minSize) { //(maxXoverall < minSize || maxZoverall < minSize){
							tooSmall = true;
						}

						if (maxXoverall / (2.0 * chebyshevRa) > maxRatio || maxYoverall / (2.0 * chebyshevRa) > maxRatio
						    || maxZoverall / (2.0 * chebyshevRa) > maxRatio) {
							tooSmall = true;
						}

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = directionA.x(); //1.0;
						plane.b = directionA.y(); //0.0;
						plane.c = directionA.z(); //0.0;
						plane.d = 1.2 * boundarySizeXmax;
						if (contactBoundaryLPCLP(plane, blkB, falseVertex)) {
							minX = directionA.dot(falseVertex); //falseVertex.x();
						} else {
							tooSmall = true;
						}
						tempCentreB = tempCentreB + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = directionB.x(); //0.0;
						plane.b = directionB.y(); //1.0;
						plane.c = directionB.z(); //0.0;
						plane.d = 1.2 * boundarySizeYmax;
						if (contactBoundaryLPCLP(plane, blkB, falseVertex)) {
							minY = directionB.dot(falseVertex); //falseVertex.y();
						} else {
							tooSmall = true;
						}
						tempCentreB = tempCentreB + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = directionC.x(); //0.0;
						plane.b = directionC.y(); //0.0;
						plane.c = directionC.z(); //1.0;
						plane.d = 1.2 * boundarySizeZmax;
						if (contactBoundaryLPCLP(plane, blkB, falseVertex)) {
							minZ = directionC.dot(falseVertex); //falseVertex.z();
						} else {
							tooSmall = true;
						}
						tempCentreB = tempCentreB + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = -directionA.x(); //-1.0;
						plane.b = -directionA.y(); //0.0;
						plane.c = -directionA.z(); //0.0;
						plane.d = 1.2 * boundarySizeXmin;
						if (contactBoundaryLPCLP(plane, blkB, falseVertex)) {
							maxX = directionA.dot(falseVertex); //falseVertex.x();
						} else {
							tooSmall = true;
						}
						tempCentreB = tempCentreB + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = -directionB.x(); //0.0;
						plane.b = -directionB.y(); //-1.0;
						plane.c = -directionB.z(); //0.0;
						plane.d = 1.2 * boundarySizeYmin;
						if (contactBoundaryLPCLP(plane, blkB, falseVertex)) {
							maxY = directionB.dot(falseVertex); //falseVertex.y();
						} else {
							tooSmall = true;
						}
						tempCentreB = tempCentreB + falseVertex;

						plane   = Discontinuity(Vector3r(0, 0, 0));
						plane.a = -directionC.x(); //0.0;
						plane.b = -directionC.y(); //0.0;
						plane.c = -directionC.z(); //-1.0;
						plane.d = 1.2 * boundarySizeZmin;
						if (contactBoundaryLPCLP(plane, blkB, falseVertex)) {
							maxZ = directionC.dot(falseVertex); //falseVertex.z();
						} else {
							tooSmall = true;
						}
						tempCentreB = tempCentreB + falseVertex;

						maxXoverall = fabs(maxX - minX);
						maxYoverall = fabs(maxY - minY);
						maxZoverall = fabs(maxZ - minZ);
						tempCentreB = 1.0 / 6.0 * tempCentreB;
						Vector3r tempB(0, 0, 0);
						Real     chebyshevRb = inscribedSphereCLP(blkB, tempB, twoDimension);
						tempCentreB          = tempB; //std::cout<<"chebyshevRb: "<<chebyshevRb<<endl;
						RblkB                = sqrt(maxXoverall * maxXoverall + maxYoverall * maxYoverall + maxZoverall * maxZoverall);

						if (saveBlockGenData == true) { //output to file
							if (!outputFile.empty()) {
								myfile.open(outputFile.c_str(), std::ofstream::app);
								myfile << "blockB, minX: " << minX << ", maxX: " << maxX << ", minY: " << minY
								       << ", maxY: " << maxY << ", minZ: " << minZ << ", maxZ: " << maxZ << ", RblkB: " << RblkB
								       << ", tempCentreB: " << tempCentreB << endl;
								myfile.close();
							}
						} else { //output to terminal
							std::cout << "blockB, minX: " << minX << ", maxX: " << maxX << ", minY: " << minY << ", maxY: " << maxY
							          << ", minZ: " << minZ << ", maxZ: " << maxZ << ", RblkB: " << RblkB
							          << ", tempCentreB: " << tempCentreB << endl;
						}

						if (chebyshevRa < 0.0 || chebyshevRb < 0.0) {
							if (saveBlockGenData == true) { //output to file
								if (!outputFile.empty()) {
									myfile.open(outputFile.c_str(), std::ofstream::app);
									myfile << "1 chebyshevRa: " << chebyshevRa << ", chebyshevRb: " << chebyshevRb << endl;
									myfile.close();
								}
							} else { //output to terminal
								std::cout << "1 chebyshevRa: " << chebyshevRa << ", chebyshevRb: " << chebyshevRb << endl;
							}

							tooSmall = true;
						}
						if (2.0 * chebyshevRb < minSize) { //(maxXoverall < minSize || maxZoverall < minSize){

							if (saveBlockGenData == true) { //output to file
								if (!outputFile.empty()) {
									myfile.open(outputFile.c_str(), std::ofstream::app);
									myfile << "2 chebyshevRa: " << chebyshevRa << ", chebyshevRb: " << chebyshevRb << endl;
									myfile.close();
								}
							} else { //output to terminal
								std::cout << "2 chebyshevRa: " << chebyshevRa << ", chebyshevRb: " << chebyshevRb << endl;
							}
							tooSmall = true;
						}

						if (maxXoverall / (2.0 * chebyshevRb) > maxRatio || maxYoverall / (2.0 * chebyshevRb) > maxRatio
						    || maxZoverall / (2.0 * chebyshevRb) > maxRatio) {
							tooSmall = true;
						}
					}

					if (tooSmall == false) {
#if 0
						/* Identify redundant planes */
						for(int k=0; k<planeNo; k++){//The last plane is the new discontinuity.  It is definitely part of the new blocks.
							Discontinuity plane=Discontinuity(blkA.centre);
							plane.a=blkA.a[k];
							plane.b=blkA.b[k];
							plane.c=blkA.c[k];
							plane.d=blkA.d[k];
							Vector3r falseVertex (0.0,0.0,0.0);

							if (!checkRedundancyLP(plane, blkA,falseVertex) ){
								blkA.redundant[k] = true;
							}
							Discontinuity plane2=Discontinuity(blkB.centre);
							plane2.a=blkB.a[k];
							plane2.b=blkB.b[k];
							plane2.c=blkB.c[k];
							plane2.d=blkB.d[k];
							if (!checkRedundancyLP(plane2, blkB,falseVertex) ){
								blkB.redundant[k] = true;
							}
						}

						/* Remove redundant planes */
						unsigned int no = 0;
						while(no<blkA.a.size()){ //The last plane is the new discontinuity.  It is definitely part of the new blocks.
							//std::cout<<"no: "<<no<<", a[no]: "<<blkA.a[no]<<" redundant: "<<blkA.redundant[no]<<endl;
							if(blkA.redundant[no] == true){
								blkA.a.erase(blkA.a.begin()+no);
								blkA.b.erase(blkA.b.begin()+no);
								blkA.c.erase(blkA.c.begin()+no);
								blkA.d.erase(blkA.d.begin()+no);
								blkA.redundant.erase(blkA.redundant.begin()+no);
								blkA.JRC.erase(blkA.JRC.begin()+no);
								blkA.JCS.erase(blkA.JCS.begin()+no);
								blkA.sigmaC.erase(blkA.sigmaC.begin()+no);
								blkA.phi_r.erase(blkA.phi_r.begin()+no);
								blkA.phi_b.erase(blkA.phi_b.begin()+no);
								blkA.asperity.erase(blkA.asperity.begin()+no);
								blkA.cohesion.erase(blkA.cohesion.begin()+no);
								blkA.tension.erase(blkA.tension.begin()+no);
								blkA.isBoundaryPlane.erase(blkA.isBoundaryPlane.begin()+no);
								blkA.lambda0.erase(blkA.lambda0.begin()+no);
								blkA.heatCapacity.erase(blkA.heatCapacity.begin()+no);
								blkA.hwater.erase(blkA.hwater.begin()+no);
								blkA.intactRock.erase(blkA.intactRock.begin()+no);
								blkA.jointType.erase(blkA.jointType.begin()+no);
								no = 0;
							}else{
								no = no+1;
							}
						}
						no = 0;
						while(no<blkB.a.size()){ //The 1st plane is the new discontinuity.  It is definitely part of the new blocks.
							//std::cout<<"no: "<<no<<", a[no]: "<<blkB.a[no]<<" redundant: "<<blkB.redundant[no]<<endl;
							if(blkB.redundant[no] == true){
								blkB.a.erase(blkB.a.begin()+no);
								blkB.b.erase(blkB.b.begin()+no);
								blkB.c.erase(blkB.c.begin()+no);
								blkB.d.erase(blkB.d.begin()+no);
								blkB.redundant.erase(blkB.redundant.begin()+no);
								blkB.JRC.erase(blkB.JRC.begin()+no);
								blkB.JCS.erase(blkB.JCS.begin()+no);
								blkB.sigmaC.erase(blkB.sigmaC.begin()+no);
								blkB.phi_r.erase(blkB.phi_r.begin()+no);
								blkB.phi_b.erase(blkB.phi_b.begin()+no);
								blkB.asperity.erase(blkB.asperity.begin()+no);
								blkB.cohesion.erase(blkB.cohesion.begin()+no);
								blkB.tension.erase(blkB.tension.begin()+no);
								blkB.isBoundaryPlane.erase(blkB.isBoundaryPlane.begin()+no);
								blkB.lambda0.erase(blkB.lambda0.begin()+no);
								blkB.heatCapacity.erase(blkB.heatCapacity.begin()+no);
								blkB.hwater.erase(blkB.hwater.begin()+no);
								blkB.intactRock.erase(blkB.intactRock.begin()+no);
								blkB.jointType.erase(blkB.jointType.begin()+no);
								no = 0;
							}else{
								no = no+1;
							}
						}
#endif
						if (joint[j].constructionJoints == true) {
							if (subMemberSize > 0) {
								blk[i].subMembers.push_back(Block(blkB.centre, kForPP, rForPP, RForPP));
								int subMemberCount                           = blk[i].subMembers.size() - 1;
								blk[i].subMembers[subMemberIter]             = blkA;
								blk[i].subMembers[subMemberCount]            = blkB;
								blk[i].subMembers[subMemberIter].R           = RblkA;
								blk[i].subMembers[subMemberCount].R          = RblkB;
								blk[i].subMembers[subMemberIter].tempCentre  = tempCentreA;
								blk[i].subMembers[subMemberCount].tempCentre = tempCentreB;
							} else {
								blk[i].subMembers.push_back(Block(blkA.centre, kForPP, rForPP, RForPP));
								blk[i].subMembers.push_back(Block(blkB.centre, kForPP, rForPP, RForPP));
								int subMemberCount                               = blk[i].subMembers.size() - 2;
								blk[i].subMembers[subMemberCount]                = blkA;
								blk[i].subMembers[subMemberCount + 1]            = blkB;
								blk[i].subMembers[subMemberCount].R              = RblkA;
								blk[i].subMembers[subMemberCount + 1].R          = RblkB;
								blk[i].subMembers[subMemberCount].tempCentre     = tempCentreA;
								blk[i].subMembers[subMemberCount + 1].tempCentre = tempCentreB;
							}
						} else {
							blk.push_back(Block(blkB.centre, kForPP, rForPP, RForPP));
							blk[blkNo]            = blkB;
							blk[i]                = blkA;
							blk[blkNo].R          = RblkB;
							blk[i].R              = RblkA;
							blk[blkNo].tempCentre = tempCentreB;
							blk[i].tempCentre     = tempCentreA;
						}
					}
				} /* outer if-braces for detected */
				subMemberIter++;

			} while (subMemberIter < subMemberSize);
		} /* outer loop for block */
	}         /* outer loop for joint */


	/* NEW Find temp centre and remove redundant planes */
	for (unsigned int i = 0; i < blk.size(); i++) {
		if (saveBlockGenData == true) { //output to file
			if (!outputFile.empty()) {
				myfile.open(outputFile.c_str(), std::ofstream::app);
				myfile << "Redundancy progress.... block no: " << i + 1 << "/" << blk.size() << endl;
				myfile.close();
			}
		} else { //output to terminal
			std::cout << "Redundancy progress.... block no: " << i + 1 << "/" << blk.size() << endl;
		}

		if (not blk[i].subMembers.empty()) {
			for (unsigned int j = 0; j < blk[i].subMembers.size(); j++) {
				/* Adjust block centroid, after every discontinuity is introduced */
				Vector3r startingPt = blk[i].subMembers[j].centre; //centroid

				//bool converge = startingPointFeasibility(blk[i].subMembers[j], startingPt);
				Real radius   = inscribedSphereCLP(blk[i].subMembers[j], startingPt, twoDimension);
				bool converge = true;
				if (radius < 0.0) {
					converge = false;
				}
				Vector3r centroid           = blk[i].subMembers[j].centre + startingPt;
				Vector3r prevCentre         = blk[i].subMembers[j].centre;
				blk[i].subMembers[j].centre = centroid;
				Real maxd                   = 0.0;
				for (unsigned int h = 0; h < blk[i].subMembers[j].a.size(); h++) {
					blk[i].subMembers[j].d[h] = -1.0
					        * (blk[i].subMembers[j].a[h] * (centroid.x() - prevCentre.x())
					           + blk[i].subMembers[j].b[h] * (centroid.y() - prevCentre.y())
					           + blk[i].subMembers[j].c[h] * (centroid.z() - prevCentre.z()) - blk[i].subMembers[j].d[h]);
					if (blk[i].subMembers[j].d[h] < 0.0) {
						if (saveBlockGenData == true) { //output to file
							if (!outputFile.empty()) {
								myfile.open(outputFile.c_str(), std::ofstream::app);
								myfile << "blk.d[h]: " << blk[i].subMembers[j].d[h] << endl;
								myfile.close();
							}
						} else { //output to terminal
							std::cout << "blk.d[h]: " << blk[i].subMembers[j].d[h] << endl;
						}
					}
					if (fabs(blk[i].subMembers[j].d[h]) > maxd) {
						maxd = fabs(blk[i].subMembers[j].d[h]);
					}
				}
				if (converge == false) {
					blk[i].subMembers[j].tooSmall = true;
					bool inside                   = checkCentroid(blk[i].subMembers[j], blk[i].subMembers[j].centre);

					if (saveBlockGenData == true) { //output to file
						if (!outputFile.empty()) {
							myfile.open(outputFile.c_str(), std::ofstream::app);
							myfile << "blki inside: " << inside << endl;
							myfile.close();
						}
					} else { //output to terminal
						std::cout << "blki inside: " << inside << endl;
					}
				}
				blk[i].subMembers[j].R = 1.2 * maxd;

				/* Identify redundant planes */
				for (unsigned int k = 0; k < blk[i].subMembers[j].a.size();
				     k++) { //The last plane is the new discontinuity.  It is definitely part of the new blocks.
					Discontinuity plane = Discontinuity(blk[i].subMembers[j].centre);
					plane.a             = blk[i].subMembers[j].a[k];
					plane.b             = blk[i].subMembers[j].b[k];
					plane.c             = blk[i].subMembers[j].c[k];
					plane.d             = blk[i].subMembers[j].d[k];
					Vector3r falseVertex(0.0, 0.0, 0.0);

					if (!checkRedundancyLPCLP(plane, blk[i].subMembers[j], falseVertex)) {
						blk[i].subMembers[j].redundant[k] = true;
					}
				}

				/* Remove redundant planes */
				unsigned int no = 0;
				while (no
				       < blk[i].subMembers[j].a.size()) { //The last plane is the new discontinuity.  It is definitely part of the new blocks.
					//std::cout<<"no: "<<no<<", a[no]: "<<blk.a[no]<<" redundant: "<<blk.redundant[no]<<endl;
					if (blk[i].subMembers[j].redundant[no] == true) {
						blk[i].subMembers[j].a.erase(blk[i].subMembers[j].a.begin() + no);
						blk[i].subMembers[j].b.erase(blk[i].subMembers[j].b.begin() + no);
						blk[i].subMembers[j].c.erase(blk[i].subMembers[j].c.begin() + no);
						blk[i].subMembers[j].d.erase(blk[i].subMembers[j].d.begin() + no);
						blk[i].subMembers[j].redundant.erase(blk[i].subMembers[j].redundant.begin() + no);
						blk[i].subMembers[j].JRC.erase(blk[i].subMembers[j].JRC.begin() + no);
						blk[i].subMembers[j].JCS.erase(blk[i].subMembers[j].JCS.begin() + no);
						blk[i].subMembers[j].sigmaC.erase(blk[i].subMembers[j].sigmaC.begin() + no);
						blk[i].subMembers[j].phi_r.erase(blk[i].subMembers[j].phi_r.begin() + no);
						blk[i].subMembers[j].phi_b.erase(blk[i].subMembers[j].phi_b.begin() + no);
						blk[i].subMembers[j].asperity.erase(blk[i].subMembers[j].asperity.begin() + no);
						blk[i].subMembers[j].cohesion.erase(blk[i].subMembers[j].cohesion.begin() + no);
						blk[i].subMembers[j].tension.erase(blk[i].subMembers[j].tension.begin() + no);
						blk[i].subMembers[j].isBoundaryPlane.erase(blk[i].subMembers[j].isBoundaryPlane.begin() + no);
						blk[i].subMembers[j].lambda0.erase(blk[i].subMembers[j].lambda0.begin() + no);
						blk[i].subMembers[j].heatCapacity.erase(blk[i].subMembers[j].heatCapacity.begin() + no);
						blk[i].subMembers[j].hwater.erase(blk[i].subMembers[j].hwater.begin() + no);
						blk[i].subMembers[j].intactRock.erase(blk[i].subMembers[j].intactRock.begin() + no);
						blk[i].subMembers[j].jointType.erase(blk[i].subMembers[j].jointType.begin() + no);
						no = 0;
					} else {
						no = no + 1;
					}
				}
			}
		} else {
			/* Adjust block centroid, after every discontinuity is introduced */
			Vector3r startingPt = blk[i].centre; //centroid

			//bool converge = startingPointFeasibility(blk[i], startingPt);
			Real radius   = inscribedSphereCLP(blk[i], startingPt, twoDimension);
			bool converge = true;
			if (radius < 0.0) {
				converge = false;
			}
			Vector3r centroid   = blk[i].centre + startingPt;
			Vector3r prevCentre = blk[i].centre;
			blk[i].centre       = centroid;
			Real maxd           = 0.0;
			for (unsigned int h = 0; h < blk[i].a.size(); h++) {
				blk[i].d[h] = -1.0
				        * (blk[i].a[h] * (centroid.x() - prevCentre.x()) + blk[i].b[h] * (centroid.y() - prevCentre.y())
				           + blk[i].c[h] * (centroid.z() - prevCentre.z()) - blk[i].d[h]);
				if (blk[i].d[h] < 0.0) {
					if (saveBlockGenData == true) { //output to file
						if (!outputFile.empty()) {
							myfile.open(outputFile.c_str(), std::ofstream::app);
							myfile << "blk.d[h]: " << blk[i].d[h] << endl;
							myfile.close();
						}
					} else { //output to terminal
						std::cout << "blk.d[h]: " << blk[i].d[h] << endl;
					}
				}
				if (fabs(blk[i].d[h]) > maxd) {
					maxd = fabs(blk[i].d[h]);
				}
			}
			if (converge == false) {
				blk[i].tooSmall = true;
				bool inside     = checkCentroid(blk[i], blk[i].centre);

				if (saveBlockGenData == true) { //output to file
					if (!outputFile.empty()) {
						myfile.open(outputFile.c_str(), std::ofstream::app);
						myfile << "blki inside: " << inside << endl;
						myfile.close();
					}
				} else { //output to terminal
					std::cout << "blki inside: " << inside << endl;
				}
			}
			blk[i].R = 1.2 * maxd;

			/* Identify redundant planes */
			for (unsigned int k = 0; k < blk[i].a.size();
			     k++) { //The last plane is the new discontinuity.  It is definitely part of the new blocks.
				Discontinuity plane = Discontinuity(blk[i].centre);
				plane.a             = blk[i].a[k];
				plane.b             = blk[i].b[k];
				plane.c             = blk[i].c[k];
				plane.d             = blk[i].d[k];
				Vector3r falseVertex(0.0, 0.0, 0.0);

				if (!checkRedundancyLPCLP(plane, blk[i], falseVertex)) {
					blk[i].redundant[k] = true;
				}
			}

			/* Remove redundant planes */
			unsigned int no = 0;
			while (no < blk[i].a.size()) { //The last plane is the new discontinuity.  It is definitely part of the new blocks.
				//std::cout<<"no: "<<no<<", a[no]: "<<blk.a[no]<<" redundant: "<<blk.redundant[no]<<endl;
				if (blk[i].redundant[no] == true) {
					blk[i].a.erase(blk[i].a.begin() + no);
					blk[i].b.erase(blk[i].b.begin() + no);
					blk[i].c.erase(blk[i].c.begin() + no);
					blk[i].d.erase(blk[i].d.begin() + no);
					blk[i].redundant.erase(blk[i].redundant.begin() + no);
					blk[i].JRC.erase(blk[i].JRC.begin() + no);
					blk[i].JCS.erase(blk[i].JCS.begin() + no);
					blk[i].sigmaC.erase(blk[i].sigmaC.begin() + no);
					blk[i].phi_r.erase(blk[i].phi_r.begin() + no);
					blk[i].phi_b.erase(blk[i].phi_b.begin() + no);
					blk[i].asperity.erase(blk[i].asperity.begin() + no);
					blk[i].cohesion.erase(blk[i].cohesion.begin() + no);
					blk[i].tension.erase(blk[i].tension.begin() + no);
					blk[i].isBoundaryPlane.erase(blk[i].isBoundaryPlane.begin() + no);
					blk[i].lambda0.erase(blk[i].lambda0.begin() + no);
					blk[i].heatCapacity.erase(blk[i].heatCapacity.begin() + no);
					blk[i].hwater.erase(blk[i].hwater.begin() + no);
					blk[i].intactRock.erase(blk[i].intactRock.begin() + no);
					blk[i].jointType.erase(blk[i].jointType.begin() + no);
					no = 0;
				} else {
					no = no + 1;
				}
			}
		}
	}

	int shapeIDcount = 0;
	/* Create blocks */
	for (unsigned int i = 0; i < blk.size(); i++) {
		if (saveBlockGenData == true) { //output to file
			if (!outputFile.empty()) {
				myfile.open(outputFile.c_str(), std::ofstream::app);
				myfile << "Generating progress.... block no: " << i + 1 << "/" << blk.size() << endl;
				myfile.close();
			}
		} else { //output to terminal
			std::cout << "Generating progress.... block no: " << i + 1 << "/" << blk.size() << endl;
		}

		if (not blk[i].subMembers.empty()) {
			//#if 0
			shared_ptr<Body>  clumpBody = shared_ptr<Body>(new Body());
			shared_ptr<Clump> clump     = shared_ptr<Clump>(new Clump());
			clumpBody->shape            = clump;
			clumpBody->setDynamic(true);
			clumpBody->setBounded(false);
			// Body::id_t clumpId=scene->bodies->insert(clumpBody);
			// std::cout<<"clumpId: "<<clumpId<<endl;
			//#endif

			vector<int> memberId;
			int         clumpMemberCount = 0;
			for (unsigned int j = 0; j < blk[i].subMembers.size(); j++) {
				if (createBlock(body, blk[i].subMembers[j], shapeIDcount /* j */)) {
					//scene->bodies->insert(body);
					Body::id_t lastId = (Body::id_t)scene->bodies->insert(body);
					/* new */ memberId.push_back(lastId);
					//Clump::add(clumpBody, /* body*/  Body::byId(lastId,scene) );
					//clump->ids.push_back(lastId);
					shapeIDcount++;
					clumpMemberCount++;
				}
				//std::cout<<"Generating progress.... sub-block no: "<<j+1<<"/"<<blk[i].subMembers.size()<<endl;
			}

			if (clumpMemberCount > 1) {
				//Body::id_t clumpId=scene->bodies->insert(clumpBody); //std::cout<<"ok 1"<<endl;
				for (unsigned int j = 0; j < memberId.size(); j++) {
					Clump::addNonSpherical(clumpBody, /* body*/ Body::byId(memberId[j], scene)); //std::cout<<"ok 2"<<endl;
					clump->ids.push_back(memberId[j]);                                           //std::cout<<"ok 3"<<endl;
				}
				Clump::updatePropertiesNonSpherical(clumpBody, /*intersecting*/ false, scene);
				//std::cout<<"ok 4"<<endl;
			}

		} else {
			if (createBlock(body, blk[i], shapeIDcount /* i */)) {
				scene->bodies->insert(body);
				shapeIDcount++;
			}
		}
	}
	scene->dt = defaultDt;
	//globalStiffnessTimeStepper->defaultDt=0.000001; //defaultDt;
	//scene->updateBound();
	//scene->bound->min = Vector3r(5,5,5);
	//scene->bound->max = -1.0*Vector3r(5,5,5);

	if (saveBlockGenData == true) { //output to file
		if (!outputFile.empty()) {
			myfile.open(outputFile.c_str(), std::ofstream::app);
			myfile << "The Block Generation is completed" << endl;
			myfile.close();
		}
	}
	std::cout << "The Block Generation is completed" << endl;

	return true;
}


//#if 0
bool BlockGen::createBlock(shared_ptr<Body>& body, struct Block block, int number)
{
	if (block.tooSmall == true) {
		return false;
	}

	block.r = block.r + initialOverlap;

	/* Set up the bounding box of the block */
	shared_ptr<Aabb> aabb(new Aabb);
	aabb->color = Vector3r(0, 1, 0);

	/* Set up the material */
	shared_ptr<FrictMat> mat(new FrictMat);
	mat->frictionAngle = frictionDeg * Mathr::PI / 180.0;

	/* Set up the particle shape : Pass values from block -> to pBlock */
	shared_ptr<PotentialBlock> pBlock(new PotentialBlock);

	pBlock->a = block.a;
	pBlock->b = block.b;
	pBlock->c = block.c;
	pBlock->d = block.d;

	// The attributes: JRC, JCS, sigmaC, asperity, isBoundaryPlane, lambda0, heatCapacity, hwater are no longer assigned to the shape class. They continue to be inhereted though to the children particles of the block generation process, through the "block" struct, in case their use is restored. @vsangelidakis
	//	pBlock->JRC=block.JRC;
	//	pBlock->JCS=block.JCS;
	//	pBlock->sigmaC=block.sigmaC;
	pBlock->phi_r = block.phi_r;
	pBlock->phi_b = block.phi_b;
	//	pBlock->asperity=block.asperity;
	pBlock->cohesion = block.cohesion;
	pBlock->tension  = block.tension;
	//	pBlock->isBoundaryPlane=block.isBoundaryPlane;
	//	pBlock->lambda0=block.lambda0;
	//	pBlock->heatCapacity=block.heatCapacity;
	//	pBlock->hwater=block.hwater;
	pBlock->intactRock = block.intactRock;
	pBlock->jointType  = block.jointType;

	pBlock->r          = block.r;
	pBlock->k          = block.k;
	pBlock->AabbMinMax = true;
	pBlock->R          = block.R;
	pBlock->id         = number;

	if (color[0] == -1 or color[1] == -1 or color[2] == -1) {
		pBlock->color = Vector3r(
		        math::unitRandom(), math::unitRandom(), math::unitRandom()); //math::max(math::max(maxXoverall, maxYoverall),maxZoverall) ; //
	} else {
		pBlock->color = color;
	}
	if (block.isBoundary == true) {
		pBlock->color = Vector3r(0, 0.4, 0);
	}

	pBlock->isBoundary = block.isBoundary;
	pBlock->vertices
	        .clear(); //Make sure the PotentialBlock is uninitialized, for PotentialBlock::postLoad to calculate vertices, volume, inertia, orientation, position, R
	pBlock->postLoad(*pBlock);

	//Update the plane coefficients block.a, block.b, block.c, block.d and the rest of the block-attributes, after the block is centred and rotated to its principal axes in the shape class and after the planes with less than 3 adjacent vertices have been removed : Pass values from pBlock -> to block
	block.a = pBlock->a;
	block.b = pBlock->b;
	block.c = pBlock->c;
	block.d = pBlock->d;
	//	block.JRC = pBlock->JRC;
	//	block.JCS = pBlock->JCS;
	//	block.sigmaC = pBlock->sigmaC;
	block.phi_r = pBlock->phi_r;
	block.phi_b = pBlock->phi_b;
	//	block.asperity = pBlock->asperity;
	block.cohesion = pBlock->cohesion;
	block.tension  = pBlock->tension;
	//	block.isBoundaryPlane = pBlock->isBoundaryPlane;
	//	block.lambda0 = pBlock->lambda0;
	//	block.heatCapacity = pBlock->heatCapacity;
	//	block.hwater = pBlock->hwater;
	block.intactRock = pBlock->intactRock;
	block.jointType  = pBlock->jointType;

	/* Set up body of the new block*/
	body              = shared_ptr<Body>(new Body);
	body->shape       = pBlock;
	body->state->mass = pBlock->volume * density;
	if (exactRotation == true) {
		body->state->inertia = pBlock->inertia * density * inertiaFactor;
	} else {
		Real maxInertia = math::max(
		        math::max(math::max(pBlock->inertia[0], pBlock->inertia[1]), pBlock->inertia[2]),
		        2.0 / 5.0 * body->state->mass / density * minSize * minSize);
		body->state->inertia = Vector3r(maxInertia, maxInertia, maxInertia) * density * inertiaFactor;
	}
	body->state->pos = block.centre + pBlock->position;
	body->state->ori = pBlock->orientation; //q.conjugate(); //pBlock.orientation
	if (block.isBoundary == true) {         //boundary blocks are considered non-dynamic bodies in BlockGen
		body->setDynamic(false);
	} else {
		body->setDynamic(true);
	}
	body->bound    = aabb;
	body->material = mat;
	body->setAspherical(true);
	//std::cout<<"BLOCKGEN pBlock->verticesCD.size() "<<pBlock->verticesCD.size()<<", pBlock->vertexStruct.size(): "<<pBlock->vertexStruct.size()<<endl;
	return true;
}
//#endif


void BlockGen::createActors(shared_ptr<Scene>& scene)
{
	shared_ptr<IGeomDispatcher>  interactionGeometryDispatcher(new IGeomDispatcher);
	shared_ptr<Ig2_PB_PB_ScGeom> cd(new Ig2_PB_PB_ScGeom);

	//	cd-> stepAngle=calAreaStep;
	cd->twoDimension   = twoDimension;
	cd->unitWidth2D    = unitWidth2D;
	cd->calContactArea = calContactArea;
	interactionGeometryDispatcher->add(cd);

	shared_ptr<IPhysDispatcher>                  interactionPhysicsDispatcher(new IPhysDispatcher);
	shared_ptr<Ip2_FrictMat_FrictMat_KnKsPBPhys> ss(new Ip2_FrictMat_FrictMat_KnKsPBPhys);
	ss->Knormal        = Kn;
	ss->Kshear         = Ks;
	ss->kn_i           = Kn;
	ss->ks_i           = Ks;
	ss->viscousDamping = viscousDamping;
	//		ss->useOverlapVol = useOverlapVol; // not used
	ss->useFaceProperties = useFaceProperties;
	//	ss->unitWidth2D = unitWidth2D;
	//	ss->calJointLength = calJointLength;
	//	ss->twoDimension = twoDimension;
	//		ss->brittleLength = brittleLength; // not used
	//		ss->u_peak = peakDisplacement; // not used
	//		ss->maxClosure = maxClosure; // not used
	interactionPhysicsDispatcher->add(ss);

	//shared_ptr<GravityEngine> gravityCondition(new GravityEngine);
	//gravityCondition->gravity = gravity;

	if (useGlobalStiffnessTimeStepper) {
		globalStiffnessTimeStepper                         = shared_ptr<GlobalStiffnessTimeStepper>(new GlobalStiffnessTimeStepper);
		globalStiffnessTimeStepper->timeStepUpdateInterval = timeStepUpdateInterval;
		globalStiffnessTimeStepper->defaultDt              = defaultDt;
	}

	scene->engines.clear();
	scene->engines.push_back(shared_ptr<Engine>(new ForceResetter));
	shared_ptr<InsertionSortCollider> collider(new InsertionSortCollider);
	collider->verletDist = 0.1 * minSize;
	collider->boundDispatcher->add(new PotentialBlock2AABB);

	scene->engines.push_back(collider);
	shared_ptr<InteractionLoop> ids(new InteractionLoop);
	ids->geomDispatcher = interactionGeometryDispatcher;
	ids->physDispatcher = interactionPhysicsDispatcher;
	ids->lawDispatcher  = shared_ptr<LawDispatcher>(new LawDispatcher);
	shared_ptr<Law2_SCG_KnKsPBPhys_KnKsPBLaw> see(new Law2_SCG_KnKsPBPhys_KnKsPBLaw);
	see->traceEnergy = traceEnergy;
	see->Talesnick   = Talesnick;
	see->neverErase  = neverErase;
	ids->lawDispatcher->add(see);
	scene->engines.push_back(ids);
	//scene->engines.push_back(globalStiffnessTimeStepper);
	//scene->engines.push_back(gravityCondition);
	shared_ptr<NewtonIntegrator> newton(new NewtonIntegrator);
	newton->damping = dampingMomentum;
	newton->gravity = gravity; //Vector3r(0,0,9.81);
	// newton->damping3DEC=damp3DEC;
	newton->exactAsphericalRot = exactRotation;
	scene->engines.push_back(newton);
	//scene->initializers.clear();
}


bool BlockGen::checkCentroid(struct Block block, Vector3r presentTrial)
{
	Real x           = presentTrial[0] - block.centre[0];
	Real y           = presentTrial[1] - block.centre[1];
	Real z           = presentTrial[2] - block.centre[2];
	int  planeNo     = block.a.size();
	bool allNegative = true;
	for (int i = 0; i < planeNo; i++) {
		Real plane = block.a[i] * x + block.b[i] * y + block.c[i] * z - block.d[i];
		if (plane < pow(10, -15)) {
			plane = 0.0;
		} else {
			allNegative = false;
		}
	}
	return allNegative;
}


bool BlockGen::contactDetectionLPCLPglobal(struct Discontinuity joint, struct Block block, Vector3r& touchingPt)
{
	if (block.tooSmall == true) {
		return false;
	}
	/* Parameters for particles A and B */
	int planeNoA       = block.a.size();
	int persistenceNoA = joint.persistence_a.size();
	/* Variables to keep things neat */
	int  NUMCON = 1 /* equality */ + planeNoA /*block inequality */ + persistenceNoA /*persistence inequalities */;
	int  NUMVAR = 3 /*3D */ + 1 /* s */;
	Real s      = 0.0;
	//bool converge = true;

	Real     xlocalA = 0;
	Real     ylocalA = 0;
	Real     zlocalA = 0;
	Real     xlocalB = 0;
	Real     ylocalB = 0;
	Real     zlocalB = 0;
	Vector3r localA(0, 0, 0);
	Vector3r xGlobalA(0, 0, 0);
	Vector3r localB(0, 0, 0);
	Vector3r xGlobalB(0, 0, 0);

	/* LINEAR CONSTRAINTS */
	ClpSimplex model2;

	model2.setOptimizationDirection(1);
	// Create space for 3 columns and 10000 rows
	int numberRows    = NUMCON;
	int numberColumns = NUMVAR;

	// Arrays will be set to default values
	model2.resize(0, numberColumns);

	// Columns - objective was packed
	model2.setObjectiveCoefficient(0, 0.0);
	model2.setObjectiveCoefficient(1, 0.0);
	model2.setObjectiveCoefficient(2, 0.0);
	model2.setObjectiveCoefficient(3, 1.0);
	model2.setColumnLower(0, -COIN_DBL_MAX);
	model2.setColumnLower(1, -COIN_DBL_MAX);
	model2.setColumnLower(2, -COIN_DBL_MAX);
	model2.setColumnLower(3, -COIN_DBL_MAX);
	model2.setColumnUpper(0, COIN_DBL_MAX);
	model2.setColumnUpper(1, COIN_DBL_MAX);
	model2.setColumnUpper(2, COIN_DBL_MAX);
	model2.setColumnUpper(3, COIN_DBL_MAX);

	Real rowLower
	        [numberRows]; //TODO: Check whether to replace C array with std::vector<> - that would be great, but first replace model2.addRow with something that can take argument std::vector<Real>
	Real rowUpper[numberRows]; //TODO: Check whether to replace C array with std::vector<>

	// Rows
	rowLower[0] = joint.a * joint.centre.x() + joint.b * joint.centre.y() + joint.c * joint.centre.z() + joint.d; //3 plane = 0
	for (int i = 0; i < planeNoA; i++) {
		rowLower[1 + i] = -COIN_DBL_MAX;
	};
	for (int i = 0; i < persistenceNoA; i++) {
		rowLower[1 + planeNoA + i] = -COIN_DBL_MAX;
	};

	rowUpper[0] = joint.a * joint.centre.x() + joint.b * joint.centre.y() + joint.c * joint.centre.z() + joint.d; //3 plane = 0
	for (int i = 0; i < planeNoA; i++) {
		rowUpper[1 + i] = block.d[i] + block.r;
	};
	for (int i = 0; i < persistenceNoA; i++) {
		rowUpper[1 + planeNoA + i] = joint.persistence_d[i]
		        + (joint.persistence_a[i] * joint.centre.x() + joint.persistence_b[i] * joint.centre.y()
		           + joint.persistence_c[i] * joint.centre.z()); //joint.persistence_d[i] ;
	};


	int  row1Index[] = { 0, 1, 2 };
	Real row1Value[] = { joint.a, joint.b, joint.c };
	model2.addRow(3, row1Index, row1Value, rowLower[0], rowUpper[0]);

	for (int i = 0; i < planeNoA; i++) {
		int  rowIndex[] = { 0, 1, 2, 3 };
		Real rowValue[] = { block.a[i], block.b[i], block.c[i], -1.0 };
		model2.addRow(4, rowIndex, rowValue, rowLower[1 + i], rowUpper[1 + i]);
	}

	for (int i = 0; i < persistenceNoA; i++) {
		int  rowIndex[] = { 0, 1, 2, 3 };
		Real rowValue[] = { joint.persistence_a[i], joint.persistence_b[i], joint.persistence_c[i], -1.0 };
		model2.addRow(4, rowIndex, rowValue, rowLower[1 + planeNoA + i], rowUpper[1 + planeNoA + i]);
	}

	model2.scaling(0);
	model2.setLogLevel(0);
	model2.primal();
	//model2.writeMps("a_clp.mps");
	// Print column solution


	// Alternatively getColSolution()
	Real* columnPrimal = model2.primalColumnSolution();

	xlocalA    = columnPrimal[0];
	ylocalA    = columnPrimal[1];
	zlocalA    = columnPrimal[2];
	localA     = Vector3r(xlocalA, ylocalA, zlocalA);
	xGlobalA   = localA + block.centre;
	localB     = Vector3r(xlocalB, ylocalB, zlocalB);
	xGlobalB   = localB + joint.centre;
	touchingPt = localA; //xGlobalA;
	s          = columnPrimal[3];
	// std::cout<<"xlocalA: "<<xlocalA<<", ylocalA: "<<ylocalA<<", zlocalA: "<<zlocalA<<", s: "<<s<<endl;
	int convergeSuccess = model2.status();
	if (s > -pow(10, -12) || convergeSuccess != 0) {
		// delete & model2;
		return false;
	} else {
		//delete & model2;
		return true;
	}
}


bool BlockGen::contactBoundaryLPCLP(struct Discontinuity joint, struct Block block, Vector3r& touchingPt)
{
	if (block.tooSmall == true) {
		return false;
	}

	Vector3r solution(0, 0, 0);
	/* Parameters for particles A and B */
	int planeNoA = block.a.size();
	/* Variables to keep things neat */
	int NUMCON = planeNoA /*block inequality */;
	int NUMVAR = 3 /*3D */;
	//Real s = 0.0;
	//Real xlocalA=0; Real ylocalA = 0; Real zlocalA = 0;
	Vector3r localA(0, 0, 0);
	Vector3r xGlobalA(0, 0, 0);

	/* LINEAR CONSTRAINTS */
	ClpSimplex model2;

	model2.setOptimizationDirection(1);
	// Create space for 3 columns and 10000 rows
	int numberRows    = NUMCON;
	int numberColumns = NUMVAR;
	model2.resize(0, numberColumns);

	// Columns - objective was packed
	model2.setObjectiveCoefficient(0, joint.a);
	model2.setObjectiveCoefficient(1, joint.b);
	model2.setObjectiveCoefficient(2, joint.c);
	model2.setColumnLower(0, -COIN_DBL_MAX);
	model2.setColumnLower(1, -COIN_DBL_MAX);
	model2.setColumnLower(2, -COIN_DBL_MAX);
	model2.setColumnUpper(0, COIN_DBL_MAX);
	model2.setColumnUpper(1, COIN_DBL_MAX);
	model2.setColumnUpper(2, COIN_DBL_MAX);

	Real rowLower
	        [numberRows]; //TODO: Check whether to replace C array with std::vector<> - that would be great, but first replace model2.addRow with something that can take argument std::vector<Real>
	Real rowUpper[numberRows]; //TODO: Check whether to replace C array with std::vector<>

	// Rows
	for (int i = 0; i < planeNoA; i++) {
		rowUpper[i] = block.d[i] + block.r;
		rowLower[i] = -COIN_DBL_MAX;
	}

	for (int i = 0; i < planeNoA; i++) {
		int  rowIndex[] = { 0, 1, 2 };
		Real rowValue[] = { block.a[i], block.b[i], block.c[i] };
		model2.addRow(3, rowIndex, rowValue, rowLower[i], rowUpper[i]);
	}
	model2.scaling(0);
	//model2.setSparseFactorization(0);
	model2.setLogLevel(0);
	//model2.setDualTolerance(10000.0) ;
	//model2.dual();
	model2.primal();
	//model2.writeMps("contactBoundary.mps");

	// Alternatively getColSolution()
	Real* columnPrimal = model2.primalColumnSolution();

	xGlobalA   = Vector3r(columnPrimal[0], columnPrimal[1], columnPrimal[2]);
	touchingPt = xGlobalA;

	int convergeSuccess = model2.status();
	if (convergeSuccess == 3 || convergeSuccess == 4) {
		//delete & model2;
		return false;
	} else {
		//delete & model2;
		return true;
	}
}

//bool BlockGen::contactBoundaryLPCLPslack(struct Discontinuity joint, struct Block block, Vector3r& touchingPt){

//	if(block.tooSmall == true){return false;}

//	Vector3r solution(0,0,0);
//	/* Parameters for particles A and B */
//	int planeNoA = block.a.size();
//	/* Variables to keep things neat */
//	int NUMCON = planeNoA /*block inequality */ + 1;
//	int NUMVAR = 3 +1/*3D */;
//	//Real s = 0.0;
//	//Real xlocalA=0; Real ylocalA = 0; Real zlocalA = 0;
//	Vector3r localA (0,0,0);
//	Vector3r xGlobalA (0,0,0);

//	ClpSimplex  model2;
//		  {
//			model2.setOptimizationDirection(1);
//	 // Create space for 3 columns and 10000 rows
//		       int numberRows = NUMCON;
//		       int numberColumns = NUMVAR;
//		       // This is fully dense - but would not normally be so
//		       int numberElements = numberRows * numberColumns;
//		       // Arrays will be set to default values
//		      model2.resize(numberRows, numberColumns);
//		      Real * elements = new Real [numberElements];
//		      CoinBigIndex * starts = new CoinBigIndex [numberColumns+1];
//		      int * rows = new int [numberElements];;
//		      int * lengths = new int[numberColumns];
//		       // Now fill in - totally unsafe but ....
//		       Real * columnLower = model2.columnLower();
//		       Real * columnUpper = model2.columnUpper();
//		       Real * objective = model2.objective();
//		       Real * rowLower = model2.rowLower();
//		       Real * rowUpper = model2.rowUpper();
//			// Columns - objective was packed
//			  objective[0] = 0.0; //joint.a;
//			  objective[1] = 0.0; //joint.b;
//			  objective[2] = 0.0; //joint.c;
//			  objective[3] = 1.0;
//		        for (int k = 0; k < numberColumns; k++){
//			    columnLower[k]= -COIN_DBL_MAX;
//			    columnUpper[k] = COIN_DBL_MAX;
//			}
//		       // Rows
//			for(int i=0; i<planeNoA; i++  ){
//			    rowUpper[i] = block.d[i]+block.r;
//		            rowLower[i] = -COIN_DBL_MAX;
//		        }
//			rowUpper[planeNoA] = COIN_DBL_MAX;
//		        rowLower[planeNoA] = 0.0;
//		       // assign to matrix
//	 // Now elements

//	  //Matrix3r Q1 = Matrix3r::Identity(); // toRotationMatrix() conjugates
//	  //Matrix3r Q2 = Matrix3r::Identity();

//	/* column 0 xA*/
//	starts[0] = 0;
//	CoinBigIndex put = 0;
//	for(int i=0; i < planeNoA; i++){
//			elements[put] = (block.a[i]);  rows[put] = (i);
//			put++;
//	}
//	elements[put] = (joint.a);  rows[put] = planeNoA;
//	put++;
//	  lengths[0] = planeNoA+1;

//	  /* column 1 yA*/
//	  starts[1] = put;
//	  for(int i=0; i < planeNoA; i++){
//			elements[put] = (block.b[i]);  rows[put] = (i);
//			put++;
//	  }
//	elements[put] = (joint.b);  rows[put] = planeNoA;
//	put++;
//	  lengths[1] = planeNoA+1;


//	   /* column 2 zA*/
//	  starts[2] = put;
//	  for(int i=0; i < planeNoA; i++){
//			elements[put] = (block.c[i]);  rows[put] = (i);
//			put++;
//	  }
//	elements[put] = (joint.c);  rows[put] = planeNoA;
//	put++;
//	  lengths[2] = planeNoA+1;


//	   /* column 3 s*/
//	  starts[3] = put;
//	  for(int i=0; i < planeNoA; i++){
//			elements[put] = 0.0;  rows[put] = (i);
//			put++;
//	  }
//	elements[put] = -1.0;  rows[put] = planeNoA;
//	put++;
//	  lengths[3] = planeNoA+1;
//	  starts[numberColumns] = put;


//	   CoinPackedMatrix * matrix = new CoinPackedMatrix(true, 0.0, 0.0);
//	   //matrix->assignMatrix(true, numberRows, numberColumns, numberElements,&aval[0], &asub[0], &aptrb[0], &columnCount[0]);
//	model2.setLogLevel(0);
//	   matrix->assignMatrix(true, numberRows, numberColumns, numberElements,elements, rows, starts, lengths);
//		       ClpPackedMatrix * clpMatrix = new ClpPackedMatrix(matrix);
//		       model2.replaceMatrix(clpMatrix, true);

//	}

//	   model2.primal();
//	  // model2.writeMps("contactBoundary.mps");
//		  // Print column solution
//		  //int numberColumns = model2.numberColumns();

//		  // Alternatively getColSolution()
//		  Real * columnPrimal = model2.primalColumnSolution();

//	    xGlobalA = Vector3r(columnPrimal[0],columnPrimal[1],columnPrimal[2]);
//	    touchingPt = xGlobalA;


//	 int convergeSuccess = model2.status();

//	   if(convergeSuccess==3 || convergeSuccess==4 ){
//		//delete & model2;
//		 return false;
//	   }else{
//		//delete & model2;
//		 return true;
//	   }

//	return true;

//}


bool BlockGen::checkRedundancyLPCLP(struct Discontinuity joint, struct Block block, Vector3r& touchingPt)
{
	if (block.tooSmall == true) {
		return false;
	}

	Vector3r solution(0, 0, 0);
	/* Parameters for particles A and B */
	int planeNoA = block.a.size();
	/* Variables to keep things neat */
	int NUMCON = planeNoA /*block inequality */;
	int NUMVAR = 3 /*3D */;
	//Real s = 0.0;
	//Real xlocalA=0; Real ylocalA = 0; Real zlocalA = 0;
	Vector3r localA(0, 0, 0);
	Vector3r xGlobalA(0, 0, 0);

	ClpSimplex model2;
	{
		model2.setOptimizationDirection(1);
		// Create space for 3 columns and 10000 rows
		int numberRows    = NUMCON;
		int numberColumns = NUMVAR;
		// This is fully dense - but would not normally be so
		int numberElements = numberRows * numberColumns;
		// Arrays will be set to default values
		model2.resize(numberRows, numberColumns);
		Real* elements = new Real
		        [numberElements]; //TODO: Check whether to replace C array with std::vector<> - This would be great. But CoinPackedMatrix accepts pointers only. Is there another library that can calculate the same thing?
		CoinBigIndex* starts  = new CoinBigIndex[numberColumns + 1];
		int*          rows    = new int[numberElements]; //TODO: Check whether to replace C array with std::vector<>
		int*          lengths = new int[numberColumns];  //TODO: Check whether to replace C array with std::vector<>
		// Now fill in - totally unsafe but ....
		Real* columnLower = model2.columnLower();
		Real* columnUpper = model2.columnUpper();
		Real* objective   = model2.objective();
		Real* rowLower    = model2.rowLower();
		Real* rowUpper    = model2.rowUpper();
		// Columns - objective was packed
		objective[0] = -joint.a;
		objective[1] = -joint.b;
		objective[2] = -joint.c;
		for (int k = 0; k < numberColumns; k++) {
			columnLower[k] = -COIN_DBL_MAX;
			columnUpper[k] = COIN_DBL_MAX;
		}
		// Rows
		for (int i = 0; i < planeNoA; i++) {
			rowLower[i] = -COIN_DBL_MAX;
			rowUpper[i] = block.d[i] + block.r;
		}

		// assign to matrix


		//Matrix3r Q1 = Matrix3r::Identity(); // toRotationMatrix() conjugates
		//Matrix3r Q2 = Matrix3r::Identity();
		/* column 0 xA*/
		starts[0]        = 0;
		CoinBigIndex put = 0;
		for (int i = 0; i < planeNoA; i++) {
			elements[put] = (block.a[i]);
			rows[put]     = (i);
			put++;
		}
		lengths[0] = planeNoA;


		/* column 1 yA*/
		starts[1] = put;
		for (int i = 0; i < planeNoA; i++) {
			elements[put] = (block.b[i]);
			rows[put]     = (i);
			put++;
		}
		lengths[1] = planeNoA;


		/* column 2 zA*/
		starts[2] = put;
		for (int i = 0; i < planeNoA; i++) {
			elements[put] = (block.c[i]);
			rows[put]     = (i);
			put++;
		}
		lengths[2]            = planeNoA;
		starts[numberColumns] = put;

		CoinPackedMatrix* matrix = new CoinPackedMatrix(true, 0.0, 0.0);
		//matrix->assignMatrix(true, numberRows, numberColumns, numberElements,&aval[0], &asub[0], &aptrb[0], &columnCount[0]);
		model2.setLogLevel(0);
		matrix->assignMatrix(true, numberRows, numberColumns, numberElements, elements, rows, starts, lengths);
		ClpPackedMatrix* clpMatrix = new ClpPackedMatrix(matrix);
		model2.replaceMatrix(clpMatrix, true);
	}
	model2.scaling(0);
	model2.dual();
	//  model2.writeMps("redundancy.mps");
	// Print column solution
	//int numberColumns = model2.numberColumns();

	// Alternatively getColSolution()
	Real* columnPrimal = model2.primalColumnSolution();

	xGlobalA   = Vector3r(columnPrimal[0], columnPrimal[1], columnPrimal[2]);
	touchingPt = xGlobalA;
	Real f     = touchingPt.x() * joint.a + touchingPt.y() * joint.b + touchingPt.z() * joint.c - joint.d - block.r;

	if (fabs(f) > pow(10, -3)) {
		//delete & model2;
		return false;
	} else {
		//delete & model2;
		return true;
	}
}


Real BlockGen::inscribedSphereCLP(struct Block block, Vector3r& initialPoint, bool twoDimension)
{
	/* minimise s */
	/* s.t. Ax - s <= d*/
	//  bool converge = true;
	/* Parameters for particles A and B */
	Real                 s        = 0.0; /* get value of x[3] after optimization */
	int                  planeNoA = block.a.size();
	vector<unsigned int> plane2Dno;
	if (twoDimension == true) {
		for (int i = 0; i < planeNoA; i++) {
			Vector3r plane = Vector3r(block.a[i], block.b[i], block.c[i]);
			if (fabs(plane.dot(Vector3r(0, 1, 0))) > 0.99) {
				plane2Dno.push_back(i);
				//std::cout<<"2d: "<<i<<endl;
			}
		}
		planeNoA = planeNoA - 2;
	}

	int      NUMCON  = planeNoA;
	int      NUMVAR  = 3 /*3D*/ + 1;
	Real     xlocalA = 0.0;
	Real     ylocalA = 0.0;
	Real     zlocalA = 0.0;
	Vector3r localA(0, 0, 0);

	ClpSimplex model2;

	model2.setOptimizationDirection(1);
	// Create space for 3 columns and 10000 rows
	int numberRows    = NUMCON;
	int numberColumns = NUMVAR;
	model2.resize(0, numberColumns);

	// Columns - objective was packed
	model2.setObjectiveCoefficient(0, 0.0);
	model2.setObjectiveCoefficient(1, 0.0);
	model2.setObjectiveCoefficient(2, 0.0);
	model2.setObjectiveCoefficient(3, -1.0);
	model2.setColumnLower(0, -COIN_DBL_MAX);
	model2.setColumnLower(1, -COIN_DBL_MAX);
	model2.setColumnLower(2, -COIN_DBL_MAX);
	model2.setColumnLower(3, -COIN_DBL_MAX);
	model2.setColumnUpper(0, COIN_DBL_MAX);
	model2.setColumnUpper(1, COIN_DBL_MAX);
	model2.setColumnUpper(2, COIN_DBL_MAX);
	model2.setColumnUpper(3, COIN_DBL_MAX);

	Real rowLower[numberRows]; //TODO: Check whether to replace C array with std::vector<>
	Real rowUpper[numberRows]; //TODO: Check whether to replace C array with std::vector<>
	if (twoDimension == true) {
		model2.setColumnLower(1, 0.0);
		model2.setColumnUpper(1, 0.0);
	}

	int planeIndex[planeNoA]; //TODO: Check whether to replace C array with std::vector<>
	// Rows
	int counter = 0;
	for (unsigned int i = 0; i < block.a.size(); i++) {
		if (twoDimension == true) {
			if (i == plane2Dno[0] || i == plane2Dno[1]) {
				continue;
			}
		}
		rowUpper[counter]   = block.d[i] + block.r;
		planeIndex[counter] = i;
		counter++;
	}

	for (int k = 0; k < planeNoA; k++) {
		rowLower[k] = -COIN_DBL_MAX;
	}


	for (int i = 0; i < planeNoA; i++) {
		int  rowIndex[] = { 0, 1, 2, 3 };
		Real rowValue[] = { block.a[planeIndex[i]], block.b[planeIndex[i]], block.c[planeIndex[i]], 1.0 };
		model2.addRow(4, rowIndex, rowValue, rowLower[i], rowUpper[i]);
	}


	model2.setLogLevel(0);

	model2.scaling(0);

	model2.dual();
	//model2.writeMps("inscribedSphere.mps");
	// Print column solution

	// Alternatively getColSolution()
	Real* columnPrimal = model2.primalColumnSolution();

	xlocalA      = columnPrimal[0];
	ylocalA      = columnPrimal[1];
	zlocalA      = columnPrimal[2];
	localA       = Vector3r(xlocalA, ylocalA, zlocalA);
	initialPoint = localA;
	s            = columnPrimal[3];

	int convergeSuccess = model2.status();

	if (convergeSuccess != 0) {
		//delete & model2;
		return -1; //false;
	}
	//delete & model2;
	return s;
}

//void BlockGen::positionRootBody(shared_ptr<Scene>& /*scene*/)
//{
//}

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
