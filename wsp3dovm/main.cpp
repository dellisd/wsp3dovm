#include "common.h"

#include "read_tet.h"
#include "write_vtk.h"
#include "create_steinerpoints.h"
#include "statistics.h"

using namespace boost::chrono;

template< class Clock >
class timer
{
  typename Clock::time_point start;
public:
  timer() : start( Clock::now() ) {}
  typename Clock::duration elapsed() const
  {
    return Clock::now() - start;
  }
  double seconds() const
  {
    return elapsed().count() * ((double)Clock::period::num/Clock::period::den);
  }
};



void set_cell_weights(Mesh &mesh)
{
	boost::mt19937 rng;
	boost::uniform_real<> unity(0.0, 1.0);
	boost::variate_generator<boost::mt19937&, boost::uniform_real<> > get_random_weight(rng, unity);
	int i = 1;

	mesh._cellWeight.resize(mesh.n_cells());

	for (auto c_it = mesh.cells_begin(); c_it != mesh.cells_end(); ++c_it)
	{
		CellHandle ch = *c_it;
		mesh.weight(ch) = get_random_weight();
		//mesh.weight(ch) = i++;
	}
}

void calc_face_weights(Mesh &mesh)
{
	mesh._faceWeight.resize(mesh.n_faces());

	for (auto f_it = mesh.faces_begin(); f_it != mesh.faces_end(); ++f_it)
	{
		OpenVolumeMesh::FaceHandle fh = *f_it;

		OpenVolumeMesh::HalfFaceHandle hfh0 = Kernel::halfface_handle(fh, 0);
		OpenVolumeMesh::HalfFaceHandle hfh1 = Kernel::halfface_handle(fh, 1);

		CellHandle cell0 = mesh.incident_cell(hfh0);
		CellHandle cell1 = mesh.incident_cell(hfh1);

		double w0 = cell0.is_valid() ? mesh.weight(cell0) : max_weight;
		double w1 = cell1.is_valid() ? mesh.weight(cell1) : max_weight;

		mesh.weight(fh) = std::min(w0, w1);
	}
}

void calc_edge_weights(Mesh &mesh)
{
	mesh._edgeWeight.resize(mesh.n_edges());

	for (auto e_it = mesh.edges_begin(); e_it != mesh.edges_end(); ++e_it)
	{
		OpenVolumeMesh::EdgeHandle eh = *e_it;

		//std::cout << "processing edge " << e_it->idx() << " " << mesh.edge(*e_it) << "of valence " << mesh.valence(*e_it) << std::endl;

		OpenVolumeMesh::HalfEdgeHandle heh0 = Kernel::halfedge_handle(eh, 0);
		//OpenVolumeMesh::HalfEdgeHandle heh1 = Kernel::halfedge_handle(eh, 1);

		double weight = max_weight;

		for( auto hec0 = mesh.hec_iter(heh0); hec0; ++hec0 )
		{
			//std::cout << "  hec0 incident cell " << hec0->idx() << std::endl;
			if (hec0->is_valid())
				weight = std::min(weight, mesh.weight(*hec0));
		}

		// this yields exactly the same cells in reverse order

		//for (auto hec1 = mesh.hec_iter(heh1); hec1; ++hec1)
		//{
		//	std::cout << "  hec1 incident cell " << hec1->idx() << std::endl;
		//if (hec1->is_valid())
		//		weight = std::min(weight, mesh.weight(*hec1));
		//}

		mesh.weight(eh) = weight;
	}
}

int main(int argc, char** argv)
{
	Mesh mesh;

	//boost::filesystem::path inputfilename("../../Meshes/holmes_off/geometry/tetrahedron.1");
	//boost::filesystem::path inputfilename("../..//Meshes/misc/cube.1");
	//boost::filesystem::path inputfilename("../..//Meshes/misc/moomoo.1");

	if(argc < 2)
	{
		std::cout << "usage: " << argv[0] << " <tetgen file>" << std::endl;
		return -1;
	}
	// generated by tetgen -p -Y -M0/0
	boost::filesystem::path inputfilename(argv[1]);
	// output:
	//	slurping tetraheder soup from ../../Meshes/GEN_IV_plenum/NewFineMesh.1...
	//	reading 51944 nodes... done.
	//	reading 172284 tetras... done.
	//	read_tet finished.
	//	min edge length : 9.55886e-006
	//	max edge length : 5.00217
	//	edge histogram bin 0 : 206980
	//	edge histogram bin 1 : 64234
	//	edge histogram bin 2 : 4087
	//	edge histogram bin 3 : 832
	//	edge histogram bin 4 : 32
	//	edge histogram bin 5 : 14
	//	edge histogram bin 6 : 18
	//	edge histogram bin 7 : 40
	//	edge histogram bin 8 : 39
	//	edge histogram bin 9 : 3
	//	write_vtk NewFineMesh.vtk
	//	graph nodes : 172284
	//	graph edges : 585204
	//	s = 0; t = 125038
	//	This is the end...
	//	Dr�cken Sie eine beliebige Taste . . .

	// generated by tetgen -p -q -Y -M0/0
	//boost::filesystem::path inputfilename("../../Meshes/GEN_IV_plenum/NewFineMesh.2/NewFineMesh.2");

	
	{
		timer<high_resolution_clock> t;
		read_tet(mesh, inputfilename.string() );
		std::cout << "read_tet: " << t.seconds() << " s" << std::endl;
	}

	set_cell_weights(mesh);
	calc_face_weights(mesh);
	calc_edge_weights(mesh);

	//dump(mesh);
	print_mesh_statistics(mesh);

	{
		timer<high_resolution_clock> t;
		write_vtk(mesh, inputfilename.filename().replace_extension(".vtk").string() );
		std::cout << "write_vtk: " << t.seconds() << " s" << std::endl;
	}
	
	Graph graph;

	{
		timer<high_resolution_clock> t;
		create_steiner_points(graph, mesh);
		std::cout << "create_steiner_points: " << t.seconds() << " s" << std::endl;
	}
	
	std::cout << "graph nodes: " << graph.m_vertices.size() << std::endl;
	std::cout << "graph edges: " << graph.m_edges.size() << std::endl;

	// the distances are temporary, so we choose an external property for that
	std::vector<double> distances(num_vertices(graph));
	std::vector<GraphNode_descriptor> predecessors(num_vertices(graph));

	{
		timer<high_resolution_clock> t;

		boost::dijkstra_shortest_paths
		(
			graph,
			*vertices(graph).first,
			boost::weight_map(get(&GraphEdge::weight, graph)).
			distance_map(boost::make_iterator_property_map(distances.begin(), get(boost::vertex_index, graph))).
			predecessor_map(boost::make_iterator_property_map(predecessors.begin(), get(boost::vertex_index, graph))).
			distance_inf(std::numeric_limits<double>::infinity())
		);
		std::cout << "dijkstra_shortest_paths: " << t.seconds() << " s" << std::endl;
	}
	
	write_shortest_path_tree_vtk(graph, predecessors, distances, inputfilename.filename().replace_extension("_wsp_tree.vtk").string());

	write_shortest_path_max_vtk(graph, predecessors, distances, inputfilename.filename().replace_extension("_wsp_max.vtk").string());

	// write_graph_dot("graph.dot", graph);

	std::cout << "This is the end..." << std::endl;

	return EXIT_SUCCESS;
}
