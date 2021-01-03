/* CWBoon 2016 */
#ifdef YADE_POTENTIAL_BLOCKS

#include <core/Clump.hpp>
#include <pkg/common/Aabb.hpp>
#include <pkg/dem/KnKsPBLaw.hpp>
#include <pkg/dem/ScGeom.hpp>

#include "Gl1_PotentialBlock.hpp"

#ifdef YADE_VTK

#include <lib/compatibility/VTKCompatibility.hpp>
#include <vtkActor2D.h>
#include <vtkAppendPolyData.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkConeSource.h>
#include <vtkContourFilter.h>
#include <vtkDiskSource.h>
#include <vtkExtractVOI.h>
#include <vtkFloatArray.h>
#include <vtkImplicitBoolean.h>
#include <vtkIntArray.h>
#include <vtkLabeledDataMapper.h>
#include <vtkLine.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSampleFunction.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangle.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVectorText.h>
#include <vtkWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

#endif // YADE_VTK

namespace yade { // Cannot have #include directive inside.

/* New script to visualise the PBs using OPENGL. The triangulation of the particles is derived from: PotentialBlock.connectivity */
#ifdef YADE_OPENGL
bool Gl1_PotentialBlock::wire;

void Gl1_PotentialBlock::go(const shared_ptr<Shape>& cm, const shared_ptr<State>&, bool wire2, const GLViewInfo&)
{
	glColor3v(cm->color); //glColor3v is used when lighting is not enabled
	PotentialBlock* pb = static_cast<PotentialBlock*>(cm.get());

	if (wire || wire2) {
		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //Turn on wireframe mode. Render front and back faces of the wireframe
	} else {
		glMaterialv(
		        GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Vector3r(cm->color[0], cm->color[1], cm->color[2])); //glMaterialv is used when lighting is enabled
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
		glPolygonMode(GL_FRONT, GL_FILL); // Turn off wireframe mode. Render only front faces
	}

	vector<vector<int>> con = pb->connectivity;

	//TODO: Orient the faces always as cwise or ccwise in the shape class, to avoid going through the if statement below in every timestep qt.View() is activated.
	for (int j = 0; j < (int)con.size(); j++) {
		Vector3r n = (pb->vertices[con[j][1]] - pb->vertices[con[j][0]]).cross(pb->vertices[con[j][2]] - pb->vertices[con[j][0]]);
		n.normalize();
		Vector3r nFace = Vector3r(pb->a[j], pb->b[j], pb->c[j]);

		glBegin(GL_TRIANGLE_FAN)
			;
			glNormal3v(nFace);
			if (n.dot(nFace) < 0.0) {
				for (int i1 = con[j].size() - 1; i1 >= 0; i1--) {
					glVertex3v(pb->vertices[con[j][i1]]);
				} // Create a fan with vertices on plane in descending order
			} else {
				for (unsigned int i2 = 0; i2 < con[j].size(); i2++) {
					glVertex3v(pb->vertices[con[j][i2]]);
				} // Create a fan with vertices on plane in ascending order
			}
		glEnd();
	}
}
YADE_PLUGIN((Gl1_PotentialBlock));
#endif // YADE_OPENGL

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* PREVIOUSLY EXISTING CODE DEVELOPED BY CW BOON USING THE MARCHING CUBES */
/* TODO  THE EXISTING CODE WILL BE USED TO VISUALISE THE INNER POTENTIAL PARTICLE, BY INTRODUCING A BOOLEAN TO CHOOSE BETWEEN display="ACTUAL_PARTICLE" OR display="INNER_PP" */

//#ifdef YADE_OPENGL

////if (display == 0){
//////	CGAL script/Actual Particle


////}else if(display==1) {
////// Marching Cubes/Inner Potential Particle

//	void Gl1_PotentialBlock::calcMinMax(const PotentialBlock& pp)
//	{
//		Min = -aabbEnlargeFactor*pp.minAabb;
//		Max =  aabbEnlargeFactor*pp.maxAabb;
//
//		float dx = (Max[0]-Min[0])/((float)(sizeX-1));
//		float dy = (Max[1]-Min[1])/((float)(sizeY-1));
//		float dz = (Max[2]-Min[2])/((float)(sizeZ-1));

//		isoStep=Vector3r(dx,dy,dz);
//	}


//	void Gl1_PotentialBlock::generateScalarField(const PotentialBlock& pp)
//	{

//		for(int i=0;i<sizeX;i++){
//			for(int j=0;j<sizeY;j++){
//				for(int k=0;k<sizeZ;k++){
//					scalarField[i][j][k] = evaluateF(pp,  Min[0]+ Real(i)*isoStep[0],  Min[1]+ Real(j)*isoStep[1],  Min[2]+Real(k)*isoStep[2]);//
//				}
//			}
//		}
//	}


//	vector<Gl1_PotentialBlock::scalarF> Gl1_PotentialBlock::SF;
//	int Gl1_PotentialBlock::sizeX, Gl1_PotentialBlock::sizeY, Gl1_PotentialBlock::sizeZ;
//	bool Gl1_PotentialBlock::store;
//	bool Gl1_PotentialBlock::initialized;
//	Real Gl1_PotentialBlock::aabbEnlargeFactor;
//	//void Gl1_PotentialBlock::clearMemory(){
//	//SF.clear();
//	//}


//	bool Gl1_PotentialBlock::wire;

//	void Gl1_PotentialBlock::go( const shared_ptr<Shape>& cm, const shared_ptr<State>& state ,bool wire2, const GLViewInfo&){


//		PotentialBlock* pp = static_cast<PotentialBlock*>(cm.get());
//		int shapeId = pp->id;

//		if(store == false) {
//			if(SF.size()>0) {
//				SF.clear();
//				initialized = false;
//			}
//		}

//		/* CONSTRUCTION OF PARTICLE SURFACE USING THE MARCHING CUBES ALGORITHM */
//		if(initialized == false ) {
//			for(const auto & b :  *scene->bodies) {
//				if (!b) continue;
//				PotentialBlock* cmbody = dynamic_cast<PotentialBlock*>(b->shape.get());
//				if (!cmbody) continue;

//					Matrix3r rotation = b->state->ori.toRotationMatrix(); //*pb->oriAabb.conjugate();
//					int count = 0;
//					for (int i=0; i<3; i++){
//						for (int j=0; j<3; j++){
//							//function->rotationMatrix[count] = directionCos(j,i);
//							rotationMatrix(i,j) = rotation(i,j);	//input is actually direction cosine?
//							count++;
//						}
//					}

//				calcMinMax(*cmbody);
//				mc.init(sizeX,sizeY,sizeZ,Min,Max);
//				mc.resizeScalarField(scalarField,sizeX,sizeY,sizeZ);
//				SF.push_back(scalarF());
//				generateScalarField(*cmbody);
//				mc.computeTriangulation(scalarField,0.0);
//				SF[cmbody->id].triangles = mc.getTriangles();
//				SF[cmbody->id].normals = mc.getNormals();
//				SF[cmbody->id].nbTriangles = mc.getNbTriangles();
//				for(unsigned int i=0; i<scalarField.size(); i++) {
//					for(unsigned int j=0; j<scalarField[i].size(); j++) scalarField[i][j].clear();
//					scalarField[i].clear();
//				}
//				scalarField.clear();
//			}
//			initialized = true;
//		}


//		/* VISUALIZATION USING OPENGL */
//		const vector<Vector3r>& triangles = SF[shapeId].triangles; //mc.getTriangles();
//		int nbTriangles = SF[shapeId].nbTriangles; // //mc.getNbTriangles();
//		const vector<Vector3r>& normals = SF[shapeId].normals; //mc.getNormals();
//		glDisable(GL_CULL_FACE);

//		if (wire || wire2) {
//			glDisable(GL_LIGHTING);
//			glBegin(GL_LINES);
//			for(int i=0; i<3*nbTriangles; i+=3) {
//				glVertex3v(triangles[i+0]); glVertex3v(triangles[i+1]);
//				glVertex3v(triangles[i+0]); glVertex3v(triangles[i+2]);
//				glVertex3v(triangles[i+1]); glVertex3v(triangles[i+2]);
//			}
//			glEnd();
//		} else {

//			glMaterialv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Vector3r(cm->color[0],cm->color[1],cm->color[2]));
//			glColor3v(cm->color);
//			//glColorMaterial(GL_BACK,GL_AMBIENT_AND_DIFFUSE);

//			glEnable(GL_LIGHTING); // 2D
//			glEnable(GL_NORMALIZE);
//			//glEnable(GL_RESCALE_NORMAL); // alternative to GL_NORMALIZE
//			glBegin(GL_TRIANGLES);

//			for(int i=0; i<3*nbTriangles; ++i) {
//	//			glNormal3v(normals[i]);
//	//			glVertex3v(triangles[i]);
//	//			glNormal3v(normals[++i]);
//	//			glVertex3v(triangles[i]);
//	//			glNormal3v(normals[++i]);
//	//			glVertex3v(triangles[i]);
//
//				glNormal3v(normals[i]);
//					glVertex3v(triangles[i]);
//				glNormal3v(normals[i+1]);
//					//glVertex3v(triangles[i+1]);
//				glNormal3v(normals[i+2]);
//					//glVertex3v(triangles[i+2]);
//			}

//			glEnd();

//		}
//		return;
//	}


//	Real Gl1_PotentialBlock::evaluateF(const PotentialBlock& pp, Real x, Real y, Real z){
//		Real r = pp.r;
//		int planeNo = pp.a.size();

//		//Vector3r xori(x,y,z);
//		//Vector3r xlocal = rotationMatrix*xori;
//		//rotationMatrix*xori;
//		//xlocal[0] = rotationMatrix(0,0)*xori[0] + rotationMatrix(0,1)*xori[1] + rotationMatrix(0,2)*xori[2];
//		//xlocal[1] = rotationMatrix(1,0)*xori[0] + rotationMatrix(1,1)*xori[1] + rotationMatrix(1,2)*xori[2];
//		//xlocal[2] = rotationMatrix(2,0)*xori[0] + rotationMatrix(2,1)*xori[1] + rotationMatrix(2,2)*xori[2];

//		vector<Real>a; vector<Real>b; vector<Real>c; vector<Real>d; vector<Real>p; Real pSum3 = 0.0;
//		for (int i=0; i<planeNo; i++){
//			Vector3r planeOri(pp.a[i],pp.b[i],pp.c[i]);
///* PREVIOUS */ 		Vector3r planeRotated = planeOri; //rotationMatrix*planeOri; //FIXME
///*TO REVISIT*/		//Vector3r planeRotated = se3.orientation*planeOri //FIXME

//			Real plane = planeRotated.x()*x + planeRotated.y()*y + planeRotated.z()*z - pp.d[i]; if (plane<pow(10,-15)){plane = 0.0;}
//			//Real plane =  pp.a[i]*xlocal[0] +  pp.b[i]*xlocal[1] +  pp.c[i]*xlocal[2] - pp.d[i]; if (plane<pow(10,-15)){plane = 0.0;}
////			p.push_back(plane);
//			pSum3 += pow(plane,2);
//		}

//		Real f = (pSum3-1.0*pow(r,2));
//	//	Real f = (pSum3-1.0*pow(r,1));

//		return f;
//	}

////} // display
//#endif // YADE_OPENGL


#ifdef YADE_VTK

ImpFuncPB* ImpFuncPB::New()
{
	// Skip factory stuff - create class
	return new ImpFuncPB;
}


// Create the function
ImpFuncPB::ImpFuncPB()
{
	clump = false;
	// Initialize members here if you need
}

ImpFuncPB::~ImpFuncPB()
{
	// Initialize members here if you need
}

// Evaluate function
Real ImpFuncPB::FunctionValue(Real x[3])
{
	int          planeNo = a.size();
	vector<Real> p;
	Real         pSum2 = 0.0;
	if (clump == false) {
		Vector3r xori(x[0], x[1], x[2]);
		Vector3r xlocal = rotationMatrix * xori;
		xlocal[0]       = rotationMatrix(0, 0) * x[0] + rotationMatrix(0, 1) * x[1] + rotationMatrix(0, 2) * x[2];
		xlocal[1]       = rotationMatrix(1, 0) * x[0] + rotationMatrix(1, 1) * x[1] + rotationMatrix(1, 2) * x[2];
		xlocal[2]       = rotationMatrix(2, 0) * x[0] + rotationMatrix(2, 1) * x[1] + rotationMatrix(2, 2) * x[2];
		//std::cout<<"rotationMatrix: "<<endl<<rotationMatrix<<endl;
		//x[0]=xlocal[0]; x[1]=xlocal[1]; x[2]=xlocal[2];

		for (int i = 0; i < planeNo; i++) {
			Real plane = a[i] * xlocal[0] + b[i] * xlocal[1] + c[i] * xlocal[2] - d[i];
			if (plane < pow(10, -15)) {
				plane = 0.0;
			}
			p.push_back(plane);
			pSum2 += pow(p[i], 2);
		}
		//  Real sphere  = (  pow(xlocal[0],2) + pow(xlocal[1],2) + pow(xlocal[2],2) ) ;
		//  Real f = (1.0-k)*(pSum2/pow(r,2) - 1.0)+k*(sphere/pow(R,2)-1.0);
		Real f = (pSum2 - 1.0 * pow(r, 2));
		return f;
	} else {
		Vector3r xori(x[0], x[1], x[2]);
		Vector3r clumpMemberCentre(clumpMemberCentreX, clumpMemberCentreY, clumpMemberCentreZ);
		Vector3r xlocal = xori - clumpMemberCentre;
		//xlocal[0] = rotationMatrix[0]*x[0] + rotationMatrix[3]*x[1] + rotationMatrix[6]*x[2];
		//xlocal[1] = rotationMatrix[1]*x[0] + rotationMatrix[4]*x[1] + rotationMatrix[7]*x[2];
		//xlocal[2] = rotationMatrix[2]*x[0] + rotationMatrix[5]*x[1] + rotationMatrix[8]*x[2];
		//std::cout<<"rotationMatrix: "<<endl<<rotationMatrix<<endl;
		//x[0]=xlocal[0]; x[1]=xlocal[1]; x[2]=xlocal[2];

		for (int i = 0; i < planeNo; i++) {
			Real plane = a[i] * xlocal[0] + b[i] * xlocal[1] + c[i] * xlocal[2] - d[i];
			if (plane < pow(10, -15)) {
				plane = 0.0;
			}
			p.push_back(plane);
			pSum2 += pow(p[i], 2);
		}
		// Real sphere  = (  pow(xlocal[0],2) + pow(xlocal[1],2) + pow(xlocal[2],2) ) ;
		// Real f = (1.0-k)*(pSum2/pow(r,2) - 1.0)+k*(sphere/pow(R,2)-1.0);
		Real f = (pSum2 - 1.5 * pow(r, 2));
		return f;
		// return 0;
	}
	// the value of the function
}


void PotentialBlockVTKRecorder::action()
{
	if (fileName.size() == 0)
		return;
	auto pbPos          = vtkSmartPointer<vtkPointsReal>::New();
	auto appendFilter   = vtkSmartPointer<vtkAppendPolyData>::New();
	auto appendFilterID = vtkSmartPointer<vtkAppendPolyData>::New();
	//auto transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	//auto transform = vtkSmartPointer<vtkTransform>::New();

	// interactions ###############################################
	auto intrBodyPos = vtkSmartPointer<vtkPoints>::New();
	auto intrCells   = vtkSmartPointer<vtkCellArray>::New();
	auto intrForceN  = vtkSmartPointer<vtkFloatArray>::New();
	intrForceN->SetNumberOfComponents(3);
	intrForceN->SetName("forceN");
	auto intrAbsForceT = vtkSmartPointer<vtkFloatArray>::New();
	intrAbsForceT->SetNumberOfComponents(1);
	intrAbsForceT->SetName("absForceT");
	// interactions ###############################################

	// interaction contact point ###############################################
	auto pbContactPoint = vtkSmartPointer<vtkPointsReal>::New();
	auto pbCellsContact = vtkSmartPointer<vtkCellArray>::New();
	auto pbNormalForce  = vtkSmartPointer<vtkFloatArray>::New();
	pbNormalForce->SetNumberOfComponents(3);
	pbNormalForce->SetName("normalForce"); //Linear velocity in Vector3 form
	auto pbShearForce = vtkSmartPointer<vtkFloatArray>::New();
	pbShearForce->SetNumberOfComponents(3);
	pbShearForce->SetName("shearForce"); //Angular velocity in Vector3 form
	auto pbTotalForce = vtkSmartPointer<vtkFloatArray>::New();
	pbTotalForce->SetNumberOfComponents(3);
	pbTotalForce->SetName("totalForce");
	auto pbTotalStress = vtkSmartPointer<vtkFloatArray>::New();
	pbTotalStress->SetNumberOfComponents(3);
	pbTotalStress->SetName("totalStress");
	auto pbMobilizedShear = vtkSmartPointer<vtkFloatArray>::New();
	pbMobilizedShear->SetNumberOfComponents(1);
	pbMobilizedShear->SetName("mobilizedShear");
	// interactions contact point###############################################


	// velocity ###################################################
	auto pbCells     = vtkSmartPointer<vtkCellArray>::New();
	auto pbLinVelVec = vtkSmartPointer<vtkFloatArray>::New();
	pbLinVelVec->SetNumberOfComponents(3);
	pbLinVelVec->SetName("linVelVec"); //Linear velocity in Vector3 form

	auto pbLinVelLen = vtkSmartPointer<vtkFloatArray>::New();
	pbLinVelLen->SetNumberOfComponents(1);
	pbLinVelLen->SetName("linVelLen"); //Length (magnitude) of linear velocity

	auto pbAngVelVec = vtkSmartPointer<vtkFloatArray>::New();
	pbAngVelVec->SetNumberOfComponents(3);
	pbAngVelVec->SetName("angVelVec"); //Angular velocity in Vector3 form

	auto pbAngVelLen = vtkSmartPointer<vtkFloatArray>::New();
	pbAngVelLen->SetNumberOfComponents(1);
	pbAngVelLen->SetName("angVelLen"); //Length (magnitude) of angular velocity


	auto pbDisplacementVec = vtkSmartPointer<vtkFloatArray>::New();
	pbDisplacementVec->SetNumberOfComponents(3);
	pbDisplacementVec->SetName("Displacement"); //Linear velocity in Vector3 form

	// velocity ####################################################

	// bodyId ##############################################################
	//#if 0
	auto pbPosID   = vtkSmartPointer<vtkPointsReal>::New();
	auto pbIdCells = vtkSmartPointer<vtkCellArray>::New();
	auto blockId   = vtkSmartPointer<vtkIntArray>::New();
	blockId->SetNumberOfComponents(1);
	blockId->SetName("id");
	// bodyId ##############################################################
	//#endif
	int                                       countID = 0;
	vtkSmartPointer<vtkVectorText>            textArray2[scene->bodies->size()];
	vtkSmartPointer<vtkPolyDataMapper>        txtMapper[scene->bodies->size()];
	vtkSmartPointer<vtkLinearExtrusionFilter> extrude[scene->bodies->size()];
	vtkSmartPointer<vtkActor>                 textActor[scene->bodies->size()];


	for (const auto& b : *scene->bodies) {
		if (!b)
			continue;
		if (b->isClumpMember() == true)
			continue;

		//const PotentialBlock* pb=dynamic_cast<PotentialBlock*>(b->shape.get());
		//if(!pb) continue;
		if (REC_ID == true) {
			//#if 0
			blockId->InsertNextValue(b->getId());
			vtkIdType pid[1];
			Vector3r  pos(b->state->pos);
			pid[0] = pbPosID->InsertNextPoint(pos);
			pbIdCells->InsertNextCell(1, pid);
			//#endif
			/* ################# Display id VTK extrusion vector ############## */

#if 0
			textArray2[countID]= vtkVectorText::New();
			//#if 0
			int bid = b->id;
			std::string testString = boost::lexical_cast<std::string>(bid);
			const char * testChar = testString.c_str();
			textArray2[countID]->SetText(testChar);
			extrude[countID] = vtkLinearExtrusionFilter::New();
		   	extrude[countID]->SetInputConnection( textArray2[countID]->GetOutputPort());
			extrude[countID]->SetExtrusionTypeToNormalExtrusion();
			extrude[countID]->SetVector(0, 0, 1.0 );
			extrude[countID]->SetScaleFactor (1.0);
#if 0
			txtMapper[countID] = vtkPolyDataMapper::New();
			txtMapper[countID]->SetInputConnection( extrude[countID]->GetOutputPort());
			textActor[countID] = vtkActor::New();
			textActor[countID]->SetMapper(txtMapper[countID]);
			textActor[countID]->RotateX(-90);
			textActor[countID]->SetPosition(b->state->pos[0], b->state->pos[1], b->state->pos[2]);
			Real contactPtSize = 5.0;
			textActor[countID]->SetScale(3.0*contactPtSize);
			textActor[countID]->GetProperty()->SetColor(0.0,0.0,0.0);
#endif
			//#if 0
			//txtMapper[countID] = vtkPolyDataMapper::New();
			//txtMapper[countID]->SetInputConnection( extrude[count]->GetOutputPort());
			//appendFilterID->AddInputConnection(extrude[countID]-> GetOutputPort());
			//auto polydata = vtkSmartPointer<vtkPolyData>::New();
			//polydata->DeepCopy(extrude[countID]-> GetOutput());
			Vector3r centre (b->state->pos[0], b->state->pos[1], b->state->pos[2]);
			auto transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
			transformFilter->SetInput( extrude[countID]-> GetOutput() );
			auto transform = vtkSmartPointer<vtkTransform>::New();
			transformFilter->SetTransform( transform );
			transform->PostMultiply();
			transform->Translate (centre[0], centre[1],centre[2]);
			//#endif
			appendFilterID->AddInputConnection(transformFilter-> GetOutputPort());
			//polydata->DeleteCells();

#endif
			countID++;
		}
		//vtkSmartPointer<ImpFuncPB> function = ImpFuncPB::New();
		vtkSmartPointer<vtkImplicitBoolean> boolFunction   = vtkSmartPointer<vtkImplicitBoolean>::New();
		vtkSmartPointer<ImpFuncPB>*         functionBool   = nullptr;
		int                                 ImplicitBoolNo = 0;
		Real                                xmin           = 0.0;
		Real                                xmax           = 0.0;
		Real                                ymin           = 0.0;
		Real                                ymax           = 0.0;
		Real                                zmin           = 0.0;
		Real                                zmax           = 0.0;
		Vector3r                            particleColour(0, 0, 0);
		if (b->isClump() == false && b->isClumpMember() == false) {
			const PotentialBlock* pb = dynamic_cast<PotentialBlock*>(b->shape.get());
			if (pb->isLining == true) {
				continue;
			}
			function->a = pb->a;
			function->b = pb->b;
			function->c = pb->c;
			function->d = pb->d;
			function->R = pb->R;
			function->r = pb->r;
			function->k = pb->k;
			//Matrix3r directionCos = b->state->ori.conjugate().toRotationMatrix();  //FIXME
			Matrix3r rotation = b->state->ori.conjugate().toRotationMatrix(); //*pb->oriAabb.conjugate();
			int      count    = 0;
			function->clump   = false;
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					//function->rotationMatrix[count] = directionCos(j,i);
					function->rotationMatrix(i, j) = rotation(i, j); //input is actually direction cosine?
					count++;
				}
			}

			if (pb->minAabb.norm() > 0 and pb->maxAabb.norm() > 0) {
				xmin = -pb->minAabb.x();
				xmax = pb->maxAabb.x();
				ymin = -pb->minAabb.y();
				ymax = pb->maxAabb.y();
				zmin = -pb->minAabb.z();
				zmax = pb->maxAabb.z();
			} else {
				const Aabb* aabb = static_cast<Aabb*>(b->bound.get());
				xmin             = aabb->min.x() - b->state->pos.x();
				xmax             = aabb->max.x() - b->state->pos.x();
				ymin             = aabb->min.y() - b->state->pos.y();
				ymax             = aabb->max.y() - b->state->pos.y();
				zmin             = aabb->min.z() - b->state->pos.z();
				zmax             = aabb->max.z() - b->state->pos.z();

				xmin *= 1.2;
				xmax *= 1.2;
				ymin *= 1.2;
				ymax *= 1.2;
				zmin *= 1.2;
				zmax *= 1.2;
			}
			particleColour = pb->color;

		} else if (b->isClump() == true) {
			//const Clump* clump = dynamic_cast<Clump*>(b->shape.get());
			const shared_ptr<Clump> clump(YADE_PTR_CAST<Clump>(b->shape));

			//vtkSmartPointer<ImpFunc> functionBool [clump->ids.size()];
			functionBool   = new vtkSmartPointer<ImpFuncPB>[clump->ids.size()];
			ImplicitBoolNo = clump->ids.size();

			for (unsigned int i = 0; i < clump->ids.size(); i++) {
				const shared_ptr<Body> clumpMember  = Body::byId(clump->ids[i], scene);
				const PotentialBlock*  pbShape      = dynamic_cast<PotentialBlock*>(clumpMember->shape.get());
				functionBool[i]                     = vtkSmartPointer<ImpFuncPB>::New();
				functionBool[i]->R                  = pbShape->R;
				functionBool[i]->r                  = pbShape->r;
				functionBool[i]->k                  = pbShape->k;
				functionBool[i]->clump              = true;
				functionBool[i]->clumpMemberCentreX = clumpMember->state->pos.x() - b->state->pos.x();
				functionBool[i]->clumpMemberCentreY = clumpMember->state->pos.y() - b->state->pos.y();
				functionBool[i]->clumpMemberCentreZ = clumpMember->state->pos.z() - b->state->pos.z();

				//Matrix3r directionCos = clumpMember->state->ori.conjugate().toRotationMatrix();  //FIXME
				Matrix3r rotation = clumpMember->state->ori.toRotationMatrix(); //*pbShape->oriAabb.conjugate();
				for (unsigned int j = 0; j < pbShape->a.size(); j++) {
					Vector3r plane = rotation * Vector3r(pbShape->a[j], pbShape->b[j], pbShape->c[j]);
					Real     d     = pbShape->d[j];
					//Real     d     = -1.0*(plane.x()*(b->state->pos.x()-clumpMember->state->pos.x() ) + plane.y()*(b->state->pos.y()-clumpMember->state->pos.y() ) + plane.z()*(b->state->pos.z()-clumpMember->state->pos.z() ) - pbShape->d[j]);
					functionBool[i]->a.push_back(plane.x());
					functionBool[i]->b.push_back(plane.y());
					functionBool[i]->c.push_back(plane.z());
					functionBool[i]->d.push_back(d);
				}

				const Aabb* aabb = static_cast<Aabb*>(clumpMember->bound.get());

				xmin = std::min(xmin, aabb->min.x() - b->state->pos.x());
				xmax = std::max(xmax, aabb->max.x() - b->state->pos.x());
				ymin = std::min(ymin, aabb->min.y() - b->state->pos.y());
				ymax = std::max(ymax, aabb->max.y() - b->state->pos.y());
				zmin = std::min(zmin, aabb->min.z() - b->state->pos.z());
				zmax = std::max(zmax, aabb->max.z() - b->state->pos.z());

				xmin *= 1.2;
				xmax *= 1.2;
				ymin *= 1.2;
				ymax *= 1.2;
				zmin *= 1.2;
				zmax *= 1.2;

				particleColour = pbShape->color;

				boolFunction->AddFunction(functionBool[i]);
			}
			boolFunction->SetOperationTypeToUnion();
		}
		auto sample = vtkSmartPointer<vtkSampleFunctionReal>::New();
		if (b->isClump() == false && b->isClumpMember() == false) {
			sample->SetImplicitFunction(function);
		} else if (b->isClump() == true) {
			sample->SetImplicitFunction(boolFunction);
			boolFunction->SetOperationTypeToUnion();
		}

