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
		ADD_NOARGS_METHOD(MyMeshExperiment::calculateDot)
		ADD_NOARGS_METHOD(MyMeshExperiment::getViewerInfo)             // <--- get viewer info

	ADD_SEPARATOR("Vectors & Matrices")          // 
		ADD_VECTOR3F_PROP(incomingRay, 0)                // <---
		ADD_VECTOR3F_PROP(vector2, 0)                // <---
		ADD_MATRIX3F_PROP(triangleRefl, 0)                 // <---
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

	incomingRay = makeVector3f(1, -1, 0);
	vector2 = makeVector3f(10, 10, 0);
	triangleRefl = makeMatrix3f(0, 1, 0,
		0, 0, 1,
		0, 0, 0);
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
	
	Vector3f tris[3];

	tris[0] = triangleRefl * makeVector3f(1, 0, 0);
	tris[1] = triangleRefl * makeVector3f(0, 1, 0);
	tris[2] = triangleRefl * makeVector3f(0, 0, 1);
	
	output << "\nvertex1: " << tris[0] << "\n"
		<< "vertex2: " << tris[1] << "\n"
		<< "vertex3: " << tris[2] << "\n";

	Vector3f outgoing;
	getOutgoingReflaction(incomingRay, tris, outgoing);

	output << "incoming: " << incomingRay << "\n";
	output << "outgoing: " << outgoing;

	/*getRays();
	shootRays();
	saveImage();*/
}

void MyMeshExperiment::getRays()
{	

	int halfsize = size / 2;
	controller = viewer->getController();
	Vector3f lookat, position, right, up, view; 
	lookat = controller->getCamera()->getLookAt();
	position = controller->getCamera()->getPosition();
	view = lookat - position;
	up = controller->getCamera()->getUp();
	right = up.crossProduct(view);
	up.normalize();
	right.normalize();
	float distance = controller->getDistance();

	for (int dx = -halfsize; dx <= halfsize; dx++)
		for (int dy = -halfsize; dy <= halfsize; dy++)
		{
		Vector3f dr = right * distance * dx / halfsize;
		Vector3f du = up * distance * dy / halfsize;
		Vector3f pixelpos = lookat - dr - du;
		rays[dx + halfsize][dy + halfsize] = tuple<Vector3f, Vector3f>(position, pixelpos - position);

		}
	output << std::get<0>(rays[halfsize][halfsize]) << "  " << std::get<1>(rays[50][50]) << "\n";
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

	/*AAT NRM;
	bool hasNormal = pts->providesAttribute("normal");
	if (hasNormal) NRM = pts->getAAT("normal");
*/
	int32 reflectionDepth = 1;

	//vector < MyTriangle > triangles;

	// loop over rays
	for (int x = 0; x <= size; x++)
	{
		for (int y = 0; y <= size; y++)
		{
			tuple<Vector3f, Vector3f> ray = rays[x][y];
			float distance;											// <-- scalar multiplied with ray direction is distance
			float minDistance = 99999;								// <-- used to find nearest triangle
			int minDistanceId = -1;									// <-- check for background
			Vector3f bestColour;

			// loop over triangle
			const card32 numTri = idx->getNumEntries();
			for (card32 i = 0; i < numTri; i++) {
				Vector3i tind = idx->get<int32, 3>(i, IDX);			// <-- triangle index of vertices (comes from triangle dynamic thing)
				Vector3f pos[3];									// <-- position of all vertices of the triangle
				pos[0] = pts->get<float32, 3>(tind[0], POS);
				pos[1] = pts->get<float32, 3>(tind[1], POS);
				pos[2] = pts->get<float32, 3>(tind[2], POS);


				// get intersection + distance
				bool xt = tri->getIntersection(std::get<0>(ray), std::get<1>(ray), pos, distance);		// <-- distance is a result
				if (xt)
					if (distance < minDistance)						// <-- check if closest
				{
					minDistance = distance;							// <-- set current triangle as closest
					bool shadow = checkShadow(ray, distance);		// <-- check if there are triangles blocking the light
					
					minDistanceId = i;								// <-- a check to see if current current triangle is closer than existing
					Vector3f colour[3];
					colour[0] = pts->get<float32, 3>(tind[0], COL);
					colour[1] = pts->get<float32, 3>(tind[1], COL);
					colour[2] = pts->get<float32, 3>(tind[2], COL);


					// REFLECTION CALCULATION
					// Needed point, normal of point


					if (!shadow)
						bestColour = ( colour[0] + colour[1] + colour[2] ) / 3;
					else
					{
						bestColour = (colour[0] + colour[1] + colour[2] ) / 0.9;
						//bestColour = (colour[0] / 3 + colour[1] / 3 + colour[2] / 3)*0.3;
					}
				}
			}
			if (minDistanceId == -1) // <-- no collision so no colour
			{
				bestColour[0] = 0;
				bestColour[1] = 0;
				bestColour[2] = 0;
			}
			colours[x][y] = bestColour;
		}//rays
	}
	delete tri;
}

