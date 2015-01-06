//---------------------------------------------------------------------------
#include "stdafx.h"
//---------------------------------------------------------------------------
#include "MyMeshExperiment.h"
//---------------------------------------------------------------------------
#include "Properties.h"
#include "GLGeometryViewer3D.h"
#include "GeoXOutput.h"   
#include "MeshImporterSMFOBJ.h"
#include "SimpleGLMeshMaterial.h"
#include "TriangleMesh.h"
#include "ObjectViewsTable.h"
//---------------------------------------------------------------------------
#include "GeoXOutput.h"                         // <--- this include lets you output text in the main window
#include "Camera.h"

IMPLEMENT_GEOX_CLASS(MyMeshExperiment, 0)
{
	BEGIN_CLASS_INIT(MyMeshExperiment);
	ADD_OBJECT_PROP(mesh, 0, TriangleMesh::getClass(), true)
		ADD_OBJECT_PROP(renderer, 0, SimpleGLMeshMaterial::getClass(), true)
		ADD_NOARGS_METHOD(MyMeshExperiment::importMesh)
		ADD_NOARGS_METHOD(MyMeshExperiment::getViewerInfo)             // <--- get viewer info
}

QWidget *MyMeshExperiment::createViewer() {
	viewer = new BasicGLViewer3D();
	viewer->addRenderCallback(this);
	return viewer;
}

MyMeshExperiment::MyMeshExperiment()
{
	viewer = NULL;
	mesh = NULL;
	renderer = new SimpleGLMeshMaterial();
}

MyMeshExperiment::~MyMeshExperiment()
{
	delete renderer;
}



void MyMeshExperiment::importMesh()
{

	QString filename = QFileDialog::getOpenFileName(viewer, "Open OBJ File...", QString(), "3D Objects (*.obj *.smf)", 0, 0);
	if (filename != "") {
		delete mesh;
		mesh = NULL;
		MeshImporterSMFOBJ importer;
		importer.performImport(qString2STLString(filename));
		mesh = importer.createTriangleMesh();
	}

	// display changes
	ObjectViewsTable::update(this);
	viewer->refresh();
}

void MyMeshExperiment::renderGL()
{
	if (renderer && mesh) {
		renderer->draw(mesh);
	}
}

void MyMeshExperiment::getViewerInfo() {
	controller = viewer->getController();
	//int *left, *top, *bottom, *right;
	//viewer->getContentsMargins(left,top, right, bottom);
	output //<< "vieweroutput:"
		//<< "height: " << *top << "\n"
		//<< "width: " << *right
		<< "position: " << controller->getCamera()->getPosition()
		<< " lookat: " << controller->getCamera()->getLookAt()
		<< " distance: " << controller->getDistance()
		<< "\n";     
	getRays();
}

void MyMeshExperiment::getRays()
{	
	tuple<Vector3f, Vector3f> rays [101][101];
	controller = viewer->getController();
	Vector3f lookat, position, right, up, view; 
	lookat = controller->getCamera()->getLookAt();
	position = controller->getCamera()->getPosition();
	view = lookat - position;
	up = controller->getCamera()->getUp();
	right = up.crossProduct(view);
	float distance = controller->getDistance();

	for (int dx = -50; dx <= 50;dx++)
		for (int dy = -50; dy <= 50; dy++)
		{
		Vector3f dr = right * distance * dx / 50;
		Vector3f du = up * distance * dy / 50;
		Vector3f pixelpos = lookat + dr + du;
		rays[dx+50][dy+50] = tuple<Vector3f, Vector3f>(position, pixelpos - position);

		}
	//tuple<Vector3f, Vector3f> ray = rays[50][50];
}