		if (twoDimension == true) {
			if (sampleY < 2) {
				ymin = 0.0;
				ymax = 0.0;
			} else if (sampleZ < 2) {
				zmin = 0.0;
				zmax = 0.0;
			}
		}

		sample->SetModelBounds(Vector3r(xmin, ymin, zmin), Vector3r(xmax, ymax, zmax));

		int sampleXno = sampleX;
		int sampleYno = sampleY;
		int sampleZno = sampleZ;
		if (fabs(xmax - xmin) / static_cast<Real>(sampleX) > maxDimension) {
			sampleXno = static_cast<int>(fabs(xmax - xmin) / maxDimension);
		}
		if (fabs(ymax - ymin) / static_cast<Real>(sampleY) > maxDimension) {
			sampleYno = static_cast<int>(fabs(ymax - ymin) / maxDimension);
		}
		if (fabs(zmax - zmin) / static_cast<Real>(sampleZ) > maxDimension) {
			sampleZno = static_cast<int>(fabs(zmax - zmin) / maxDimension);
		}

		if (twoDimension == true) {
			if (sampleY < 2) {
				sampleYno = 1;
			} else if (sampleZ < 2) {
				sampleZno = 1;
			}
		}

		sample->SetSampleDimensions(sampleXno, sampleYno, sampleZno);
		sample->ComputeNormalsOff();
		//sample->Update();
		auto contours = vtkSmartPointer<vtkContourFilter>::New();
		contours->SetInputConnection(sample->GetOutputPort());
		contours->SetNumberOfContours(1);
		contours->SetValue(0, 0.0);
		auto polydata = vtkSmartPointer<vtkPolyData>::New();
		contours->Update();
		polydata->DeepCopy(contours->GetOutput());
		//polydata->Update();

