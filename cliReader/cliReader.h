#pragma once

// Vertex: x,y
struct Vertex
{
	float px;
	float py;
};

// Polyline: 
// id
// direction - 0,1,2
// number of points
// Vertex[]
// Computed area -> (-, +, 0) for dir (0,1,2) respectively
class Polyline
{
public:
	int id;
	int dir;
	int n;
	Vertex* vertices;

	float area() const;

	Polyline(){}

	Polyline(const Polyline&);
	
	~Polyline()
	{
		if (vertices != nullptr)
		{
			delete[] vertices;
			vertices = nullptr;
		}
	}
};

// Layer:
// index
// height
// total area
struct Layer
{
	int index;
	float height;
	float layer_area;

	Layer()
	{
		index = -1;
		height = -1.0;
		layer_area = 0.0;
	}

	void print_layer_data() const;
};



