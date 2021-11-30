#ifndef WASM_H
#define WASM_H

#ifdef __EMSCRIPTEN__

#include "common.h"
#include <emscripten/bind.h>

struct Options {
    int start_vertex;
    int termination_vertex;
    int random_s_t_vertices;
    double spanner_stretch;
    double yardstick;
    bool use_random_cellweights;

    Options(int start_vertex = -1,
            int termination_vertex = -1,
            int random_s_t_vertices = 0,
            double spanner_stretch = 0.0,
            double yardstick = 0.0,
            bool use_random_cellweights = false);
};

class WspResult {
private:
    std::string mesh_vtk;
    std::string steiner_graph_vtk;
    std::string shortest_path_tree_vtk;
    std::string shortest_path_from_to_vtk;
    std::string shortest_path_cells_from_to_vtk;
    double approx_ratio;

public:
    WspResult(std::string mesh_vtk,
              std::string steiner_graph_vtk,
              std::string shortest_path_tree_vtk,
              std::string shortest_path_from_to_vtk,
              std::string shortest_path_cells_from_to_vtk,
              double approx_ratio);

    std::string get_mesh_vtk() const { return mesh_vtk; }
    std::string get_steiner_graph_vtk() const { return steiner_graph_vtk; }
    std::string get_shortest_path_tree_vtk() const { return shortest_path_tree_vtk; }
    std::string get_shortest_path_from_to_vtk() const { return shortest_path_from_to_vtk; }
    std::string get_shortest_path_cells_from_to_vtk() const { return shortest_path_cells_from_to_vtk; }
    double get_approx_ratio() const { return approx_ratio; }
};

// Convert the .node and .ele files into VTK format
std::string gen_vtk(std::string node, std::string ele);
#endif // __EMSCRIPTEN__
#endif // WASM_H