		auto pbColors = vtkSmartPointer<vtkUnsignedCharArray>::New();
		pbColors->SetName("pbColors");
		pbColors->SetNumberOfComponents(3);

		Vector3r      color = particleColour;
		unsigned char c[3]; //c = {color[0],color[1],color[2]};
		c[0] = (unsigned char)(color[0]);
		c[1] = (unsigned char)(color[1]);
		c[2] = (unsigned char)(color[2]);

		int nbCells = polydata->GetNumberOfPoints();
		for (int i = 0; i < nbCells; i++) {
			pbColors->INSERT_NEXT_TUPLE(c);
		}
		polydata->GetPointData()->SetScalars(pbColors);
		//polydata->Update();


		Vector3r    centre(b->state->pos[0], b->state->pos[1], b->state->pos[2]);
		Quaternionr orientation = b->state->ori;
		orientation.normalize();
		//AngleAxisr aa(orientation); Vector3r axis = aa.axis(); /* axis.normalize(); */ Real angle = aa.angle()/3.14159*180.0;	Real xAxis = axis[0]; Real yAxis = axis[1]; Real zAxis = axis[2];
		auto transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
		transformFilter->SetInputData(polydata);
		auto transform = vtkSmartPointer<vtkTransformReal>::New();

		transformFilter->SetTransform(transform);
		transform->PostMultiply();

