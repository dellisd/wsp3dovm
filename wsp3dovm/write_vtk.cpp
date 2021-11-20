#include "write_vtk.h"

void write_shortest_path_tree_vtk
(
	const Graph& graph,
	GraphNode_descriptor s,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	const std::string& filename
)
{
	std::ofstream file(filename, std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "failed to open file " << filename << std::endl;
		return;
	}

	write_shortest_path_tree_vtk(graph, s, predecessors, distances, file);
}

void write_shortest_path_tree_vtk
(
	const Graph& graph,
	GraphNode_descriptor s,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	std::ostream& output
)
{
	size_t n = boost::num_vertices(graph);

	output <<
		"# vtk DataFile Version 2.0\n"
		"shortest paths tree with root node " << graph[s].vh.idx() << "\n"
		"ASCII\n"
		"DATASET UNSTRUCTURED_GRID\n";
	output << "POINTS " << n << " double\n";

	std::vector<int> id(num_vertices(graph));
	int i = 0;
	Graph::vertex_iterator vertexIt, vertexEnd;
	for (boost::tie(vertexIt, vertexEnd) = boost::vertices(graph); vertexIt != vertexEnd; ++vertexIt)
	{
		GraphNode_descriptor v = *vertexIt;
		id[v] = i++;
		output << graph[v].point << "\n";
	}

	output << "CELLS " << n << " " << 3 * n << "\n";
	for (boost::tie(vertexIt, vertexEnd) = boost::vertices(graph); vertexIt != vertexEnd; ++vertexIt)
	{
		GraphNode_descriptor u = *vertexIt;
		GraphNode_descriptor v = predecessors[u];
		output << "2 " << id[u] << " " << id[v] << "\n";
	}

	output << "CELL_TYPES " << n << "\n";
	for (size_t i = 0; i < n; ++i)
	{
		// vtk cell type 3 is line
		output << "3" "\n";
	}

	output
		<< "POINT_DATA " << n << "\n"
		<< "SCALARS distance double 1\n"
		<< "LOOKUP_TABLE default\n";

	for (boost::tie(vertexIt, vertexEnd) = boost::vertices(graph); vertexIt != vertexEnd; ++vertexIt)
	{
		GraphNode_descriptor u = *vertexIt;
		double distance = distances[u];
		if (isfinite(distance))
		{
			output << distance << "\n";
		}
		else
		{
			// ParaView cannot handle 1.#INF or the like in the .vtk file
			// but we keep the value distinguishable from a normal 0
			output << "-0\n";
		}
	}
	output << "\n";
}

int hops(const std::vector<GraphNode_descriptor> &predecessors, GraphNode_descriptor s, GraphNode_descriptor t)
{
	int h = 0;
	while (s != t)
	{
		t = predecessors[t];
		++h;
	}
	return h;
}

void write_shortest_path_from_to_vtk
(
	const Graph& graph,
	GraphNode_descriptor s, 
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	const std::string& filename
)
{
	if (distances[t] == std::numeric_limits<double>::infinity())
	{
		std::cout << "write_shortest_path_from_to_vtk: target node unreachable" << std::endl;
	}

	int h = hops(predecessors, s, t);

	std::cout 
	<< "from s=" << graph[s].vh.idx() 
	<< " to t=" << graph[t].vh.idx() 
	<< " distance=" << distances[t]
	<< " #hops = " << h 
	<< std::endl;

	std::ofstream file(filename, std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "failed to open file " << filename << std::endl;
		return;
	}

	write_shortest_path_from_to_vtk(graph, s, t, predecessors, distances, file);
}

