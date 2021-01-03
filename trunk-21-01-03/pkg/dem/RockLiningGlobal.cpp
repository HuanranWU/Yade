/*CWBoon 2016 */
/* Please cite: */
/* CW Boon, GT Houlsby, S Utili (2015).  Designing Tunnel Support in Jointed Rock Masses Via the DEM.  Rock Mechanics and Rock Engineering,  48 (2), 603-632. */
#if defined(YADE_POTENTIAL_BLOCKS) && defined(YADE_VTK)
#include "RockLiningGlobal.hpp"
#include <lib/compatibility/VTKCompatibility.hpp> // fix InsertNextTupleValue → InsertNextTuple name change (and others in the future)
#include <lib/high-precision/Constants.hpp>
//#include<pkg/dem/KnKsLaw.hpp>
#include <core/Material.hpp>
#include <core/Omega.hpp>
#include <pkg/common/ElastMat.hpp>
#include <pkg/dem/ScGeom.hpp>
#include <cstdlib>
#include <ctime>

#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkContourFilter.h>
#include <vtkExtractVOI.h>
#include <vtkFloatArray.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangle.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>


#include <vtkDiskSource.h>
#include <vtkIntArray.h>
#include <vtkLabeledDataMapper.h>
#include <vtkLine.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkProperty.h>
#include <vtkRegularPolygonSource.h>
#include <vtkSphereSource.h>
#include <vtkVectorText.h>

#include <vtkLineSource.h>