void MyMeshExperiment::calculateDot()
{
	output << "dot: " << incomingRay * vector2 << "\n";
}
void MyMeshExperiment::getOutgoingReflaction(Vector3f incomingRay, Vector3f triangle[3], Vector3f &outgoingRay)
{
	Vector3f t = ((triangle[2] - triangle[0]).crossProduct(triangle[1] - triangle[0]));
	t = t / norm(t);
	Vector3f d = incomingRay.componentProduct(t);
	float dotProd = d[0] + d[1] + d[2];
	outgoingRay = (incomingRay.operator*(2).operator*(dotProd).operator-(incomingRay));
	/*Vector3f t = ((triangle[2] - triangle[0]).crossProduct(triangle[1] - triangle[0]));	
	t = t/norm(t);
	outgoingRay = (rayDirection - (t * 2 ) * (rayDirection * t));*/
	
}

bool MyMeshExperiment::checkShadow(tuple<Vector3f, Vector3f> ray, float distance)
{
	Vector3f hit = (std::get<0>(ray) + std::get<1>(ray)*distance);				// <-- location of where ray hit triangle 
	Vector3f light = makeVector3f(0, 0, 10) - hit;								// <-- intersection to lightsource vector
	float newDistance;
	TriangleRayIntersection* tri = new TriangleRayIntersection();

	DynamicArrayOfStructures *idx = mesh->getTriangles();
	if (!idx->providesAttribute("index"));
	AAT IDX = idx->getAAT("index");

	if (mesh == NULL);
	DynamicArrayOfStructures *pts = mesh->getVertices();
	if (!pts->providesAttribute("position"));
	AAT POS = pts->getAAT("position");

	// loop over triangle
	const card32 numTri = idx->getNumEntries();
	for (card32 i = 0; i < numTri; i++) 
	{
		Vector3i tind = idx->get<int32, 3>(i, IDX);
		Vector3f pos[3];
		pos[0] = pts->get<float32, 3>(tind[0], POS);
		pos[1] = pts->get<float32, 3>(tind[1], POS);
		pos[2] = pts->get<float32, 3>(tind[2], POS);

		bool intersect = tri->getIntersection(hit+light*0.01, light, pos, newDistance);				// <-- hit+light*0.01 prevents shadow dots 
		if (intersect && newDistance <= 1)		// && newDistance > 0.01)
			return true;		//There is shadow.
	}
	return false;
}

void MyMeshExperiment::saveImage()
{
	QImage image(size + 1, size+1, QImage::Format_RGB32);
	for (int x = 0; x <= size; x++)
	{
		for (int y = 0; y <= size; y++)
		{
			image.setPixel(x, y, qRgb(int(colours[x][y][0] * 255), int(colours[x][y][1] * 255), int(colours[x][y][2] * 255)));
		}
	}
	image.save(QString("test.png"));
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