void write_shortest_path_from_to_vtk
(
	const Graph& graph,
	GraphNode_descriptor s, 
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	std::ostream& output
)
{
	if (distances[t] == std::numeric_limits<double>::infinity())
	{
		std::cout << "write_shortest_path_from_to_vtk: target node unreachable" << std::endl;
	}

	int h = hops(predecessors, s, t);

	std::cout 
	<< "from s=" << graph[s].vh.idx() 
	<< " to t=" << graph[t].vh.idx() 
	<< " distance=" << distances[t]
	<< " #hops = " << h 
	<< std::endl;

	size_t n = boost::num_vertices(graph);

	output <<
		"# vtk DataFile Version 2.0\n"
		"shortest path from " << graph[s].vh.idx() << " to " << graph[t].vh.idx() << "\n"
		"ASCII\n"
		"DATASET UNSTRUCTURED_GRID\n";
	output << "POINTS " << n << " double\n";

	std::vector<int> id(num_vertices(graph));
	int i = 0;
	Graph::vertex_iterator vertexIt, vertexEnd;
	for (boost::tie(vertexIt, vertexEnd) = boost::vertices(graph); vertexIt != vertexEnd; ++vertexIt)
	{
		GraphNode_descriptor u = *vertexIt;
		id[u] = i++;
		output << graph[u].point << "\n";
	}

	output << "CELLS " << h << " " << 3 * h << "\n";
	for (GraphNode_descriptor r = t; r != s; r = predecessors[r])
	{
		GraphNode_descriptor v = predecessors[r];
		output << "2 " << id[r] << " " << id[v] << "\n";
	}

	output << "CELL_TYPES " << h << "\n";
	for (int i = 0; i < h; ++i)
	{
		// vtk cell type 3 is line
		output << "3" "\n";
	}

	output
		<< "POINT_DATA " << n << "\n"
		<< "SCALARS distance double 1\n"
		<< "LOOKUP_TABLE default\n";

	for (boost::tie(vertexIt, vertexEnd) = boost::vertices(graph); vertexIt != vertexEnd; ++vertexIt)
	{
		GraphNode_descriptor u = *vertexIt;
		double distance = distances[u];
		if (isfinite(distance))
		{
			output << distance << "\n";
		}
		else
		{
			// ParaView cannot handle 1.#INF or the like in the .vtk file
			// but we keep the value distinguishable from a normal 0
			output << "-0\n";
		}
	}
	output << "\n";
}

void write_vtk(const Mesh& mesh, const std::string& filename)
{
	std::cout << "write_vtk " << filename << std::endl;
	std::ofstream file(filename, std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "failed to open file " << filename << std::endl;
		return;
	}
	
	write_vtk(mesh, file);
	file.close();
}

void write_vtk(const Mesh& mesh, std::ostream& output)
{
	int number_of_vertices = mesh.n_vertices();

	output <<
		"# vtk DataFile Version 2.0\n"
		"tetrahedralization\n"
		"ASCII\n"
		"DATASET UNSTRUCTURED_GRID\n";

	output << "POINTS " << number_of_vertices << " double\n";

	std::map<VertexHandle, int> V;
	int inum = 0;

	for (auto vit = mesh.vertices_begin(), end = mesh.vertices_end();
		vit != end;
		++vit
	)
	{
		Point point = mesh.vertex(*vit);

		output
			<< point[0] << " "
			<< point[1] << " "
			<< point[2] << " "
			<< "\n";

		V[*vit] = inum++;
	}

	int number_of_cells = mesh.n_cells();

	// for each cell 5 values are given: numPoints (4) and the 4 vertex indices
	output << "CELLS " << number_of_cells << " " << 5 * number_of_cells << "\n";

	for (
		auto cit = mesh.cells_begin(), end = mesh.cells_end();
		cit != end;
		++cit)
	{
		CellHandle ch = *cit;

		output << "4 ";
		for (auto vit = mesh.cv_iter(ch); vit; ++vit)
			output << vit->idx() << " ";
		
		output << "\n";
	}

	output << "CELL_TYPES " << number_of_cells << "\n";
	for (int i = 0; i < number_of_cells; ++i)
	{
		// vtk cell type 10 is tetrahedron
		output << "10" "\n";
	}

	output
		<< "CELL_DATA " << number_of_cells << "\n"
		<< "SCALARS weight double 1\n"
		<< "LOOKUP_TABLE default\n";

	for (
		auto cit = mesh.cells_begin(), end = mesh.cells_end();
		cit != end;
	++cit)
	{
		output << mesh.weight(*cit);
		output << "\n";
	}
}

void write_graph_vtk
(
	const Graph &graph,
	std::string filename
)
{
	std::ofstream file(filename, std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "failed to open file " << filename << std::endl;
		return;
	}

	write_graph_vtk(graph, file);
}

