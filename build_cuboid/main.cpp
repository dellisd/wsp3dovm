#include <string>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

/////////////////////////////// generation with open volume mesh //////////////////////

#define INCLUDE_TEMPLATES
#include <OpenVolumeMesh/Geometry/VectorT.hh>
#include <OpenVolumeMesh/Mesh/HexahedralMesh.hh>
#include <OpenVolumeMesh/Core/OpenVolumeMeshProperty.hh>
#include <OpenVolumeMesh/Core/PropertyDefines.hh>
#include <OpenVolumeMesh/Core/PropertyPtr.hh>
#include <OpenVolumeMesh/FileManager/FileManager.hh>
#include <OpenVolumeMesh/Attribs/StatusAttrib.hh>

#undef INCLUDE_TEMPLATES

// Make some typedefs to facilitate your life
typedef double                                      Real;
typedef OpenVolumeMesh::Geometry::Vec3d             Vector;
typedef OpenVolumeMesh::Geometry::Vec3d             Point;

typedef OpenVolumeMesh::VertexHandle            VertexHandle;
typedef OpenVolumeMesh::EdgeHandle              EdgeHandle;
typedef OpenVolumeMesh::FaceHandle              FaceHandle;
typedef OpenVolumeMesh::CellHandle              CellHandle;
typedef OpenVolumeMesh::OpenVolumeMeshHandle    MeshHandle;

typedef OpenVolumeMesh::HalfFaceHandle          HalfFaceHandle;
typedef OpenVolumeMesh::HalfEdgeHandle          HalfEdgeHandle;

typedef OpenVolumeMesh::TopologyKernel          Kernel;

typedef Kernel::Cell                            Cell;
typedef Kernel::Face                            Face;
typedef Kernel::Edge                            Edge;

typedef double Weight;

const Weight max_weight = std::numeric_limits<double>::max();

using namespace std;
using namespace OpenVolumeMesh;
using namespace OpenVolumeMesh::Geometry;

struct Mesh : GeometricHexahedralMeshV3f
{
	CellPropertyT<float> weight = request_cell_property<float>("weight");

	void write_poly(const std::string filename)
	{
		std::ofstream file(filename, std::ios::trunc);
		if (!file.is_open())
		{
			std::cerr << "failed to open file " << filename << std::endl;
			return;
		}

		file << "# poly file generated by build_cuboid" << endl;

		{
			file << n_vertices() << " 3 0 0" << endl;

			for (auto v : vertices())
			{
				auto point = vertex(v);
				file << std::setw(6) << 1+v.idx() << " " << point[0] << " " << point[1] << " " << point[2] << "\n";
			}
		}

		// select halffaces for output
		StatusAttrib statusAttrib(*this);

		int k = 0;

		for (auto hf : halffaces()) {
			statusAttrib[hf].set_selected(true);
			k++;
		}

		for (auto hf : halffaces()) {
			if (statusAttrib[hf].selected() ) {
				statusAttrib[ opposite_halfface_handle(hf) ].set_selected(false);
				k--;
			}
		}

		{
			file << k << " 0\n";
			for (auto hf : halffaces()) {
				if (statusAttrib[hf].selected()) {
					file << "1" << endl;  // 1 polygon
					file << "4 ";	// we know they are quads
					for (auto v : halfface_vertices(hf)) {
						file << " " << 1 + v.idx();
					}
					file << endl;
				}
			}
		}

		// no holes
		file << " 0" << endl;

		// regions
		{
			file << n_cells() << endl;

			for (auto c : cells()) {
				Vec3f pt = barycenter(c);
				file << c.idx() + 1 << " " << pt[0] << " " << pt[1] << " " << pt[2] << " " << weight[c] << endl;
			}
		}

		file.close();
	}

	void write_nodes(const std::string filename)
	{
		std::ofstream file(filename, std::ios::trunc);
		if (!file.is_open())
		{
			std::cerr << "failed to open file " << filename << std::endl;
			return;
		}

		file << n_vertices() << " 3 0 0\n";
		int i = 1;

		for (auto v : vertices() )
		{
			auto point = vertex(v);
			file << std::setw(6) << i++ << " " << point[0] << " " << point[1] << " " << point[2] << "\n";
		}
		file << "# node file generated by wsp3dovm\n";
		file.close();
	}

