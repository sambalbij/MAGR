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
	const static int rayContainerSize = 101;
	const static int imageSize = 100;
	const static int numberOfRandomRays = 3;			// <-- there will be an  extra added for the centre of the pixel

	BasicGLViewer3D* viewer;
	TriangleMesh *mesh;
	SimpleGLMeshMaterial *renderer;
	ExaminerCameraController* controller;
	tuple<Vector3f, Vector3f> rays[rayContainerSize][rayContainerSize][numberOfRandomRays+1]; //[Heighth][Width][numRays]. 1-3 reserved for random, 4 for center ray.
	Vector3f colours[rayContainerSize][rayContainerSize];
	Vector3f colorOfRays[numberOfRandomRays+1];
	int 	size = imageSize;
public:

	MyMeshExperiment();

	virtual void renderGL();
	void importMesh();
	virtual QWidget *createViewer();

	void getViewerInfo();
	void getRays();
	void shootRays();
	bool checkShadow(tuple<Vector3f, Vector3f>, float);
	int mod(float, int);

	//int32 gridSize;
	//Vector3f incomingRay;                           // <---
	//Vector3f vertex1;
	//Vector3f vertex2;
	//Vector3f vertex3;
	//Matrix3f triangleRefl;                           // <--- declare the parameters you need (will be registered in *.cpp file)

	void calculateDot();
	void calculateMatrixNormal();

	void makeMeshChecker(Vector3f incromingRay, Vector3f triangle[3], float32 &colour);			// <-- Makes colour black or white

	void calculateSurfaceNormal(Vector3f triangle[3], Vector3f &normal);
	void getOutgoingReflection(Vector3f incomingRay, Vector3f triangle[3], Vector3f &outgoingRay);
	void saveImage();




	~MyMeshExperiment();
};


#endif                                         