void write_graph_vtk
(
	const Graph& graph,
	std::ostream& output
)
{
	output <<
		"# vtk DataFile Version 2.0\n"
		"steiner graph\n"
		"ASCII\n"
		"DATASET UNSTRUCTURED_GRID\n";
	output << "POINTS " << num_vertices(graph) << " double\n";

	std::vector<int> id(num_vertices(graph));
	int i = 0;
	Graph::vertex_iterator vertexIt, vertexEnd;
	for (boost::tie(vertexIt, vertexEnd) = boost::vertices(graph); vertexIt != vertexEnd; ++vertexIt)
	//for (auto v : graph.m_vertices )
	{
		GraphNode_descriptor u = *vertexIt;
		id[u] = i++;
		output << graph[u].point << "\n";
	}

	output << "CELLS " << num_edges(graph) << " " << 3 * num_edges(graph) << "\n";
	for (auto e : graph.m_edges)
	{
		GraphNode_descriptor u = e.m_source;
		GraphNode_descriptor v = e.m_target;
		output << "2 " << id[u] << " " << id[v] << "\n";
	}

	output << "CELL_TYPES " << num_edges(graph) << "\n";
	for (size_t i = 0; i < num_edges(graph); ++i)
	{
		// vtk cell type 3 is line
		output << "3" "\n";
	}

	// dump weight (not cost!) of edges to check that the adjacent  fetures have been calculated correctly
	output
		<< "CELL_DATA " << num_edges(graph) << "\n"
		<< "SCALARS edge_weight double 1\n"
		<< "LOOKUP_TABLE default\n";

	Graph::edge_iterator edgeIt, edgeEnd;
	for (boost::tie(edgeIt, edgeEnd) = boost::edges(graph); edgeIt != edgeEnd; ++edgeIt)
	{
		GraphEdge_descriptor edge = *edgeIt;
		output << graph[edge].weight << "\n";
	}
	output << "\n";
}

void write_shortest_path_cells_from_to_vtk
(
	const Graph& graph,
	const Mesh& mesh,
	GraphNode_descriptor s,
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	const std::string& filename
)
{
	std::ofstream file(filename, std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "failed to open file " << filename << std::endl;
		return;
	}

	write_shortest_path_cells_from_to_vtk(graph, mesh, s, t, predecessors, distances, file);
}

void write_shortest_path_cells_from_to_vtk
(
	const Graph& graph,
	const Mesh& mesh,
	GraphNode_descriptor s,
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	std::ostream& output
)
{
	if (distances[t] == std::numeric_limits<double>::infinity())
	{
		std::cout << "write_shortest_path_cells_from_to_vtk: target node unreachable" << std::endl;
	}

	std::set<CellHandle> cells = cells_from_graph_nodes(graph, mesh, s, t, predecessors);

	int h = hops(predecessors, s, t);

	size_t n = boost::num_vertices(graph);

	output <<
		"# vtk DataFile Version 2.0\n"
		"cells along shortest path from " << graph[s].vh.idx() << " to " << graph[t].vh.idx() << "\n"
		"ASCII\n"
		"DATASET UNSTRUCTURED_GRID\n";
	output << "POINTS " << n << " double\n";

	std::vector<int> id(num_vertices(graph));
	int i = 0;
	Graph::vertex_iterator vertexIt, vertexEnd;
	for (boost::tie(vertexIt, vertexEnd) = boost::vertices(graph); vertexIt != vertexEnd; ++vertexIt)
	{
		GraphNode_descriptor u = *vertexIt;
		id[u] = i++;
		output << graph[u].point << "\n";
	}

	output << "CELLS " << cells.size() << " " << 5 * cells.size() << "\n";
	for (
		auto cit =cells.begin(), end = cells.end();
		cit != end;
		++cit
		)
	{
		CellHandle ch = *cit;

		output << "4 ";
		for (auto vit = mesh.cv_iter(ch); vit; ++vit)
			output << vit->idx() << " ";

		output << "\n";
	}

	output << "CELL_TYPES " << cells.size() << "\n";
	for (int i = 0; i < cells.size(); ++i)
	{
		// vtk cell type 10 is tetra
		output << "10" "\n";
	}

	output
		<< "CELL_DATA " << cells.size() << "\n"
		<< "SCALARS cellweight double 1\n"
		<< "LOOKUP_TABLE default\n";

	for (
		auto cit = cells.begin(), end = cells.end();
		cit != end;
		++cit
		)
	{
		CellHandle ch = *cit;
		output << mesh.weight(ch) << "\n";
	}
}
