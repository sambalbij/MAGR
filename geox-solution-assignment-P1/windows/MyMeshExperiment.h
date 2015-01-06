//---------------------------------------------------------------------------
#ifndef MyMeshExperimentH
#define MyMeshExperimentH
//---------------------------------------------------------------------------
#include "Experiment.h"
#include "LinearAlgebra.h"
#include "BasicGLViewer3D.h"
#include "BasicGLViewer3D.h"
//---------------------------------------------------------------------------


class TriangleMesh;
class SimpleGLMeshMaterial;

///
/// This is an example experiment that demonstrates importing and rendering triangle meshes.
///
class MyMeshExperiment: public Experiment, public RenderableObject {
	GEOX_CLASS(MyMeshExperiment)
private:
	BasicGLViewer3D* viewer;
	TriangleMesh *mesh;
	SimpleGLMeshMaterial *renderer;
	ExaminerCameraController* controller;
public:

	MyMeshExperiment();

	virtual void renderGL();
	void importMesh();
	virtual QWidget *createViewer();

	void getViewerInfo();
	void getRays();


	~MyMeshExperiment();
};


#endif                                         
