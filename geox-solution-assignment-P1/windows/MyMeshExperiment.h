//---------------------------------------------------------------------------
#ifndef MyMeshExperimentH
#define MyMeshExperimentH
//---------------------------------------------------------------------------
#include "Experiment.h"
#include "LinearAlgebra.h"
#include "BasicGLViewer3D.h"
#include "BasicGLViewer3D.h"
//---------------------------------------------------------------------------
#include "Camera.h"


class TriangleMesh;
class SimpleGLMeshMaterial;

///
/// This is an example experiment that demonstrates importing and rendering triangle meshes.
///
class MyMeshExperiment: public Experiment, public RenderableObject {
	GEOX_CLASS(MyMeshExperiment)
private:
	const static int rayContainerSize = 201;
	const static int imageSize = 200;
	const static int numberOfRandomRays = 3;			// <-- there will be an  extra added for the centre of the pixel
	const static int gridSize = 3;						// <-- SHOULD ONLY BE UNEVEN
	const static int gridSpacer = (gridSize - 1) / 2;
	const static int gridSizeSquared = gridSize * gridSize;

	bool randomIsOn = false;

	BasicGLViewer3D* viewer;
	TriangleMesh *mesh;
	SimpleGLMeshMaterial *renderer;
	ExaminerCameraController* controller;
	Camera *camera;
	
	tuple<Vector3f, Vector3f> rays[rayContainerSize][rayContainerSize][numberOfRandomRays+1]; //[Heighth][Width][numRays]. 1-#rand reserved for random, last for center ray.
	tuple<Vector3f, Vector3f> raysGridded[rayContainerSize][rayContainerSize][gridSizeSquared+1]; //[Heighth][Width][numRays]. 1-gridSq reserved for random, last for center ray.
	
	Vector3f colours[rayContainerSize][rayContainerSize];
	Vector3f colourOfRays[numberOfRandomRays+1];
	Vector3f gridRays[gridSizeSquared+1];
	Vector3f hits[rayContainerSize][rayContainerSize][numberOfRandomRays + 1];
	Vector3f binnedCoulours[rayContainerSize][rayContainerSize][numberOfRandomRays + 1];
	
	int 	size = imageSize;

public:

	MyMeshExperiment();

	virtual void renderGL();
	void importMesh();
	virtual QWidget *createViewer();

	void setCurrentCameraPosition();
	void getCurrentCameraInfo();

	void getViewerInfo();
	void getRays();
	void shootRays();
	bool checkShadow(tuple<Vector3f, Vector3f>, float);

	void calculateDot();
	void calculateMatrixNormal();
	int mod(float, int);

	void calculateSurfaceNormal(Vector3f triangle[3], Vector3f &normal);
	void getOutgoingReflection(Vector3f incomingRay, Vector3f triangle[3], Vector3f &outgoingRay);
	void saveImage();
	
	Vector3f getGausianColour(int x, int y);
	Vector3f getAverageSubPixels(Vector3f rays[], int arraySize);


	~MyMeshExperiment();
};


#endif                                         