namespace yade { // Cannot have #include directive inside.

void RockLiningGlobal::action()
{
	const Real PI = 3.14159;
	if (openingCreated == true && installed == false) {
		Real angleInterval = 2.0 * PI / static_cast<Real>(totalNodes);
		for (int n = 0; n < totalNodes; n++) {
			Real     currentAngle = 0.0 + n * angleInterval; /* from 0 degrees east */
			Real     unitX        = cos(currentAngle);
			Real     unitY        = sin(currentAngle);
			Vector3r searchDir(unitX, 0, unitY);

			vector<Real> distanceFrOpening;
			vector<int>  IDs;
			Real         outerRadius = openingRad + 1.0;
			for (const auto& b : *scene->bodies) {
				if (!b)
					continue;
				if (b->isClump() == true)
					continue;
				PotentialBlock* pb = static_cast<PotentialBlock*>(b->shape.get());
				if (!pb)
					continue;
				if (pb->isBoundary == true || pb->erase == true || pb->isLining == true) {
					continue;
				}
				State*   state1 = b->state.get();
				Vector3r intersectionPt(0, 0, 0);
				if (installLining(pb, state1, startingPoint, searchDir, outerRadius, intersectionPt)) {
					IDs.push_back(b->id);
					distanceFrOpening.push_back((intersectionPt - startingPoint).norm());
					//std::cout<<"currentAngle: "<<currentAngle<<", b->id: "<<b->id<<", dist: "<<(intersectionPt-startingPoint).norm()<<endl;
				}
			}

			/* find closest block */
			int  totalBlocks     = IDs.size();
			Real closestDistance = 100000.0;
			int  closestID       = 0;
			for (int i = 0; i < totalBlocks; i++) {
				if (distanceFrOpening[i] < closestDistance) {
					closestID       = IDs[i];
					closestDistance = distanceFrOpening[i];
				}
			}
			stickIDs.push_back(closestID);
			IDs.clear();
			distanceFrOpening.clear();
			//std::cout<<"closestID: "<<closestID<<endl;

			/* find intersection with edges of polygon */
			Vector3r        jointIntersection(0, 0, 0);
			State*          state1      = Body::byId(closestID, scene)->state.get();
			Shape*          shape1      = Body::byId(closestID, scene)->shape.get();
			PotentialBlock* pb          = static_cast<PotentialBlock*>(shape1);
			int             totalPlanes = pb->a.size();
			//int intersectNo = 0;
			Vector3r nodeLocalPos(0, 0, 0);
			Vector3r nodeGlobalPos(0, 0, 0);
			//std::cout<<"totalPlanes: "<<totalPlanes<<endl;
			Real closestPlaneDist = 1000000;
			for (int i = 0; i < totalPlanes; i++) {
				Vector3r plane  = state1->ori * Vector3r(pb->a[i], pb->b[i], pb->c[i]);
				Real     planeD = plane.dot(state1->pos) + pb->d[i] + pb->r;
				if (intersectPlane(pb, state1, startingPoint, searchDir, outerRadius, jointIntersection, plane, planeD)) {
					Real distance = jointIntersection.norm();
					if (distance < closestPlaneDist) {
						closestPlaneDist = distance;
						nodeLocalPos     = state1->ori.conjugate() * (jointIntersection - state1->pos);
						nodeGlobalPos    = jointIntersection;
					}
				}
			}
			if (nodeGlobalPos.norm() > 1.03 * openingRad) {
				nodeGlobalPos = 1.03 * openingRad * searchDir;
			}
			//if(nodeGlobalPos.norm() < 0.98*openingRad){ continue;}
			//initOverlap = interfaceTension/interfaceStiffness;
			nodeGlobalPos = nodeGlobalPos + searchDir * initOverlap;
			localCoordinates.push_back(nodeLocalPos);
			refPos.push_back(nodeGlobalPos);
			int nodeID = insertNode(nodeGlobalPos, lumpedMass, contactLength);
			blockIDs.push_back(nodeID);                //(nodeID); //(closestID);
			refOri.push_back(Quaternionr::Identity()); //(state1->ori);
			installed = true;

			axialForces.push_back(0.0);
			shearForces.push_back(0.0);
			moment.push_back(0.0);
			sigmaMax.push_back(0.0);
			sigmaMin.push_back(0.0);
			displacement.push_back(0.0);
			radialDisplacement.push_back(0.0);
		}
		totalNodes = blockIDs.size();


		/* Assembling global stiffness matrix */
		for (int n = 0; n < totalNodes; n++) {
			int nextID = n + 1;
			if (nextID == totalNodes) {
				nextID = 0;
			}
			Real Length = (refPos[nextID] - refPos[n]).norm();
			lengthNode.push_back(Length);

			Vector3r localDir = refPos[nextID] - refPos[n];
			localDir.normalize();
			refDir.push_back(localDir);
			Real     angle     = acos(localDir.dot(Vector3r(1, 0, 0)));
			Vector3r signAngle = Vector3r(1, 0.0, 0).cross(localDir);

			if (signAngle.dot(Vector3r(0, -1.0, 0)) < 0.0) {
				angle = 2.0 * PI - angle;
			}
			refAngle.push_back(angle);
			std::cout << "angle " << n << " : " << angle / PI * 180.0 << endl;
		}
	}

	//std::cout<<"complete installation"<<endl;

	if (installed == true && blockIDs.size() >= 2) {
		Real displacementMatrix[totalNodes * 3];
		memset(displacementMatrix, 0.0, sizeof(displacementMatrix));
		//averageForce = 0.0; maxForce = 0.0;
		int blockNo = blockIDs.size();

		for (int j = 0; j < blockNo; j++) {
			axialForces[j] = 0.0;
			shearForces[j] = 0.0;
			moment[j]      = 0.0;
		}

		for (int j = 0; j < blockNo; j++) {
			int nextNode = j + 1;
			if (nextNode == blockNo) {
				nextNode = 0;
			}
			State*      state1 = Body::byId(blockIDs[j], scene)->state.get();
			State*      state2 = Body::byId(blockIDs[nextNode], scene)->state.get();
			Quaternionr qA     = (state1->ori);
			Real        thetaA = 2.0 * acos(qA.w());
			if (qA.y() < 0.0) {
				thetaA = -thetaA;
			}
			Quaternionr qB     = (state2->ori);
			Real        thetaB = 2.0 * acos(qB.w());
			if (qB.y() < 0.0) {
				thetaB = -thetaB;
			}

			Real deformedAngle = refAngle[j]; // - thetaA;

			//Real temperatureForce = expansionFactor*lengthNode[j]*EA;
			//Vector3r tempForceGlobal = Vector3r(temperatureForce*cos(refAngle[j] ) , 0.0,-temperatureForce*sin(refAngle[j] ) ) ; //Ttranspose

			Vector3r globalDispA = state1->pos - refPos[j];
			Real     localXa     = globalDispA.x() * cos(deformedAngle) + globalDispA.z() * sin(deformedAngle);
			Real     localYa     = -globalDispA.x() * sin(deformedAngle) + globalDispA.z() * cos(deformedAngle);
			Vector3r globalDispB = state2->pos - refPos[nextNode];
			Real     localXb     = globalDispB.x() * cos(deformedAngle) + globalDispB.z() * sin(deformedAngle);
			Real     localYb     = -globalDispB.x() * sin(deformedAngle) + globalDispB.z() * cos(deformedAngle);

			axialForces[j] = EA / lengthNode[j] * (localXa - localXb);
			shearForces[j] = 12.0 * EI / (pow(lengthNode[j], 3)) * (localYa - localYb) - 6.0 * EI / (pow(lengthNode[j], 2)) * (thetaA + thetaB);
			moment[j]      = -6.0 * EI / (pow(lengthNode[j], 2)) * (localYa - localYb) + 2.0 * EI / lengthNode[j] * (2.0 * thetaA + thetaB);

			Real globalForceX = axialForces[j] * cos(deformedAngle) - shearForces[j] * sin(deformedAngle);
			Real globalForceZ = axialForces[j] * sin(deformedAngle) + shearForces[j] * cos(deformedAngle);

			Vector3r totalForceA = Vector3r(globalForceX, 0.0, globalForceZ);
			Vector3r torqueA     = moment[j] * Vector3r(0, 1, 0);

			Real axialForcesB = EA / lengthNode[j] * (localXb - localXa);
			Real shearForcesB = -12.0 * EI / (pow(lengthNode[j], 3)) * (localYa - localYb) + 6.0 * EI / (pow(lengthNode[j], 2)) * (thetaA + thetaB);
			Real momentB      = -6.0 * EI / (pow(lengthNode[j], 2)) * (localYa - localYb) + 2.0 * EI / lengthNode[j] * (thetaA + 2.0 * thetaB);
			Real globalForceXb   = axialForcesB * cos(deformedAngle) - shearForcesB * sin(deformedAngle);
			Real globalForceZb   = axialForcesB * sin(deformedAngle) + shearForcesB * cos(deformedAngle);
			Vector3r totalForceB = Vector3r(globalForceXb, 0.0, globalForceZb);
			Vector3r torqueB     = momentB * Vector3r(0, 1, 0);

			Real area             = liningThickness * 1.0;
			sigmaMax[j]           = axialForces[j] / area + fabs(moment[j]) * (2.0 * liningThickness) / Inertia;
			sigmaMin[j]           = axialForces[j] / area - fabs(moment[j]) * (2.0 * liningThickness) / Inertia;
			Real displacementSign = (state1->pos - refPos[j]).dot(refPos[j]);
			displacement[j]       = math::sign(displacementSign) * (state1->pos - refPos[j]).norm();
			Vector3r dir          = refPos[j];
			dir.normalize();
			radialDisplacement[j] = (state1->pos - refPos[j]).dot(dir);

			scene->forces.addTorque(blockIDs[j], -torqueA);
			scene->forces.addForce(blockIDs[j], -totalForceA);
			scene->forces.addTorque(blockIDs[nextNode], -torqueB);
			scene->forces.addForce(blockIDs[nextNode], -totalForceB);
		}

		//std::cout<<"end of rock llining global"<<endl;
	}

#if 0
	if ((scene->iter-vtkRefTimeStep)%vtkIteratorInterval == 0 && installed == true && blockIDs.size()>=2){
	 	vtkRefTimeStep = scene->iter;
	 	vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
		int blockNo = blockIDs.size();

		/// lining FORCE //
		vtkSmartPointer<vtkPoints> liningNode = vtkSmartPointer<vtkPoints>::New();
		vtkSmartPointer<vtkCellArray> liningNodeCells = vtkSmartPointer<vtkCellArray>::New();
		vtkSmartPointer<vtkFloatArray> liningNodalMoment = vtkSmartPointer<vtkFloatArray>::New();
		liningNodalMoment->SetNumberOfComponents(3);
		liningNodalMoment->SetName("lining Moment");		//Linear velocity in Vector3 form
		vtkSmartPointer<vtkFloatArray> liningAxialForce = vtkSmartPointer<vtkFloatArray>::New();
		liningAxialForce->SetNumberOfComponents(3);
		liningAxialForce->SetName("AxialForce");		//Linear velocity in Vector3 form
		vtkSmartPointer<vtkFloatArray> liningShearForce = vtkSmartPointer<vtkFloatArray>::New();
		liningShearForce->SetNumberOfComponents(3);
		liningShearForce->SetName("Shear Force");		//Linear velocity in Vector3 form
		//#if 0
		for (int i=0; i <blockNo; i++){
			int nextID = i+1;
			if(nextID == blockNo){nextID = 0;}
			State* state1 = Body::byId(blockIDs[i],scene)->state.get();
			State* state2 = Body::byId(blockIDs[nextID],scene)->state.get();
		  	Vector3r globalPoint1 = state1->pos+state1->ori*localCoordinates[i];
			Vector3r globalPoint2 = state2->pos+state2->ori*localCoordinates[nextID];
			vtkSmartPointer<vtkLineSource> lineSource =  vtkSmartPointer<vtkLineSource>::New();
			Real p0[3] = {globalPoint1[0], globalPoint1[1], globalPoint1[2]};
  			Real p1[3] = {globalPoint2[0], globalPoint2[1], globalPoint2[2]};
			lineSource->SetPoint1(p0);
			lineSource->SetPoint2(p1);
			appendFilter->AddInputConnection(lineSource-> GetOutputPort());
//#if 0
			/* try to draw forces */
			vtkIdType pid[1];
			Vector3r midPoint =  0.5*(globalPoint1+globalPoint2);
			pid[0] = liningNode->InsertNextPoint( midPoint[0],  midPoint[1],  midPoint[2]);
			liningNodeCells->InsertNextCell(1,pid);
			Vector3r plotDirection = -midPoint; //local z-direction is pointing into the tunnel (positive), and clockwise moment is positive (outer lining is subject to compression)
			plotDirection.normalize();
			Vector3r nodalMoment = moment[i]*plotDirection;
			float m[3]={nodalMoment[0],nodalMoment[1],nodalMoment[2]};

			Vector3r axialForce = axialForces[i]*plotDirection; //axialForce tension is negative  (plotdirection is pointing inwards), tension is pointing outwards
			float fa[3]={axialForce[0],axialForce[1],axialForce[2]};

			Vector3r shearForce = shearForces[i]*plotDirection;
			float fs[3]={shearForce[0],shearForce[1],shearForce[2]};
//#endif
			if(blockIDs[i] == blockIDs[nextID]  ){
				m={0,0,0};
				fa = {0,0,0};
				fs = {0,0,0};
			}
			if (Body::byId(blockIDs[i],scene)->isClumpMember()==true && Body::byId(blockIDs[nextID],scene)->isClumpMember()==true ){
				if (Body::byId(blockIDs[i],scene)->clumpId == Body::byId(blockIDs[nextID],scene)->clumpId){
					m={0,0,0};
					fa = {0,0,0};
					fs = {0,0,0};
				}

			}
			liningNodalMoment->INSERT_NEXT_TUPLE(m);
			liningAxialForce->INSERT_NEXT_TUPLE(fa);
			liningShearForce->INSERT_NEXT_TUPLE(fs);
			//lineSource->Update();
  		}
		//#endif

		//#if 0
			vtkSmartPointer<vtkUnstructuredGrid> pbUgCP = vtkSmartPointer<vtkUnstructuredGrid>::New();
			pbUgCP->SetPoints(liningNode);
			pbUgCP->SetCells(VTK_VERTEX, liningNodeCells);
			pbUgCP->GetPointData()->AddArray(liningNodalMoment);
			pbUgCP->GetPointData()->AddArray(liningAxialForce);
			pbUgCP->GetPointData()->AddArray(liningShearForce);
			vtkSmartPointer<vtkXMLUnstructuredGridWriter> writerB = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			writerB->SetDataModeToAscii();
			string filelining=fileName+"liningNodeForce"+name+"."+std::to_string(scene->iter)+".vtu";
			writerB->SetFileName(filelining.c_str());
			writerB->SetInput(pbUgCP);
			writerB->Write();


		vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkXMLPolyDataWriter::New();
		writer->SetDataModeToAscii();
		string fn=fileName+"-lining"+name+"."+std::to_string(scene->iter)+".vtp";
		writer->SetFileName(fn.c_str());
		writer->SetInputConnection(appendFilter->GetOutputPort());
		writer->Write();
		//#endif
	}
#endif

	//#if 0
	if ((scene->iter - vtkRefTimeStep) % vtkIteratorInterval == 0 && installed == true && blockIDs.size() >= 2) {
		vtkRefTimeStep                                  = scene->iter;
		vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
		int                                blockNo      = blockIDs.size();

		/// lining FORCE //
		vtkSmartPointer<vtkPointsReal> liningNode        = vtkSmartPointer<vtkPointsReal>::New();
		vtkSmartPointer<vtkCellArray>  liningNodeCells   = vtkSmartPointer<vtkCellArray>::New();
		vtkSmartPointer<vtkFloatArray> liningNodalMoment = vtkSmartPointer<vtkFloatArray>::New();
		liningNodalMoment->SetNumberOfComponents(3);
		liningNodalMoment->SetName("lining Moment"); //Linear velocity in Vector3 form
		vtkSmartPointer<vtkFloatArray> liningAxialForce = vtkSmartPointer<vtkFloatArray>::New();
		liningAxialForce->SetNumberOfComponents(3);
		liningAxialForce->SetName("AxialForce"); //Linear velocity in Vector3 form
		vtkSmartPointer<vtkFloatArray> liningShearForce = vtkSmartPointer<vtkFloatArray>::New();
		liningShearForce->SetNumberOfComponents(3);
		liningShearForce->SetName("Shear Force"); //Linear velocity in Vector3 form
		vtkSmartPointer<vtkFloatArray> liningNormalPressure = vtkSmartPointer<vtkFloatArray>::New();
		liningNormalPressure->SetNumberOfComponents(3);
		liningNormalPressure->SetName("Normal Pressure"); //Linear velocity in Vector3 form
		vtkSmartPointer<vtkFloatArray> liningNormalPressureIdeal = vtkSmartPointer<vtkFloatArray>::New();
		liningNormalPressureIdeal->SetNumberOfComponents(3);
		liningNormalPressureIdeal->SetName("Normal Pressure Magnitude");
		vtkSmartPointer<vtkFloatArray> liningTotalPressure = vtkSmartPointer<vtkFloatArray>::New();
		liningTotalPressure->SetNumberOfComponents(3);
		liningTotalPressure->SetName("Total Pressure"); //Linear velocity in Vector3 form
		//#if 0
		for (int i = 0; i < blockNo; i++) {
			int nextID = i + 1;
			if (nextID == blockNo) {
				nextID = 0;
			}
			State*          state1 = Body::byId(blockIDs[i], scene)->state.get();
			State*          state2 = Body::byId(blockIDs[nextID], scene)->state.get();
			PotentialBlock* pb     = static_cast<PotentialBlock*>(Body::byId(blockIDs[i], scene)->shape.get());

			Vector3r                       globalPoint1 = state1->pos;
			Vector3r                       globalPoint2 = state2->pos;
			vtkSmartPointer<vtkLineSource> lineSource   = vtkSmartPointer<vtkLineSource>::New();
			Real                           p0[3]        = { globalPoint1[0], globalPoint1[1], globalPoint1[2] };
			Real                           p1[3]        = { globalPoint2[0], globalPoint2[1], globalPoint2[2] };
			lineSource->SetPoint1(p0);
			lineSource->SetPoint2(p1);
			appendFilter->AddInputConnection(lineSource->GetOutputPort());
			//#if 0
			/* try to draw forces */
			vtkIdType pid[1];
			Vector3r  midPoint = globalPoint1; //  0.5*(globalPoint1+globalPoint2);
			pid[0]             = liningNode->InsertNextPoint(midPoint);
			liningNodeCells->InsertNextCell(1, pid);
			Vector3r plotDirection = midPoint;
			plotDirection.normalize();
			Vector3r nodalMoment = moment[i] * plotDirection;
			float    m[3]        = { (float)nodalMoment[0], (float)nodalMoment[1], (float)nodalMoment[2] };
			liningNodalMoment->INSERT_NEXT_TUPLE(m);

			Vector3r axialForce = -axialForces[i] * plotDirection;
			float    fa[3]      = { (float)axialForce[0], (float)axialForce[1], (float)axialForce[2] };
			liningAxialForce->INSERT_NEXT_TUPLE(fa);

			Vector3r shearForce = shearForces[i] * plotDirection;
			float    fs[3]      = { (float)shearForce[0], (float)shearForce[1], (float)shearForce[2] };
			liningShearForce->INSERT_NEXT_TUPLE(fs);

			Vector3r normalP = pb->liningNormalPressure;
			Vector3r totalP  = pb->liningTotalPressure;
			float    pN[3]   = { (float)normalP[0], (float)normalP[1], (float)normalP[2] };
			float    pT[3]   = { (float)totalP[0], (float)totalP[1], (float)totalP[2] };
			liningNormalPressure->INSERT_NEXT_TUPLE(pN);
			liningTotalPressure->INSERT_NEXT_TUPLE(pT);
			Vector3r normalPideal = -1.0 * (normalP.norm()) * plotDirection;
			float    pNi[3]       = { (float)normalPideal[0], (float)normalPideal[1], (float)normalPideal[2] };
			liningNormalPressureIdeal->INSERT_NEXT_TUPLE(pNi);
			//#endif

			//lineSource->Update();
		}
		//#endif

		//#if 0
		vtkSmartPointer<vtkUnstructuredGrid> pbUgCP = vtkSmartPointer<vtkUnstructuredGrid>::New();
		pbUgCP->SetPoints(liningNode);
		pbUgCP->SetCells(VTK_VERTEX, liningNodeCells);
		pbUgCP->GetPointData()->AddArray(liningNodalMoment);
		pbUgCP->GetPointData()->AddArray(liningAxialForce);
		pbUgCP->GetPointData()->AddArray(liningShearForce);
		pbUgCP->GetPointData()->AddArray(liningNormalPressure);
		pbUgCP->GetPointData()->AddArray(liningNormalPressureIdeal);
		pbUgCP->GetPointData()->AddArray(liningTotalPressure);
		vtkSmartPointer<vtkXMLUnstructuredGridWriter> writerB = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		writerB->SetDataModeToAscii();
		string filelining = fileName + "liningNodeForce" + name + "." + std::to_string(scene->iter) + ".vtu";
		writerB->SetFileName(filelining.c_str());
		writerB->SetInputData(pbUgCP);
		writerB->Write();


		vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkXMLPolyDataWriter::New();
		writer->SetDataModeToAscii();
		string fn = fileName + "-lining" + name + "." + std::to_string(scene->iter) + ".vtp";
		writer->SetFileName(fn.c_str());
		writer->SetInputConnection(appendFilter->GetOutputPort());
		writer->Write();
		//#endif
	}
	//#endif
}


int RockLiningGlobal::insertNode(Vector3r pos, Real mass, Real intervalLength)
{
	shared_ptr<BodyContainer>& bodies = scene->bodies;

	//std::cout<<"pos: "<<pos<<", mass: "<<mass<<", intervalLength: "<<intervalLength<<endl;

	shared_ptr<Body>           body(new Body()); /* new body */
	shared_ptr<PotentialBlock> pBlock(new PotentialBlock);
	shared_ptr<Aabb>           aabb(new Aabb);
	shared_ptr<FrictMat>       mat(new FrictMat);
	pBlock->isLining = true;
	/* Shape object, variables not added: node, gridVol,vertices */
	pBlock->AabbMinMax      = false;
	pBlock->R               = 1.0;
	pBlock->liningLength    = intervalLength;
	pBlock->liningStiffness = interfaceStiffness;
	pBlock->liningFriction  = interfaceFriction;
	pBlock->cohesion.push_back(interfaceCohesion);
	pBlock->tension.push_back(interfaceTension);
	pBlock->liningTensionGap = interfaceTension / interfaceStiffness;
	const int body_size      = bodies->size();
	int       newID          = body_size;
	pBlock->id               = newID;
	aabb->color              = Vector3r(0, 1, 0);


	body->state->ori = Quaternionr::Identity();
	body->state->pos = pos;

	mat->frictionAngle = interfaceFriction / 180.0 * 3.14159;
	body->setDynamic(true);
	body->state->mass    = mass;
	body->state->inertia = 1.0 / 12.0 * body->state->mass * (liningThickness * liningThickness + intervalLength * intervalLength) * Vector3r(1, 1, 1);
	//std::cout<<"mass: "<<mass<<endl;
	//std::cout<<"inertia: "<<body->state->inertia<<endl;

	body->shape    = pBlock;
	body->bound    = aabb;
	body->material = mat;

	//	std::cout<<"pos: "<<pos<<endl;
	//std::cout<<"before insert"<<endl;
	return bodies->insert(body);
	//std::cout<<"after insert"<<endl;
}


Vector3r RockLiningGlobal::getNodeDistance(
        const PotentialBlock* /*cm1*/,
        const State* state1,
        const PotentialBlock* /*cm2*/,
        const State*   state2,
        const Vector3r localPt1,
        const Vector3r localPt2)
{
	//Vector3r nodeDist = Vector3r(0,0,0.0);
	Vector3r global1 = state1->ori * localPt1 + state1->pos;
	Vector3r global2 = state2->ori * localPt2 + state2->pos;
	return (global2 - global1);
}


Real RockLiningGlobal::evaluateFNoSphereVol(const PotentialBlock* s1, const State* state1, const Vector3r newTrial)
{
	Vector3r tempP1 = newTrial - state1->pos;
	/* Direction cosines */
	//state1.ori.normalize();
	Vector3r localP1 = state1->ori.conjugate() * tempP1;
	Real     x       = localP1.x();
	Real     y       = localP1.y();
	Real     z       = localP1.z();
	int      planeNo = s1->a.size();

	Real r           = s1->r;
	int  insideCount = 0;
	for (int i = 0; i < planeNo; i++) {
		Real plane = s1->a[i] * x + s1->b[i] * y + s1->c[i] * z - s1->d[i] - 1.0002 * r; //-pow(10,-10);
		if (math::sign(plane) * 1.0 < 0.0) {
			insideCount++;
		}
	}

	/* Complete potential particle */
	Real f = 1.0;
	if (insideCount == planeNo) {
		f = -1.0;
	}
	return f;
}


bool RockLiningGlobal::installLining(
        const PotentialBlock* s1, const State* state1, const Vector3r startingPt, const Vector3r direction, const Real length, Vector3r& intersectionPt)
{
	//Vector3r endPt = startingPt + length*direction;

	// PotentialBlock *s1=static_cast<PotentialBlock*>(cm1.get());
	int planeNoA = s1->a.size();


	/* line equality */
	// x = x0 + t*dirX
	// y = y0 + t*dirY
	// z = z0 + t*dirZ

	/* linear inequality for blocks */
	// Ax - d < 0

	/* Variables to keep things neat */
	int  NUMCON = 3 /* equality */ + planeNoA /*block inequality */;
	int  NUMVAR = 3 /*3D */ + 1 /*t */ + 1 /* s */;
	Real s      = 0.0;
	//bool converge = true;

	Matrix3r Q1 = (state1->ori.conjugate()).toRotationMatrix();
	MatrixXr A1 = MatrixXr::Zero(planeNoA, 3);
	for (int i = 0; i < planeNoA; i++) {
		A1(i, 0) = s1->a[i];
		A1(i, 1) = s1->b[i];
		A1(i, 2) = s1->c[i];
	}
	MatrixXr AQ1 = A1 * Q1;
	MatrixXr pos1(3, 1);
	pos1(0, 0)      = state1->pos.x();
	pos1(1, 0)      = state1->pos.y();
	pos1(2, 0)      = state1->pos.z();
	MatrixXr Q1pos1 = AQ1 * pos1;


	ClpSimplex model2;
	model2.setOptimizationDirection(1);
	// Create space for 3 columns and 10000 rows
	int numberRows    = NUMCON;
	int numberColumns = NUMVAR;
	// This is fully dense - but would not normally be so

	// Arrays will be set to default values
	model2.resize(0, numberColumns);
	model2.setObjectiveCoefficient(0, 0.0);
	model2.setObjectiveCoefficient(1, 0.0);
	model2.setObjectiveCoefficient(2, 0.0);
	model2.setObjectiveCoefficient(3, 0.0);
	model2.setObjectiveCoefficient(4, 1.0);

	for (int k = 0; k < 3; k++) {
		model2.setColumnLower(k, -COIN_DBL_MAX);
		model2.setColumnUpper(k, COIN_DBL_MAX);
	}
	model2.setColumnLower(3, openingRad);
	model2.setColumnUpper(3, length);
	model2.setColumnLower(4, -COIN_DBL_MAX);
	model2.setColumnUpper(4, COIN_DBL_MAX);
	// Rows
	Real rowLower[numberRows];
	Real rowUpper[numberRows];


	rowLower[0] = startingPt.x();
	rowLower[1] = startingPt.y();
	rowLower[2] = startingPt.z();

	rowUpper[0] = startingPt.x();
	rowUpper[1] = startingPt.y();
	rowUpper[2] = startingPt.z();

	for (int k = 0; k < planeNoA; k++) {
		rowLower[3 + k] = -COIN_DBL_MAX;
		rowUpper[3 + k] = s1->d[k] + s1->r + Q1pos1(k, 0);
	}

	int  row1Index[] = { 0, 3 };
	Real row1Value[] = { 1.0, -1.0 * direction.x() };
	model2.addRow(2, row1Index, row1Value, rowLower[0], rowUpper[0]);

	int  row2Index[] = { 1, 3 };
	Real row2Value[] = { 1.0, -1.0 * direction.y() };
	model2.addRow(2, row2Index, row2Value, rowLower[1], rowUpper[1]);

	int  row3Index[] = { 2, 3 };
	Real row3Value[] = { 1.0, -1.0 * direction.z() };
	model2.addRow(2, row3Index, row3Value, rowLower[2], rowUpper[2]);

	for (int i = 0; i < planeNoA; i++) {
		int  rowIndex[] = { 0, 1, 2, 4 };
		Real rowValue[] = { AQ1(i, 0), AQ1(i, 1), AQ1(i, 2), -1.0 };
		model2.addRow(4, rowIndex, rowValue, rowLower[3 + i], rowUpper[3 + i]);
	}

	model2.scaling(0);
	model2.setLogLevel(0);
	model2.primal();

	// Alternatively getColSolution()
	Real* columnPrimal = model2.primalColumnSolution();


	Vector3r temp  = Vector3r(columnPrimal[0], columnPrimal[1], columnPrimal[2]);
	intersectionPt = temp; //state1->ori.conjugate()*(temp-state1->pos);
	s              = columnPrimal[4];

	int convergeSuccess = model2.status();
	if (s > -pow(10, -8) || convergeSuccess != 0) {
		return false;
	} else {
		return true;
	}
}


bool RockLiningGlobal::intersectPlane(
        const PotentialBlock* s1,
        const State*          state1,
        const Vector3r        startingPt,
        const Vector3r        direction,
        const Real            length,
        Vector3r&             intersectionPt,
        const Vector3r        plane,
        const Real            planeD)
{
	//bool feasible = true;
	//Vector3r endPt = startingPt + length*direction;

	// PotentialBlock *s1=static_cast<PotentialBlock*>(cm1.get());
	//int planeNoA = s1->a.size();

	/* Variables to keep things neat */
	int  NUMCON = 3 /* equality */ + 1 /*planeEquality */;
	int  NUMVAR = 3 /*3D */ + 1 /*t */;
	Real t      = 0.0;
	//bool converge = true;

	/* line equality */
	// x = x0 + t*dirX
	// y = y0 + t*dirY
	// z = z0 + t*dirZ

	/* linear equality for blocks */
	// Ax - d = 0
	/* LINEAR CONSTRAINTS */

	ClpSimplex model2;
	model2.setOptimizationDirection(1);
	// Create space for 3 columns and 10000 rows
	int numberRows    = NUMCON;
	int numberColumns = NUMVAR;
	// This is fully dense - but would not normally be so

	// Arrays will be set to default values
	model2.resize(0, numberColumns);
	model2.setObjectiveCoefficient(0, 0.0);
	model2.setObjectiveCoefficient(1, 0.0);
	model2.setObjectiveCoefficient(2, 0.0);
	model2.setObjectiveCoefficient(3, 1.0);

	for (int k = 0; k < 4; k++) {
		model2.setColumnLower(k, -COIN_DBL_MAX);
		model2.setColumnUpper(k, COIN_DBL_MAX);
	}

	// Rows
	Real rowLower[numberRows];
	Real rowUpper[numberRows];

	rowLower[0] = startingPt.x();
	rowLower[1] = startingPt.y();
	rowLower[2] = startingPt.z();
	rowLower[3] = planeD;
	rowUpper[0] = startingPt.x();
	rowUpper[1] = startingPt.y();
	rowUpper[2] = startingPt.z();
	rowUpper[3] = planeD;

	int  row1Index[] = { 0, 3 };
	Real row1Value[] = { 1.0, -1.0 * direction.x() };
	model2.addRow(2, row1Index, row1Value, rowLower[0], rowUpper[0]);

	int  row2Index[] = { 1, 3 };
	Real row2Value[] = { 1.0, -1.0 * direction.y() };
	model2.addRow(2, row2Index, row2Value, rowLower[1], rowUpper[1]);

	int  row3Index[] = { 2, 3 };
	Real row3Value[] = { 1.0, -1.0 * direction.z() };
	model2.addRow(2, row3Index, row3Value, rowLower[2], rowUpper[2]);

	int  row4Index[] = { 0, 1, 2 };
	Real row4Value[] = { plane.x(), plane.y(), plane.z() };
	model2.addRow(3, row4Index, row4Value, rowLower[3], rowUpper[3]);

	model2.scaling(0);
	model2.setLogLevel(0);
	model2.primal();
	Real* columnPrimal = model2.primalColumnSolution();


	Vector3r temp  = Vector3r(columnPrimal[0], columnPrimal[1], columnPrimal[2]);
	intersectionPt = temp; //state1->ori.conjugate()*(temp-state1->pos);
	t              = columnPrimal[3];

	int  convergeSuccess = model2.status();
	Real f               = evaluateFNoSphereVol(s1, state1, intersectionPt);
	//std::cout<<"t: "<<t<<", f: "<<f<<", status: "<<status<<endl;
	if (t > 1.001 * length || t < 0.0 || f > 0.0 || convergeSuccess != 0) {
		return false;
	} else {
		return true;
	}
}

YADE_PLUGIN((RockLiningGlobal));

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS && YADE_VTK
