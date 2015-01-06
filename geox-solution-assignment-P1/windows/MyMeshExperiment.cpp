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
#include "TriangleRayIntersection.h"

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
	shootRays();
}

void MyMeshExperiment::getRays()
{	
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
	output << std::get<0>(rays[50][50]) << "  " << std::get<1>(rays[50][50])<<"\n";
}

void MyMeshExperiment::shootRays()
{
	TriangleRayIntersection* tri = new TriangleRayIntersection();

	if (mesh == NULL) return;
	DynamicArrayOfStructures *pts = mesh->getVertices();
	if (!pts->providesAttribute("position")) return;
	AAT POS = pts->getAAT("position");

	AAT COL;
	bool hasCol = pts->providesAttribute("color");
	if (hasCol) COL = pts->getAAT("color");

	DynamicArrayOfStructures *idx = mesh->getTriangles();
	if (!idx->providesAttribute("index")) return;
	AAT IDX = idx->getAAT("index");

	//vector < MyTriangle > triangles;

	// loop over rays
	for (int x = 0; x <= 100; x++)
	{
		for (int y = 0; y <= 100; y++)
		{
			tuple<Vector3f, Vector3f> ray = rays[x][y];
			float distance;
			float minDistance = 99999;
			int minDistanceId = -1;
			Vector3f bestColour;

			// loop over triangle
			const card32 numTri = idx->getNumEntries();
			for (card32 i = 0; i < numTri; i++) {
				Vector3i tind = idx->get<int32, 3>(i, IDX);
				Vector3f pos[3];
				pos[0] = pts->get<float32, 3>(tind[0], POS);
				pos[1] = pts->get<float32, 3>(tind[1], POS);
				pos[2] = pts->get<float32, 3>(tind[2], POS);


				// get intersection + distance
				bool xt = tri->getIntersection(std::get<0>(ray), std::get<1>(ray), pos, distance);
				if (xt)
					if (distance < minDistance)
				{
					minDistanceId = i; Vector3f colour[3];
					colour[0] = pts->get<float32, 3>(tind[0], COL);
					colour[1] = pts->get<float32, 3>(tind[1], COL);
					colour[2] = pts->get<float32, 3>(tind[2], COL);
					bestColour = colour[0] / 3 + colour[1] / 3 + colour[2] / 3;
				}
			}
			if (minDistanceId == -1)
			{
				bestColour[0] = 0;
				bestColour[1] = 0;
				bestColour[2] = 0;
			}
			colours[x][y] = bestColour;
		}//rays
	}
}
/*
MyTriangle t;
t.pos[0] = pos[0];
t.pos[1] = pos[1];
t.pos[2] = pos[2];\

triangles.push_back(t);

struct MyTriangle
{
Vector3f pos[3];

};
*/