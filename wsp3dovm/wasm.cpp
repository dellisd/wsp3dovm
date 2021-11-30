#include "wasm.h"
#include "read_tet.h"
#include "write_vtk.h"

#ifdef __EMSCRIPTEN__

Options::Options(int start_vertex,
                 int termination_vertex,
                 int random_s_t_vertices,
                 double spanner_stretch,
                 double yardstick,
                 bool use_random_cellweights)
    : start_vertex(start_vertex),
      termination_vertex(termination_vertex),
      random_s_t_vertices(random_s_t_vertices),
      spanner_stretch(spanner_stretch),
      yardstick(yardstick),
      use_random_cellweights(use_random_cellweights) {
}

WspResult::WspResult(std::string mesh_vtk,
                     std::string steiner_graph_vtk,
                     std::string shortest_path_tree_vtk,
                     std::string shortest_path_from_to_vtk,
                     std::string shortest_path_cells_from_to_vtk,
                     double approx_ratio)
    : mesh_vtk(mesh_vtk),
      steiner_graph_vtk(steiner_graph_vtk),
      shortest_path_tree_vtk(shortest_path_tree_vtk),
      shortest_path_from_to_vtk(shortest_path_from_to_vtk),
      shortest_path_cells_from_to_vtk(shortest_path_cells_from_to_vtk),
      approx_ratio(approx_ratio) {
}

std::string gen_vtk(std::string node, std::string ele) {
  Mesh mesh;

  std::stringstream node_stream(node);
  std::stringstream ele_stream(ele);

  read_tet(mesh, node_stream, ele_stream);

  std::ostringstream output;
  write_vtk(mesh, output);

  return output.str();
}

EMSCRIPTEN_BINDINGS(wsp3dovm_wasm) {
    emscripten::class_<Options>("Options")
        .constructor<int, int, int, double, double, bool>()
        .property("startVertex", &Options::start_vertex);

    emscripten::class_<WspResult>("WspResult")
        .constructor<std::string, std::string, std::string, std::string, std::string, double>()
        .property("meshVtk", &WspResult::get_mesh_vtk)
        .property("steinerGraphVtk", &WspResult::get_steiner_graph_vtk)
        .property("shortestPathTreeVtk", &WspResult::get_shortest_path_tree_vtk)
        .property("shortestPathFromToVtk", &WspResult::get_shortest_path_from_to_vtk)
        .property("shortestPathCellsFromToVtk", &WspResult::get_shortest_path_cells_from_to_vtk);

    emscripten::function("genVtk", &gen_vtk);
}

#endif