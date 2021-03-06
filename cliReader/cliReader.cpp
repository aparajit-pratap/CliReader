// cliReader.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "cliReader.h"
#include <ratio>

using namespace std;

Polyline::Polyline(const Polyline& p)
{
	id = p.id;
	dir = p.dir;
	n = p.n;
	vertices = new Vertex[n];
	int i = 0;
	for (auto v = p.vertices; i < n; v++)
	{
		vertices[i].px = v->px;
		vertices[i].py = v->py;
		i++;
	}
}

float Polyline::area() const
{
	float signed_area = 0;
	if (n == 0)
		return 0;

	// sum of cross-product of vectors along sides of polygon
	auto v1 = vertices[0];
	for (int i = 0; i < n; ++i) {
		const Vertex v2 = vertices[(i + 1) % n];
		signed_area += v1.px * v2.py - v1.py * v2.px;
		v1 = v2;
	}
	return fabs(signed_area * 0.5);
}

void Layer::print_layer_data() const
{
	if (index >= 0)
	{
		if(layer_area < 0)
		{
			cout << layer_area << endl;
		}
		cout << "Layer index: " << index << ", Layer height: " << height
			<< ", Layer part area: " << layer_area << endl << endl;
	}
}

Polyline output_polyline(string line, int pos)
{
	Polyline p;
	auto params = line.substr(pos + 1);

	pos = params.find(',');
	auto param = params.substr(0, pos);
	p.id = stoi(param);

	params = params.substr(pos + 1);
	pos = params.find(','); 
	param = params.substr(0, pos);
	p.dir = stoi(param);

	params = params.substr(pos + 1);
	pos = params.find(',');
	param = params.substr(0, pos);
	p.n = stoi(param);

	p.vertices = new Vertex[p.n];

	// Loop over vertices of polyline:
	params = params.substr(pos + 1);
	int vcount = 0;
	pos = params.find(',');
	do
	{
		Vertex v;

		param = params.substr(0, pos);
		v.px = stof(param);

		params = params.substr(pos + 1);
		pos = params.find(',');
		param = params.substr(0, pos);
		v.py = stof(param);

		p.vertices[vcount] = v;
		vcount++;
		pos = params.find(',');
		params = params.substr(pos + 1);
	} while (pos != string::npos);

	return p;
}

Polyline output_polyline(ifstream& cli_file)
{
	Polyline polyline;

	char buf[4];
	cli_file.read(buf, 4);
	polyline.id = *reinterpret_cast<int*>(buf);

	cli_file.read(buf, 4);
	polyline.dir = *reinterpret_cast<int*>(buf);

	cli_file.read(buf, 4);
	polyline.n = *reinterpret_cast<int*>(buf);

	polyline.vertices = new Vertex[polyline.n];
	for (int i = 0; i < polyline.n; i++)
	{
		Vertex v;

		cli_file.read(buf, 4);
		v.px = *reinterpret_cast<float*>(buf);

		cli_file.read(buf, 4);
		v.py = *reinterpret_cast<float*>(buf);

		polyline.vertices[i] = v;
	}
	return polyline;
}

void read_binary(ifstream& cli_file, streampos size_of_file)
{
	// move read pointer to end of HEADER
	auto startpos = cli_file.tellg();
	char hdr_end[12];
	cli_file.get(hdr_end, sizeof(hdr_end));
	while (strcmp(hdr_end, "$$HEADEREND") != 0)
	{
		cli_file.clear();
		cli_file.seekg(startpos, ios::beg);

		string temp;
		cli_file >> temp;
		startpos = cli_file.tellg();
		cli_file.get();
		cli_file.get(hdr_end, sizeof(hdr_end));
	}

	int index = 0;
	Layer layer;

	// begin geometry section
	while (cli_file.tellg() < size_of_file)
	{
		// read command index
		char idx[2];
		cli_file.read(idx, 2);
		auto cid = *reinterpret_cast<unsigned short*>(idx);

		// if command LAYER
		if (cid == 127)
		{
			layer.print_layer_data();

			char ht[4];
			cli_file.read(ht, 4);

			layer.height = *reinterpret_cast<float*>(ht);
			layer.index = index;
			layer.layer_area = 0;
			index++;

		}
		else if (cid == 132)
		{
			break;
		}
		else if (cid == 130)
		{
			auto polyline = output_polyline(cli_file);
			if (polyline.dir == 1)
				layer.layer_area += polyline.area();
			else if (polyline.dir == 0)
				layer.layer_area -= polyline.area();
		}
	}
	layer.print_layer_data();
}

void read_ascii(ifstream& cli_file)
{
	string line;
	while (!cli_file.eof())
	{
		//cli_file >> line;
		getline(cli_file, line);
		if (line == "$$GEOMETRYSTART")
		{
			break;
		}
	}

	Layer layer;
	int index = 0;

	// Start parsing layers
	while (!cli_file.eof())
	{

		cli_file >> line;
		auto pos = line.find('/');
		auto command = line.substr(0, pos);
		if (command == "$$LAYER")
		{
			layer.print_layer_data();

			const auto params = line.substr(pos + 1);
			//char ht[50];
			layer.index = index;
			layer.height = stof(params);
			layer.layer_area = 0;
			//sprintf_s(ht, "%.3f", l.height);
			index++;
		}
		if (command == "$$POLYLINE")
		{
			auto polyline = output_polyline(line, pos);
			auto area = polyline.area();

			if (polyline.dir == 1)
				layer.layer_area += area;
			else if (polyline.dir == 0)
				layer.layer_area -= area;
		}
	}
	layer.print_layer_data();
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		cout << "Provide file path\n";
		return 0;
	}

	const string file_path = argv[1];

	ifstream cli_file(file_path, ios::in | ios::binary);
	if(!cli_file.is_open())
	{
		cout << "Unable to open file" << endl;
		return 0;
	}

	streampos size_of_file;
	if(cli_file.is_open())
	{
		// find file size
		cli_file.seekg(0, ios::end);
		size_of_file = cli_file.tellg();
		cli_file.seekg(0, ios::beg);

		// Determine if file is Ascii or binary
		string line;
		bool is_ascii = false;
		while (!cli_file.eof())
			
		{
			cli_file >> line;
			if (line == "$$ASCII")
			{
				is_ascii = true;
				break;
			}
			if (line == "$$BINARY")
			{
				is_ascii = false;
				break;
			}
		}
		// BINARY
		if(!is_ascii)
		{
			read_binary(cli_file, size_of_file);
		}
		// ASCII
		else
		{
			read_ascii(cli_file);
		}
	}

	
}

