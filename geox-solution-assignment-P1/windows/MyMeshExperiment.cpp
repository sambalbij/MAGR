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
		//ADD_NOARGS_METHOD(MyMeshExperiment::calculateDot)
		//ADD_NOARGS_METHOD(MyMeshExperiment::calculateMatrixNormal)
		ADD_NOARGS_METHOD(MyMeshExperiment::getViewerInfo)             // <--- get viewer info

	//ADD_SEPARATOR("Vectors & Matrices")          // 
		//ADD_INT32_PROP(gridSize, 0)
		//ADD_VECTOR3F_PROP(incomingRay, 0)                // <---
		//ADD_VECTOR3F_PROP(vertex1, 0)                // <---
		//ADD_VECTOR3F_PROP(vertex2, 0)                // <---
		//ADD_VECTOR3F_PROP(vertex3, 0)                // <---
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

	//gridSize = -1;
	//incomingRay = makeVector3f(0, 1, -1);
	//vertex1 = makeVector3f(0, 0, 0);
	//vertex2 = makeVector3f(0, 1, 0);
	//vertex3 = makeVector3f(1, 0, 0);
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

	output	<< "position: " << controller->getCamera()->getPosition()
		<< " lookat: " << controller->getCamera()->getLookAt()
		<< " distance: " << controller->getDistance()
		<< "\n";     
	
	//Vector3f tris[3] = {vertex1, vertex2, vertex3};

	//
	//output << "\n (x, y, z)" << "\n";
	//output << "vertex1: " << tris[0] << "\n"
	//	<< "vertex2: " << tris[1] << "\n"
	//	<< "vertex3: " << tris[2] << "\n";

	//Vector3f outgoing;
	//getOutgoingReflection(incomingRay, tris, outgoing);

	//output << "incoming: " << incomingRay << "\n";
	//output << "outgoing: " << outgoing << "\n";

	getRays();
	shootRays();
	saveImage();
	output << "\ndone\n";
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

	// Center Ray.
	/*for (int dx = -halfsize; dx <= halfsize; dx++)
		for (int dy = -halfsize; dy <= halfsize; dy++)
		{
			Vector3f dr = right * distance * dx / halfsize;
			Vector3f du = up * distance * dy / halfsize;
			Vector3f pixelpos = lookat - dr - du;
			rays[dx + halfsize][dy + halfsize][0] = tuple<Vector3f, Vector3f>(position, pixelpos - position);
		}*/

	// Random Ray.
	Vector3f dr, du, pixelpos, newPixelpos, pixeldr, pixeldu, randomdr, randomdu;
	float rndR, rndU;

	for (int dx = -halfsize; dx <= halfsize; dx++)
		for (int dy = -halfsize; dy <= halfsize; dy++)
		{
			dr = right * distance * dx / halfsize;
			du = up * distance * dy / halfsize;
			pixelpos = lookat - dr - du;
			newPixelpos;
			pixeldr = right * distance / halfsize;
			pixeldu = up * distance / halfsize;

			// Random rays.
			for (int i = 0; i < numberOfRandomRays; ++i)
				{
					rndR = -0.5 + static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 0.5);
					rndU = -0.5 + static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 0.5);
					randomdr = pixeldr.operator*(rndR);
					randomdu = pixeldu.operator*(rndU);
					newPixelpos = pixelpos+randomdr+randomdu;
					rays[dx + halfsize][dy + halfsize][i] = tuple<Vector3f, Vector3f>(position, newPixelpos - position);
				}

			// Center Ray
			rays[dx + halfsize][dy + halfsize][3] = tuple<Vector3f, Vector3f>(position, pixelpos - position);
		}
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
	bool reflectionDepth = true;

	//vector < MyTriangle > triangles;


	int allSubRays = numberOfRandomRays + 1;
	// loop over rays
	for (int x = 0; x <= size; x++)
	{
		for (int y = 0; y <= size; y++)
		{
			for (int r = 0; r < allSubRays; ++r)
			{
				tuple<Vector3f, Vector3f> ray = rays[x][y][r];
				float distance;											// <-- scalar multiplied with ray direction is distance
				float minDistance = 99999;								// <-- used to find nearest triangle
				int minDistanceId = -1;									// <-- check for background
				Vector3f bestColour, bestColour2;;
				Vector3f outgoingRay;
				bool thereWasARefelction = false;
				bool thereIsAReflection = false;

				// loop over triangle
				const card32 numTri = idx->getNumEntries();
				for (card32 i = 0; i < numTri; i++)
				{
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
						getOutgoingReflection(std::get<1>(ray), pos, outgoingRay); // get outgoing ray for reflection

						minDistance = distance;							// <-- set current triangle as closest
						bool shadow = checkShadow(ray, distance);		// <-- check if there are triangles blocking the light

						minDistanceId = i;								// <-- a check to see if current current triangle is closer than existing						

						//Vector3f colour[3];

						Vector3f hit = (std::get<0>(ray) +std::get<1>(ray)*distance);	// <-- location of where ray hit triangle 
						hits[x][y][r] = hit;							// <-- used when using gausian AA
						if (mod(hit[0] / 5, 2) == mod(hit[1] / 5, 2))
						{
							bestColour = makeVector3f(1, 0, 0);
						}
						else
						{
							//colour[0] = pts->get<float32, 3>(tind[0], COL);
							//colour[1] = pts->get<float32, 3>(tind[1], COL);
							//colour[2] = pts->get<float32, 3>(tind[2], COL);

							//if (!shadow)
							bestColour = makeVector3f(1, 1, 1);// (colour[0] + colour[1] + colour[2]) / 3;
							//else
							//	bestColour = (colour[0] + colour[1] + colour[2]) / 9;
						}
						}
				}
				if (minDistanceId == -1) // <-- no collision so no colour
				{
					hits[x][y][r] = makeVector3f(0, 0, -3000);
					bestColour[0] = 0;
					bestColour[1] = 0;
					bestColour[2] = 0;
				}
				else // do reflection
				{
					// REFLECTION CALCULATION
					float distance2;											// <-- scalar multiplied with ray direction is distance
					float minDistance2 = 99999;								// <-- used to find nearest triangle
					int minDistanceId2 = -1;									// <-- check for background
					Vector3f bestColour2;



					Vector3f hit = std::get<0>(ray) +std::get<1>(ray)*minDistance;
					thereWasARefelction = false;
					for (card32 j = 0; j < numTri; j++) {
						Vector3i tind2 = idx->get<int32, 3>(j, IDX);			// <-- triangle index of vertices (comes from triangle dynamic thing)
						Vector3f pos2[3];									// <-- position of all vertices of the triangle
						pos2[0] = pts->get<float32, 3>(tind2[0], POS);
						pos2[1] = pts->get<float32, 3>(tind2[1], POS);
						pos2[2] = pts->get<float32, 3>(tind2[2], POS);


						// get intersection + distance
						thereIsAReflection = tri->getIntersection(hit + outgoingRay*0.02, outgoingRay, pos2, distance2);		// <-- distance is a result
						if (thereIsAReflection)
							if (distance2 < minDistance2)						// <-- check if closest
							{
							thereWasARefelction = true;
							minDistance2 = distance2;							// <-- set current triangle as closest
							//bool shadow2 = checkShadow(tuple<Vector3f, Vector3f>(std::get<1>(ray)*distance, outgoingRay), distance2);		// <-- check if there are triangles blocking the light

							minDistanceId2 = j;								// <-- a check to see if current current triangle is closer than existing
							Vector3f colour2[3];
							colour2[0] = pts->get<float32, 3>(tind2[0], COL);
							colour2[1] = pts->get<float32, 3>(tind2[1], COL);
							colour2[2] = pts->get<float32, 3>(tind2[2], COL);

							bestColour2 = makeVector3f(1, 0, 0);//(colour2[0] + colour2[1] + colour2[2]) / 3;
							}
					}
					if (minDistanceId2 == -1) // <-- no collision on the reflected ray so no colour
					{
						bestColour2[0] = 0;
						bestColour2[1] = 0;
						bestColour2[2] = 0;
					}
					//colours[x][y] = bestColour2;
				}
				if (!thereWasARefelction)
					binnedCoulours[x][y][r] = bestColour;// <-- used for gausians
					//colorOfRays[r] = bestColour; // <-- used for random rays
					//colours[x][y] = bestColour; // <-- used for no AA
				else
					binnedCoulours[x][y][r] = bestColour; // <-- used for gausians
					//colorOfRays[r] = bestColour*0.5 + bestColour2 * 0.5; // <-- used fo random rays
					//colours[x][y] = bestColour*0.5 + bestColour2 * 0.5; //<-- used for no AA
			}
			 //colours[x][y] = colorOfRays[3]; // Center ray color. 
			 colours[x][y] = (colorOfRays[0] + colorOfRays[1] + colorOfRays[2]) / 3;	// Average of random rays colors.
		}//rays
	}
	// now we have all samples we can do gausians 
	for (int x = 0; x <= size; x++) // (COMMENT AWAY WHEN NOT USING GAUSIANS!!!)
		for (int y = 0; y <= size; y++) // (COMMENT AWAY WHEN NOT USING GAUSIANS!!!)
			colours[x][y] = GetGausianColor(x, y); // (COMMENT AWAY WHEN NOT USING GAUSIANS!!!)
	delete tri;
}