		transform->Translate(centre);
		//transform->RotateWXYZ(angle,xAxis, yAxis, zAxis);
		//transformFilter->Update();
		appendFilter->AddInputConnection(transformFilter->GetOutputPort());


		// ################## velocity ####################
		if (REC_VELOCITY == true) {
			vtkIdType pid[1];
			Vector3r  pos(b->state->pos);
			pid[0] = pbPos->InsertNextPoint(pos);
			pbCells->InsertNextCell(1, pid);
			const Vector3r& vel = b->state->vel;
			float           v[3]; //v = { vel[0],vel[1],vel[2] };
			v[0] = float(vel[0]);
			v[1] = float(vel[1]);
			v[2] = float(vel[2]);
			pbLinVelVec->INSERT_NEXT_TUPLE(v);
			pbLinVelLen->InsertNextValue(float(vel.norm()));
			const Vector3r& angVel = b->state->angVel;
			float           av[3]; //av = { angVel[0],angVel[1],angVel[2] };
			av[0] = float(angVel[0]);
			av[1] = float(angVel[1]);
			av[2] = float(angVel[2]);
			pbAngVelVec->INSERT_NEXT_TUPLE(av);
			pbAngVelLen->InsertNextValue(float(angVel.norm()));
			//if(b->state->refPos.squaredNorm()>0.001){ //if initialized
			Vector3r displacement = pos - b->state->refPos;
			float    disp[3];
			disp[0] = float(displacement[0]);
			disp[1] = float(displacement[1]);
			disp[2] = float(displacement[2]);
			pbDisplacementVec->INSERT_NEXT_TUPLE(disp);

			//}
		}
		// ################ velocity ###########################
		polydata->DeleteCells();
		//#if 0
		if (b->isClump() == true) {
			//boolFunction->Delete();
			for (int i = 0; i < ImplicitBoolNo; i++) {
				functionBool[i]->Delete();
			}
			//boolFunction = NULL;
		}
		//#endif
	}

	if (REC_VELOCITY == true) {
		auto pbUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
		pbUg->SetPoints(pbPos);
		pbUg->SetCells(VTK_VERTEX, pbCells);
		pbUg->GetPointData()->AddArray(pbLinVelVec);
		pbUg->GetPointData()->AddArray(pbAngVelVec);
		pbUg->GetPointData()->AddArray(pbLinVelLen);
		pbUg->GetPointData()->AddArray(pbAngVelLen);
		pbUg->GetPointData()->AddArray(pbDisplacementVec);
		auto writerA = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		writerA->SetDataModeToAscii();
		string fv = fileName + "vel." + std::to_string(scene->iter) + ".vtu";
		writerA->SetFileName(fv.c_str());
		writerA->SetInputData(pbUg);
		writerA->Write();
	}

	//###################### bodyId ###############################
	if (REC_ID == true) {
#if 0
			auto writerA = vtkXMLPolyDataWriter::New();
			writerA->SetDataModeToAscii();
			string fn=fileName+"-Id."+std::to_string(scene->iter)+".vtp";
			writerA->SetFileName(fn.c_str());
			writerA->SetInputConnection(appendFilterID->GetOutputPort());//(extrude->GetOutputPort());
			writerA->Write();
	
			writerA->Delete();
#endif
		//#if 0
		auto pbUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
		pbUg->SetPoints(pbPosID);
		pbUg->SetCells(VTK_VERTEX, pbIdCells);
		pbUg->GetPointData()->AddArray(blockId);
		auto writerA = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		writerA->SetDataModeToAscii();
		string fv = fileName + "Id." + std::to_string(scene->iter) + ".vtu";
		writerA->SetFileName(fv.c_str());
		writerA->SetInputData(pbUg);
		writerA->Write();
		//#endif
	}