	void write_ele(const std::string filename)
	{
		std::ofstream file(filename, std::ios::trunc);
		if (!file.is_open())
		{
			std::cerr << "failed to open file " << filename << std::endl;
			return;
		}

		int i = 1;

		file << n_cells() << "  8  0\n";
		for (auto c : cells()) {
			cout << "n_vertices_in_cell=" << n_vertices_in_cell(c) << endl; // should be 8 
			barycenter(c);
			file << std::setw(5) << i++ << " ";
			for (auto v : cell_vertices(c) )
				file << std::setw(5) << 1 + v.idx() << " "; // tet format is 1-based
			file << "\n";
		}

		file << "# ele file generated by wsp3dovm\n";
		file.close();
	}

	// check for and avoid duplicates
	VertexHandle add_vertex(const Vec3f& point) {
		for (auto v : vertices() ) {
			if (point == vertex(v)) {
				return v;
			}
		}
		return OpenVolumeMesh::GeometricHexahedralMeshV3f::add_vertex(point);
	}

	// a unit size cube 
	void add_cube(int x, int y, int z, int weight )
	{
#if 0
		// shortcut because we know where a wsp will run!
		// dont add red+green cubes which are too far away from center
		if (z < 2048 - 6)
			return;
		if (z > 2048 + 6)
			return;

		// shortcut because we know where a wsp will run!
		// dont add red+green cubes which are too far away from wsp
		if(weight>=4) {

			int near = z % 1024;
			
			if (near >= 512)		// make mod signed
				near -= 1024;

			if (near < 0)			// abs diff to 0
				near = -near;

			if (near > 10)			// not close enough
				return;
				;
		}
#endif


#if 1
// build only the front face "d2n4"
		if (z != 0)
			return;
		if (y < 0)
			return; 
		if (y > 63) // for n==4
			return;
#endif

		const int dx = 1;
		const int dy = 1;
		const int dz = 1;

		VertexHandle ltf = add_vertex(Vec3f(x, y, z));
		VertexHandle ltr = add_vertex(Vec3f(x, y, z + dz));
		VertexHandle lbf = add_vertex(Vec3f(x, y + dy, z));
		VertexHandle lbr = add_vertex(Vec3f(x, y + dy, z + dz));
		VertexHandle rtf = add_vertex(Vec3f(x + dx, y, z));
		VertexHandle rtr = add_vertex(Vec3f(x + dx, y, z + dz));
		VertexHandle rbf = add_vertex(Vec3f(x + dx, y + dy, z));
		VertexHandle rbr = add_vertex(Vec3f(x + dx, y + dy, z + dz));

		// set last param to true for (slow!) topology check after each insertion
		auto ch = add_cell( {{ lbf, rbf, rtf, ltf, lbr, ltr, rtr, rbr } }, false );
		if (ch == InvalidCellHandle) {
			cerr << "failed to create cube at (" << x << ", " << y << ", "  << z << ") exit." << endl;
			exit(-1);
		}
		this->weight[ch] = weight;
	}

	// x increases left -> right
	// y increases top -> bottom
	// z increases into the screen (front --> rear)
	void add_cuboid(int x, int y, int z, int dx, int dy, int dz, int weight )
	{
		if (dx < 0) { x += dx; dx = -dx; }
		if (dy < 0) { y += dy; dy = -dy; }
		if (dz < 0) { z += dz; dz = -dz; }

#if 1
		// add a full set of unit size cubes (slow, for wsp search)
		for (int xx = x; xx < x + dx; ++xx)
			for (int yy = y; yy < y + dy; ++yy)
				for (int zz = z; zz < z + dz; ++zz)
					add_cube( xx, yy, zz, weight);
#else
		// add the whole cuboid at once (faster, for rendering)
		VertexHandle ltf = add_vertex(Vec3f(x, y, z));
		VertexHandle ltr = add_vertex(Vec3f(x, y, z + dz));
		VertexHandle lbf = add_vertex(Vec3f(x, y + dy, z));
		VertexHandle lbr = add_vertex(Vec3f(x, y + dy, z + dz));
		VertexHandle rtf = add_vertex(Vec3f(x + dx, y, z));
		VertexHandle rtr = add_vertex(Vec3f(x + dx, y, z + dz));
		VertexHandle rbf = add_vertex(Vec3f(x + dx, y + dy, z));
		VertexHandle rbr = add_vertex(Vec3f(x + dx, y + dy, z + dz));

		// set last param to true for (slow!) topology check after each insertion
		auto ch = add_cell({ { lbf, rbf, rtf, ltf, lbr, ltr, rtr, rbr } }, false);
		if (ch == InvalidCellHandle) {
			cerr << "failed to create cuboid at (" << x << ", " << y << ", " << z << ") exit." << endl;
			exit(-1);
		}
		this->weight[ch] = weight;
#endif
	}