Vector3f MyMeshExperiment::GetGausianColor(int x, int y)
{
	float sigma = 4;
	float totalWeight =0;
	Vector3f colour = makeVector3f(0,0,0);
	float weight = 0;
	for (int dx = -1; dx < 2; dx++)
		for (int dy = -1; dy < 2; dy++) // loop over 9 pixels
			for (int i = 0; i <= numberOfRandomRays; i++){ // and over all sampels withing the pixels
				int finalX = x + dx;
				int finalY = y + dy;
				if (finalX > size || finalY>size || finalY < 0 || finalX < 0) // out of bounds prevention
					continue;
				if (hits[x][y][i] == makeVector3f(0, 0, -3000))//ray did not hit
					continue;
				float distancex = hits[x][y][i][0] - hits[x][y][numberOfRandomRays][0];
				float distancey = hits[x][y][i][1] - hits[x][y][numberOfRandomRays][1];
				float distanceToSample = distancex*distancex + distancey*distancey;
				weight = exp((-1.0f / (2.0f * sigma*sigma))*distanceToSample);
				totalWeight += weight;
				colour+=binnedCoulours[x][y][i]* weight;
			}
	return colour / totalWeight;

}
int MyMeshExperiment::mod(float f, int i)
{
 if (f >= 0)
  return (int)f%i;
 else
  return 1 - (((int)f%i + i) % i);
}