#if 0
	if(REC_ID== true){
		int counter =0;
		for(const auto & b :  *scene->bodies){
			if (!b) {continue;}
			if (b->isClumpMember()==true) {continue;}
			textArray2[counter]->Delete();
			//txtMapper[count]->Delete();
			extrude[counter]->Delete();
			counter++;
		}
	}

#endif
	// ################## contact point ####################
	if (REC_INTERACTION == true) {
		int count = 0;
		FOREACH(const shared_ptr<Interaction>& I, *scene->interactions)
		{
			if (!I->isReal()) {
				continue;
			}
			const KnKsPBPhys* phys = YADE_CAST<KnKsPBPhys*>(I->phys.get());
			const ScGeom*     geom = YADE_CAST<ScGeom*>(I->geom.get());
			vtkIdType         pid[1];
			Vector3r          pos(geom->contactPoint);
			pid[0] = pbContactPoint->InsertNextPoint(pos);
			pbCellsContact->InsertNextCell(1, pid);
			//intrBodyPos->InsertNextPoint(geom->contactPoint[0],geom->contactPoint[1],geom->contactPoint[2]);
			// gives _signed_ scalar of normal force, following the convention used in the respective constitutive law
			float fn[3]         = { (float)phys->normalForce[0], (float)phys->normalForce[1], (float)phys->normalForce[2] };
			float fs[3]         = { (float)phys->shearForce[0], (float)phys->shearForce[1], (float)phys->shearForce[2] };
			float totalForce[3] = { fn[0] + fs[0], fn[1] + fs[1], fn[2] + fs[2] };
			float totalStress[3]
			        = { 0.0, 0.0, 0.0 }; //{totalForce[0]/phys->contactArea, totalForce[1]/phys->contactArea, totalForce[2]/phys->contactArea};
			float mobilizedShear = float(phys->mobilizedShear);
			pbTotalForce->INSERT_NEXT_TUPLE(totalForce);
			pbMobilizedShear->InsertNextValue(mobilizedShear);
			pbNormalForce->INSERT_NEXT_TUPLE(fn);
			pbShearForce->INSERT_NEXT_TUPLE(fs);
			pbTotalStress->INSERT_NEXT_TUPLE(totalStress);
			count++;
		}
		if (count > 0) {
			auto pbUgCP = vtkSmartPointer<vtkUnstructuredGrid>::New();
			pbUgCP->SetPoints(pbContactPoint);
			pbUgCP->SetCells(VTK_VERTEX, pbCellsContact);
			pbUgCP->GetPointData()->AddArray(pbNormalForce);
			pbUgCP->GetPointData()->AddArray(pbShearForce);
			pbUgCP->GetPointData()->AddArray(pbTotalForce);
			pbUgCP->GetPointData()->AddArray(pbMobilizedShear);
			pbUgCP->GetPointData()->AddArray(pbTotalStress);
			auto writerB = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			writerB->SetDataModeToAscii();
			string fcontact = fileName + "contactPoint." + std::to_string(scene->iter) + ".vtu";
			writerB->SetFileName(fcontact.c_str());
			writerB->SetInputData(pbUgCP);
			writerB->Write();
		}
	}


	// ################ contact point ###########################


	auto writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
	writer->SetDataModeToAscii();
	string fn = fileName + "-pb." + std::to_string(scene->iter) + ".vtp";
	writer->SetFileName(fn.c_str());
	writer->SetInputConnection(appendFilter->GetOutputPort());
	writer->Write();
}

YADE_PLUGIN((PotentialBlockVTKRecorder));

#endif // YADE_VTK

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