	void generate(int n, string filename)
	{
		const int wx = 16;
		const int wx1 = 15; 
		const int wy = wx/4;
		const int wz = wy/4;

		const int lx = 1*(wx/wx);
		const int ly = n*lx*(wx/wy);
		const int lz = n*ly*(wx/wz);

		// weight=16: center
		// these are n vertical slabs of height 1 and weight 1
		// they are only partitioned into smaller parts to match satellite cuboid sizes
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				for (int k = 0; k < n; ++k) {
					if ((i % 2) == 0) {
						add_cuboid(i * lx, j * ly, k * lz, lx, ly, lz, wx);
					}
					else {
						// little less weight (epsilon)
						add_cuboid(i * lx, j * ly, k * lz, lx, ly, lz, wx1);
					}
				}
			}
		}

		// weight=4: green satellites of rank 2
		for (int j = 0; j < n; ++j) {

			// the full length is divided only for technical reasons into n parts
			for (int k = 0; k < n; ++k) {
				if ( (j % 2)==1 ) {
					add_cuboid(0 * lx, j * ly, k * lz, -1, ly, lz, wy);
				}
				else {
					add_cuboid(n * lx, j * ly, k * lz, +1, ly, lz, wy);
				}
			}
		}

		// weight=1: blue satellites of rank 3
		for (int i = 0; i < n; ++i) {		// the full x-width is divided only for technical reasons into n parts
			for (int k = 0; k < n; ++k) {

				// the shims of thickness 1
				add_cuboid(i * lx, 0 * ly, k * lz, +1, -1, lz, wx);
				add_cuboid(i * lx, n * ly, k * lz, +1, +1, lz, wx);

				if ((k % 2) == 1) {
					add_cuboid(i * lx, 0 * ly - 1, k * lz, +1, -1, lz, wz); // y: -1 for shim
				}
				else {
					add_cuboid(i * lx, n * ly + 1, k * lz, +1, +1, lz, wz); // y: +1 for shim
				}
			}
		}
		cout << " n_cells=" << n_cells();
		cout << " n_faces=" << n_faces();
		cout << " n_edges=" << n_edges();
		cout << " n_vertices=" << n_vertices();
		cout << " genus=" << genus();
		cout << endl;

		write_poly(filename);
	}

};


int main()
{
	Mesh mesh;
	
	//mesh.enable_bottom_up_incidences(false);
	//mesh.generate(6, "D:/Carleton/2019/d3n6/d3n6.poly");
	mesh.generate(4, "D:/Carleton/2019/d2n4/d2n4.poly");
	//mesh.generate(2, "D:/Carleton/2019/d3n2/d3n2.poly");

	// ..\..\tetgen1.5.0\x64\Release\tetgen.exe -pA D:\Carleton\2019\d3n4\d3n4.poly
	// or: ..\..\tetgen1.5.0\x64\Release\tetgen.exe -pqAV D:\Carleton\2019\d3n4\d3n4.poly
	// generates: .node .ele. face .edge

	// reads .node and .ele, ignores others

	// wsp on left face:
	// ..\x64\RelWithDebInfo\wsp3dovm.exe --input-mesh D:\Carleton\2019\d3n4\d3n4.1 --write_mesh_vtk 1 --start_vertex 49 --termination_vertex 49

	// ..\x64\RelWithDebInfo\wsp3dovm.exe --input-mesh test.1  --write_mesh_vtk 1 

	return 0;
}