//void MyMeshExperiment::shootRays()
//{
//	TriangleRayIntersection* tri = new TriangleRayIntersection();
//
//	if (mesh == NULL) return;
//	DynamicArrayOfStructures *pts = mesh->getVertices();
//	if (!pts->providesAttribute("position")) return;
//	AAT POS = pts->getAAT("position");
//
//	AAT COL;
//	bool hasCol = pts->providesAttribute("color");
//	if (hasCol) COL = pts->getAAT("color");
//
//	DynamicArrayOfStructures *idx = mesh->getTriangles();
//	if (!idx->providesAttribute("index")) return;
//	AAT IDX = idx->getAAT("index");
//
//	/*AAT NRM;
//	bool hasNormal = pts->providesAttribute("normal");
//	if (hasNormal) NRM = pts->getAAT("normal");
//*/
//	bool reflectionDepth = true;
//
//	//vector < MyTriangle > triangles;
//
//	// loop over rays
//	for (int x = 0; x <= size; x++)
//	{
//		for (int y = 0; y <= size; y++)
//		{
//			tuple<Vector3f, Vector3f> ray = rays[x][y];
//			float distance;											// <-- scalar multiplied with ray direction is distance
//			float minDistance = 99999;								// <-- used to find nearest triangle
//			int minDistanceId = -1;									// <-- check for background
//			Vector3f bestColour;
//
//			bool thereIsAReflection = false;
//			// loop over triangle
//			const card32 numTri = idx->getNumEntries();
//			for (card32 i = 0; i < numTri; i++) {
//				Vector3i tind = idx->get<int32, 3>(i, IDX);			// <-- triangle index of vertices (comes from triangle dynamic thing)
//				Vector3f pos[3];									// <-- position of all vertices of the triangle
//				pos[0] = pts->get<float32, 3>(tind[0], POS);
//				pos[1] = pts->get<float32, 3>(tind[1], POS);
//				pos[2] = pts->get<float32, 3>(tind[2], POS);
//
//
//				// get intersection + distance
//				bool xt = tri->getIntersection(std::get<0>(ray), std::get<1>(ray), pos, distance);		// <-- distance is a result
//				if (xt)
//					if (distance < minDistance)						// <-- check if closest
//				{
//					minDistance = distance;							// <-- set current triangle as closest
//					bool shadow = checkShadow(ray, distance);		// <-- check if there are triangles blocking the light
//					
//					minDistanceId = i;								// <-- a check to see if current current triangle is closer than existing
//					Vector3f colour[3];
//					colour[0] = pts->get<float32, 3>(tind[0], COL);
//					colour[1] = pts->get<float32, 3>(tind[1], COL);
//					colour[2] = pts->get<float32, 3>(tind[2], COL);
//
//
//					// REFLECTION CALCULATION
//					if (reflectionDepth)
//					{
//						float distance2;											// <-- scalar multiplied with ray direction is distance
//						float minDistance2 = 99999;								// <-- used to find nearest triangle
//						int minDistanceId2 = -1;									// <-- check for background
//						Vector3f bestColour2;
//
//
//						Vector3f outgoingRay;
//						getOutgoingReflection(std::get<1>(ray), pos, outgoingRay);
//
//						for (card32 j = 0; j < numTri; j++) {
//							Vector3i tind2 = idx->get<int32, 3>(j, IDX);			// <-- triangle index of vertices (comes from triangle dynamic thing)
//							Vector3f pos2[3];									// <-- position of all vertices of the triangle
//							pos2[0] = pts->get<float32, 3>(tind2[0], POS);
//							pos2[1] = pts->get<float32, 3>(tind2[1], POS);
//							pos2[2] = pts->get<float32, 3>(tind2[2], POS);
//
//
//							// get intersection + distance
//							thereIsAReflection = tri->getIntersection(std::get<1>(ray)*distance, outgoingRay, pos2, distance2);		// <-- distance is a result
//							if (thereIsAReflection)
//								if (distance2 < minDistance2)						// <-- check if closest
//								{
//									minDistance2 = distance2;							// <-- set current triangle as closest
//									//bool shadow2 = checkShadow(tuple<Vector3f, Vector3f>(std::get<1>(ray)*distance, outgoingRay), distance2);		// <-- check if there are triangles blocking the light
//
//									minDistanceId2 = j;								// <-- a check to see if current current triangle is closer than existing
//									Vector3f colour2[3];
//									colour2[0] = pts->get<float32, 3>(tind2[0], COL);
//									colour2[1] = pts->get<float32, 3>(tind2[1], COL);
//									colour2[2] = pts->get<float32, 3>(tind2[2], COL);
//
//									bestColour2 = (colour2[0] + colour2[1] + colour2[2]) / 3;
//								}
//						}
//						if (minDistanceId2 == -1) // <-- no collision so no colour
//						{
//							bestColour2[0] = 0;
//							bestColour2[1] = 0;
//							bestColour2[2] = 0;
//						}
//						colours[x][y] = bestColour2;
//					}
//					if (!shadow)
//						bestColour = ( colour[0] + colour[1] + colour[2] ) / 3;
//					else
//					{
//						bestColour = (colour[0] + colour[1] + colour[2] ) / 0.9;
//						//bestColour = (colour[0] / 3 + colour[1] / 3 + colour[2] / 3)*0.3;
//					}
//				}
//			}
//			if (minDistanceId == -1) // <-- no collision so no colour
//			{
//				bestColour[0] = 0;
//				bestColour[1] = 0;
//				bestColour[2] = 0;
//			}
//			if (!reflectionDepth || !thereIsAReflection)
//				colours[x][y] = bestColour;
//			else
//				colours[x][y] = bestColour*0.7 + colours[x][y] * 0.3;
//		}//rays
//	}
//	delete tri;
//}

