#include "common.h"

// write tetrahedralization in vtk format
void write_vtk(const Mesh& mesh, std::ostream& output);

// write tetrahedralization in vtk format to a file
void write_vtk(const Mesh& mesh, const std::string& filename);

// write single source shortest path tree in vtk format
void write_shortest_path_tree_vtk
(
	const Graph& graph,
	GraphNode_descriptor s, 
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	std::ostream& output
);

// write single source shortest path tree in vtk format to a file
void write_shortest_path_tree_vtk
(
	const Graph& graph,
	GraphNode_descriptor s, 
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	const std::string& filename
);

// write a longest shortest path in vtk format
void write_shortest_path_from_to_vtk
(
	const Graph& graph,
	GraphNode_descriptor s,
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	std::ostream& filename
);

// write a longest shortest path in vtk format to a file
void write_shortest_path_from_to_vtk
(
	const Graph& graph,
	GraphNode_descriptor s,
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	const std::string& filename
);

void write_shortest_path_cells_from_to_vtk
(
	const Graph& graph,
	const Mesh& mesh,
	GraphNode_descriptor s,
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	std::ostream& output
);

void write_shortest_path_cells_from_to_vtk
(
	const Graph& graph,
	const Mesh& mesh,
	GraphNode_descriptor s,
	GraphNode_descriptor t,
	const std::vector<GraphNode_descriptor>& predecessors,
	const std::vector<double>& distances,
	const std::string& filename
);

// write complete graph, mainly for debugging small examples
void write_graph_vtk
(
	const Graph& graph,
	std::string filename
);

void write_graph_vtk
(
	const Graph& graph,
	std::ostream& output
);