//void MyMeshExperiment::calculateDot()
//{
//	output << "dot: " << incomingRay * vertex1 << "\n";
//}

//void MyMeshExperiment::calculateMatrixNormal()
//{
//	Vector3f normal;
//	Vector3f tris[3] = { vertex1, vertex2, vertex3 };
//
//	//tris[0] = triangleRefl * makeVector3f(1, 0, 0);
//	//tris[1] = triangleRefl * makeVector3f(0, 1, 0);
//	//tris[2] = triangleRefl * makeVector3f(0, 0, 1);
//
//	calculateSurfaceNormal(tris, normal);
//
//	output << "SurfaceNormal: " << normal << "\n";
//}

void MyMeshExperiment::makeMeshChecker(Vector3f incomingRay, Vector3f triangle[3], float32 &colour)
{
	
}

void MyMeshExperiment::calculateSurfaceNormal(Vector3f triangle[3], Vector3f &normal)
{
	Vector3f t = ((triangle[2] - triangle[0]).crossProduct(triangle[1] - triangle[0]));
	normal = t / norm(t);
}

void MyMeshExperiment::getOutgoingReflection(Vector3f incomingRay, Vector3f triangle[3], Vector3f &outgoingRay)
{
	Vector3f normal;
	calculateSurfaceNormal(triangle, normal);

	Vector3f ri = incomingRay;
	Vector3f d = ri.componentProduct(normal);
	float dotProd = d[0] + d[1] + d[2];
	outgoingRay = ri - (normal.operator*(2)).operator*(dotProd);

	/*Vector3f ri = incomingRay.operator*(-1);
	Vector3f d = ri.componentProduct(normal);
	float dotProd = d[0] + d[1] + d[2];
	outgoingRay = (ri.operator*(2).operator*(dotProd).operator-(ri));*/

	/*Vector3f incomingRayNormalized = incomingRay / norm(incomingRay);

	outgoingRay = normal * ((normal * incomingRayNormalized) * 2) - incomingRayNormalized;*/
	
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