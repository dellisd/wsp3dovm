#include "common.h"

#include "create_steinerpoints.h"
#include "read_tet.h"
#include "statistics.h"
#include "write_tet.h"
#include "write_vtk.h"

using namespace std;
using namespace boost;
using namespace boost::chrono;

template <class Clock>
class timer {
    typename Clock::time_point start;

public:
    timer() : start(Clock::now()) {}
    typename Clock::duration elapsed() const {
        return Clock::now() - start;
    }
    double seconds() const {
        return elapsed().count() * ((double)Clock::period::num / Clock::period::den);
    }
};

void set_random_cell_weights(Mesh& mesh) {
    // random_device may be a non-deterministic random number generator using hardware entropy source
    //std::random_device random_device;
    std::mt19937 generator /*(random_device())*/;
    std::uniform_real_distribution<> random_value(1, 1000);

    // is not clear to me, what a good distribution could look like.
    // the lower limit 1 is such that it is not too close to 0 (where all paths would have equal length 0)
    // but allows for a higher max/min ratio such that path reflection on faces/edges is quite possible
    mesh._cellWeight.resize(mesh.n_cells());

    for (auto c_it = mesh.cells_begin(); c_it != mesh.cells_end(); ++c_it) {
        CellHandle ch = *c_it;
        mesh.weight(ch) = random_value(generator);
    }
}

void calc_face_weights(Mesh& mesh) {
    mesh._faceWeight.resize(mesh.n_faces());

    for (auto f_it = mesh.faces_begin(); f_it != mesh.faces_end(); ++f_it) {
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

void calc_edge_weights(Mesh& mesh) {
    mesh._edgeWeight.resize(mesh.n_edges());

    for (auto e_it = mesh.edges_begin(); e_it != mesh.edges_end(); ++e_it) {
        OpenVolumeMesh::EdgeHandle eh = *e_it;

        //std::cout << "processing edge " << e_it->idx() << " " << mesh.edge(*e_it) << "of valence " << mesh.valence(*e_it) << std::endl;

        OpenVolumeMesh::HalfEdgeHandle heh0 = Kernel::halfedge_handle(eh, 0);
        //OpenVolumeMesh::HalfEdgeHandle heh1 = Kernel::halfedge_handle(eh, 1);

        Weight weight = max_weight;

        for (auto hec0 = mesh.hec_iter(heh0); hec0; ++hec0) {
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

std::ofstream distance_stream;

double run_single_dijkstra(
    const Graph& graph,
    const Mesh& mesh,
    int s_node,
    int t_node,
    bool dump_path = false,
    bool dump_cells = false,
    bool dump_tree = false,
    filesystem::path basename = "out") {
    // the distances are temporary, so we choose an external property for that
    std::vector<double> distance(num_vertices(graph));
    std::vector<GraphNode_descriptor> predecessor(num_vertices(graph));

    boost::dijkstra_shortest_paths(
        graph,
        s_node,
        boost::weight_map(get(&GraphEdge::weight, graph)).distance_map(boost::make_iterator_property_map(distance.begin(), get(boost::vertex_index, graph))).predecessor_map(boost::make_iterator_property_map(predecessor.begin(), get(boost::vertex_index, graph))).distance_inf(std::numeric_limits<double>::infinity()));

    double euclidean_distance = norm(graph[s_node].point, graph[t_node].point);
    double approx_distance = distance[t_node];
    double approx_ratio = approx_distance / euclidean_distance;

    distance_stream << approx_distance << ", ";

    if (dump_tree) {
        timer<high_resolution_clock> t;

        stringstream extension;
        extension << "_wsp_tree_s" << s_node << ".vtk";

        write_shortest_path_tree_vtk(
            graph,
            s_node,
            predecessor,
            distance,
            basename.filename().replace_extension(extension.str()).string());

        std::cout << "write_shortest_path_tree_vtk: " << t.seconds() << " s" << std::endl;
    }

    if (dump_path) {
        timer<high_resolution_clock> t;

        stringstream extension;
        extension << "_wsp_path_s" << s_node << "_t" << t_node << ".vtk";

        write_shortest_path_from_to_vtk(
            graph,
            s_node,
            t_node,
            predecessor,
            distance,
            basename.filename().replace_extension(extension.str()).string());
        std::cout << "write_shortest_path_to_vtk: " << t.seconds() << " s" << std::endl;
    }

    if (dump_cells) {
        timer<high_resolution_clock> t;

        stringstream extension;
        extension << "_wsp_path_cells_s" << s_node << "_t" << t_node << ".vtk";

        write_shortest_path_cells_from_to_vtk(
            graph,
            mesh,
            s_node,
            t_node,
            predecessor,
            distance,
            basename.filename().replace_extension(extension.str()).string());
        std::cout << "write_shortest_path_cells_from_to_vtk: " << t.seconds() << " s" << std::endl;

        std::set<CellHandle> cells = cells_from_graph_nodes(graph, mesh, s_node, t_node, predecessor);

        write_cells_tet(mesh, cells, basename.filename().replace_extension(extension.str()).string());
    }

    return approx_ratio;
}

#ifndef __EMSCRIPTEN__
int main(int argc, char** argv) {
    timer<high_resolution_clock> total_time;

    try {
        std::cout << "sizeof an int:   " << sizeof(int) << std::endl;
        std::cout << "sizeof a void*:  " << sizeof(void*) << std::endl;
        std::cout << "sizeof a Handle: " << sizeof(EdgeHandle) << std::endl;

        int start_vertex;       // for single source shortest paths (Dijkstra)
        int termination_vertex; // an optional termination vertex for which the shortest path will be reported
        bool write_mesh_vtk;
        bool write_steiner_graph_vtk;
        bool use_random_cellweights;

        int num_random_s_t_vertices; // number of randomly generated s and t vertex pairs

        double stretch;   // spaner graph stretch factor
        double yardstick; // max. size of edge for edge subdivisions

        program_options::options_description desc("Allowed options");
        desc.add_options()("help,h", "produce help message")("start_vertex,s", program_options::value<int>(&start_vertex)->default_value(-1), "shortest path start vertex number (-1==none/random)")("termination_vertex,t", program_options::value<int>(&termination_vertex)->default_value(-1), "shortest path termination vertex number (-1==none/random)")("random_s_t_vertices,r", program_options::value<int>(&num_random_s_t_vertices)->default_value(0), "number of randomly generated s and t vertex pairs")("spanner_stretch,x", program_options::value<double>(&stretch)->default_value(0.0), "spanner graph stretch factor")("yardstick,y", program_options::value<double>(&yardstick)->default_value(0.0), "interval length for interval scheme (0: do not subdivide edges)")("write_mesh_vtk,m", program_options::value<bool>(&write_mesh_vtk)->default_value(false), "write input mesh as .vtk")("write_steiner_graph_vtk,g", program_options::value<bool>(&write_steiner_graph_vtk)->default_value(false), "write steiner graph as .vtk")("use-random-cellweights,u", program_options::value<bool>(&use_random_cellweights)->default_value(false), "generate (pseudo-)random cell weights internally")("input-mesh", program_options::value<std::string>(), "set input filename (tetgen 3D mesh files wo extension)");

        program_options::positional_options_description positional_options;
        positional_options.add("input-mesh", 1);

        program_options::variables_map vm;
        try {
            program_options::store(
                program_options::command_line_parser(argc, argv).options(desc).positional(positional_options).run(),
                vm);
        } catch (...) {
            // exceptions in cmd line parsing are aweful, but happen easily
            std::cerr << "exception while parsing command line, exit." << std::endl;
            std::cerr << desc << endl;
            return EXIT_FAILURE;
        }

        program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << endl;
            return EXIT_FAILURE;
        }

        if (vm.count("input-mesh")) {
            cout << "Input mesh: " << vm["input-mesh"].as<string>() << endl;
        } else {
            cout << "no input-mesh specified, exit." << endl;
            std::cout << desc << endl;
            return EXIT_FAILURE;
        }

        boost::filesystem::path inputfilename(vm["input-mesh"].as<string>());

        Mesh mesh;

        timer<high_resolution_clock> t;
        read_tet(mesh, inputfilename.string());
        std::cout << "read_tet [s]: " << t.seconds() << std::endl;

        // cell weight are initialized from tet file, weight 1.0 is used when no specific weights are present
        if (use_random_cellweights) {
            set_random_cell_weights(mesh);
        }
        calc_face_weights(mesh);
        calc_edge_weights(mesh);

        mesh.print_memory_statistics();
        print_mesh_statistics(mesh);

        if (write_mesh_vtk) {
            timer<high_resolution_clock> t;
            write_vtk(mesh, inputfilename.filename().replace_extension(".vtk").string());
            std::cout << "write_vtk [s]: " << t.seconds() << std::endl;
        }

        Graph graph;

        {
            timer<high_resolution_clock> t;

            //create_barycentric_steiner_points(graph, mesh);
            //std::cout << "create_barycentric_steiner_points: " << t.seconds() << " s" << std::endl;

            if (stretch < 0.0) {
                create_surface_steiner_points(graph, mesh);
                std::cout << "create_surface_steiner_points [s]: " << t.seconds() << std::endl;
            } else {
                std::cout << "create_steiner_graph_improved_spanner with stretch " << stretch << " and interval " << yardstick << std::endl;
                create_steiner_graph_improved_spanner(graph, mesh, stretch, yardstick);
                std::cout << "create_steiner_graph_improved_spanner [s]: " << t.seconds() << std::endl;
            }
        }

        print_steiner_point_statistics(mesh);

        std::cout << "graph nodes: " << graph.m_vertices.size() << std::endl;
        std::cout << "graph edges: " << graph.m_edges.size() << std::endl;

        if (write_steiner_graph_vtk) {
            timer<high_resolution_clock> t;
            write_graph_vtk(graph, inputfilename.filename().replace_extension("_steiner_graph.vtk").string());
            std::cout << "write_steiner_graph_vtk [s]: " << t.seconds() << std::endl;
        }

        if (start_vertex >= 0 && termination_vertex >= 0) {
            std::cout << "running single dijkstra for s=" << start_vertex << " and t=" << termination_vertex << std::endl;

            timer<high_resolution_clock> t;

            double approx_ratio = run_single_dijkstra(graph, mesh, start_vertex, termination_vertex, true, true, true, inputfilename.filename());

            std::cout << "total time [s] for " << 1 << " dijkstra_shortest_paths: " << t.seconds() << std::endl;

            std::cout << "shortest path approximation ratio: " << approx_ratio << std::endl;
        }

        if (num_random_s_t_vertices > 0) {
            std::cout << "running " << num_random_s_t_vertices << " dijkstra for random vertex pairs" << std::endl;

            std::mt19937 generator;
            // we take only original mesh vertices into account such that computations for different steiner graphs keep comparable
            std::uniform_int_distribution<> random_value(0, mesh.n_vertices() - 1); // closed interval (including max)

            double min_approx_ratio = std::numeric_limits<double>::max();
            int min_s;
            int min_t;
            double max_approx_ratio = std::numeric_limits<double>::min();
            int max_s;
            int max_t;
            double sum_approx_ratio = 0;

            timer<high_resolution_clock> t;

            // create histogram with 10 bins in range 100% .. 110%
            const int num_bins = 10;
            double histo_min = 1.0;
            double histo_max = 1.1;
            double histo[num_bins];
            std::fill(begin(histo), end(histo), 0);

            distance_stream.open("distances.csv", fstream::out | fstream::app);
            distance_stream << stretch << ", " << yardstick << ", " << graph.m_vertices.size() << ", " << graph.m_edges.size() << ", ";

            for (int i = 0; i < num_random_s_t_vertices; ++i) {
                int s;
                int t;

                do {
                    if (start_vertex < 0)
                        s = random_value(generator);
                    else
                        s = start_vertex;

                    if (termination_vertex < 0)
                        t = random_value(generator);
                    else
                        t = termination_vertex;
                } while (s == t);

                double approx_ratio = run_single_dijkstra(graph, mesh, s, t, true, false, false, inputfilename.filename());

                int bin = (int)(num_bins * (approx_ratio - histo_min) / (histo_max - histo_min));
                if (bin < 0) {
                    std::cerr << "ERROR: Dijkstra (" << s << "," << t << ") produced shorter path than Euclid would allow." << std::endl;
                }
                if (bin >= num_bins) {
                    bin = num_bins - 1;
                }
                ++histo[bin];

                if (approx_ratio < min_approx_ratio) {
                    min_approx_ratio = approx_ratio;
                    min_s = s;
                    min_t = t;
                }

                if (approx_ratio > max_approx_ratio) {
                    max_approx_ratio = approx_ratio;
                    max_s = s;
                    max_t = t;
                }

                sum_approx_ratio += approx_ratio;
            }

            distance_stream << "\n";
            distance_stream.close();

            double avg_approx_ratio = sum_approx_ratio / num_random_s_t_vertices;

            std::cout << "total time [s] for " << num_random_s_t_vertices << " dijkstra_shortest_paths: " << t.seconds() << std::endl;
            std::cout << "min shortest path approximation ratio: " << min_approx_ratio << " s=" << min_s << " , t=" << min_t << std::endl;
            std::cout << "avg shortest path approximation ratio: " << avg_approx_ratio << std::endl;
            std::cout << "max shortest path approximation ratio: " << max_approx_ratio << " s=" << max_s << " , t=" << max_t << std::endl;

            for (int bin = 0; bin < num_bins; ++bin) {
                std::cout << "approx. ratio histo: " << histo_min + bin * ((histo_max - histo_min) / num_bins) << " : " << histo[bin] << std::endl;
            }
        }

        // write_graph_dot("graph.dot", graph);

        std::cout << "This is the end. Total time [s]: " << total_time.seconds() << std::endl;
        // save the time used for d'tor
        _Exit(EXIT_SUCCESS); // Terminates the process normally by returning control to the host environment, but without performing any of the regular cleanup tasks for terminating processes (as function exit does).

    } catch (const std::exception& e) {
        std::cerr << "exception happened: " << e.what() << ", exit." << endl;
    } catch (...) {
        std::cerr << "unknown exception happened, exit." << endl;
    }

    return EXIT_SUCCESS;
}
#else
#include <emscripten.h>
#include <emscripten/bind.h>

double run_single_dijkstra(
    const Graph& graph,
    const Mesh& mesh,
    int s_node,
    int t_node,
    std::ostream& shortest_path_tree_vtk,
    std::ostream& shortest_path_from_to_vtk,
    std::ostream& shortest_path_cells_from_to_vtk
    //std::ostream& cells_tet
) {
    // the distances are temporary, so we choose an external property for that
    std::vector<double> distance(num_vertices(graph));
    std::vector<GraphNode_descriptor> predecessor(num_vertices(graph));

    boost::dijkstra_shortest_paths(
        graph,
        s_node,
        boost::weight_map(get(&GraphEdge::weight, graph)).distance_map(boost::make_iterator_property_map(distance.begin(), get(boost::vertex_index, graph))).predecessor_map(boost::make_iterator_property_map(predecessor.begin(), get(boost::vertex_index, graph))).distance_inf(std::numeric_limits<double>::infinity()));

    double euclidean_distance = norm(graph[s_node].point, graph[t_node].point);
    double approx_distance = distance[t_node];
    double approx_ratio = approx_distance / euclidean_distance;

    distance_stream << approx_distance << ", ";

    {
        //timer<high_resolution_clock> t;

        write_shortest_path_tree_vtk(
            graph,
            s_node,
            predecessor,
            distance,
            shortest_path_tree_vtk);

        //std::cout << "write_shortest_path_tree_vtk: " << t.seconds() << " s" << std::endl;
    }

    {
        //timer<high_resolution_clock> t;

        write_shortest_path_from_to_vtk(
            graph,
            s_node,
            t_node,
            predecessor,
            distance,
            shortest_path_from_to_vtk);

        //std::cout << "write_shortest_path_to_vtk: " << t.seconds() << " s" << std::endl;
    }

    {
        //timer<high_resolution_clock> t;

        stringstream extension;
        extension << "_wsp_path_cells_s" << s_node << "_t" << t_node << ".vtk";

        write_shortest_path_cells_from_to_vtk(
            graph,
            mesh,
            s_node,
            t_node,
            predecessor,
            distance,
            shortest_path_cells_from_to_vtk);
        //std::cout << "write_shortest_path_cells_from_to_vtk: " << t.seconds() << " s" << std::endl;

        std::set<CellHandle> cells = cells_from_graph_nodes(graph, mesh, s_node, t_node, predecessor);

        // TODO: ?
        //write_cells_tet(mesh, cells, basename.filename().replace_extension(extension.str()).string());
    }

    return approx_ratio;
}

// EMSCRIPTEN_KEEPALIVE void do_thing(const char* node, const char* ele) {
// 	Mesh mesh;

// 	std::stringstream
// }

int main(int argc, char** argv) {
    return EXIT_SUCCESS;
}

#include "wasm.h"

WspResult wsp(const Options& options, const std::string node_text, const std::string ele_text) {
    Mesh mesh;

    std::stringstream node(node_text);
    std::stringstream ele(ele_text);

    //timer<high_resolution_clock> t;
    read_tet(mesh, node, ele);
    //std::cout << "read_tet [s]: " << t.seconds() << std::endl;

    // cell weight are initialized from tet file, weight 1.0 is used when no specific weights are present
    if (options.use_random_cellweights) {
        set_random_cell_weights(mesh);
    }
    calc_face_weights(mesh);
    calc_edge_weights(mesh);

    mesh.print_memory_statistics();
    print_mesh_statistics(mesh);

    std::ostringstream mesh_vtk;
    {
        //timer<high_resolution_clock> t;
        write_vtk(mesh, mesh_vtk);
        //std::cout << "write_vtk [s]: " << t.seconds() << std::endl;
    }

    Graph graph;

    {
        //timer<high_resolution_clock> t;

        //create_barycentric_steiner_points(graph, mesh);
        //std::cout << "create_barycentric_steiner_points: " << t.seconds() << " s" << std::endl;

        if (options.spanner_stretch < 0.0) {
            create_surface_steiner_points(graph, mesh);
            //std::cout << "create_surface_steiner_points [s]: " << t.seconds() << std::endl;
        } else {
            std::cout << "create_steiner_graph_improved_spanner with stretch " << options.spanner_stretch << " and interval " << options.yardstick << std::endl;
            create_steiner_graph_improved_spanner(graph, mesh, options.spanner_stretch, options.yardstick);
            //std::cout << "create_steiner_graph_improved_spanner [s]: " << t.seconds() << std::endl;
        }
    }

    print_steiner_point_statistics(mesh);

    std::cout << "graph nodes: " << graph.m_vertices.size() << std::endl;
    std::cout << "graph edges: " << graph.m_edges.size() << std::endl;

    std::ostringstream steiner_graph_vtk;
    {
        //timer<high_resolution_clock> t;
        write_graph_vtk(graph, steiner_graph_vtk);
        //std::cout << "write_steiner_graph_vtk [s]: " << t.seconds() << std::endl;
    }

    // if (options.start_vertex >= 0 && options.termination_vertex >= 0) {
    //     std::cout << "running single dijkstra for s=" << options.start_vertex << " and t=" << options.termination_vertex << std::endl;

    //     timer<high_resolution_clock> t;

    //     // TODO
    //     //double approx_ratio = run_single_dijkstra(graph, mesh, options.start_vertex, options.termination_vertex, true, true, true, inputfilename.filename());
    //     std::cout << "total time [s] for " << 1 << " dijkstra_shortest_paths: " << t.seconds() << std::endl;
    //     //std::cout << "shortest path approximation ratio: " << approx_ratio << std::endl;
    // }

    std::ostringstream shortest_path_tree_vtk;
    std::ostringstream shortest_path_from_to_vtk;
    std::ostringstream shortest_path_cells_from_to_vtk;
    double approx_ratio;

    if (options.start_vertex >= 0 && options.termination_vertex >= 0) {
        std::cout << "running single dijkstra for s=" << options.start_vertex << " and t=" << options.termination_vertex << std::endl;

        //timer<high_resolution_clock> t;

        approx_ratio = run_single_dijkstra(graph, mesh, options.start_vertex, options.termination_vertex, shortest_path_tree_vtk, shortest_path_from_to_vtk, shortest_path_cells_from_to_vtk);

        //std::cout << "total time [s] for " << 1 << " dijkstra_shortest_paths: " << t.seconds() << std::endl;

        std::cout << "shortest path approximation ratio: " << approx_ratio << std::endl;
    }

    if (options.random_s_t_vertices > 0) {
        std::cout << "running " << options.random_s_t_vertices << " dijkstra for random vertex pairs" << std::endl;

        std::mt19937 generator;
        // we take only original mesh vertices into account such that computations for different steiner graphs keep comparable
        std::uniform_int_distribution<> random_value(0, mesh.n_vertices() - 1); // closed interval (including max)

        double min_approx_ratio = std::numeric_limits<double>::max();
        int min_s;
        int min_t;
        double max_approx_ratio = std::numeric_limits<double>::min();
        int max_s;
        int max_t;
        double sum_approx_ratio = 0;

        //timer<high_resolution_clock> t;

        // create histogram with 10 bins in range 100% .. 110%
        const int num_bins = 10;
        double histo_min = 1.0;
        double histo_max = 1.1;
        double histo[num_bins];
        std::fill(begin(histo), end(histo), 0);

        distance_stream.open("distances.csv", fstream::out | fstream::app);
        distance_stream << options.spanner_stretch << ", " << options.yardstick << ", " << graph.m_vertices.size() << ", " << graph.m_edges.size() << ", ";

        for (int i = 0; i < options.random_s_t_vertices; ++i) {
            int s;
            int t;

            do {
                if (options.start_vertex < 0)
                    s = random_value(generator);
                else
                    s = options.start_vertex;

                if (options.termination_vertex < 0)
                    t = random_value(generator);
                else
                    t = options.termination_vertex;
            } while (s == t);

            approx_ratio = run_single_dijkstra(graph, mesh, s, t, shortest_path_tree_vtk, shortest_path_from_to_vtk, shortest_path_cells_from_to_vtk);

            int bin = (int)(num_bins * (approx_ratio - histo_min) / (histo_max - histo_min));
            if (bin < 0) {
                std::cerr << "ERROR: Dijkstra (" << s << "," << t << ") produced shorter path than Euclid would allow." << std::endl;
            }
            if (bin >= num_bins) {
                bin = num_bins - 1;
            }
            ++histo[bin];

            if (approx_ratio < min_approx_ratio) {
                min_approx_ratio = approx_ratio;
                min_s = s;
                min_t = t;
            }

            if (approx_ratio > max_approx_ratio) {
                max_approx_ratio = approx_ratio;
                max_s = s;
                max_t = t;
            }

            sum_approx_ratio += approx_ratio;
        }

        distance_stream << "\n";
        distance_stream.close();

        double avg_approx_ratio = sum_approx_ratio / options.random_s_t_vertices;

        //std::cout << "total time [s] for " << options.random_s_t_vertices << " dijkstra_shortest_paths: " << t.seconds() << std::endl;
        std::cout << "min shortest path approximation ratio: " << min_approx_ratio << " s=" << min_s << " , t=" << min_t << std::endl;
        std::cout << "avg shortest path approximation ratio: " << avg_approx_ratio << std::endl;
        std::cout << "max shortest path approximation ratio: " << max_approx_ratio << " s=" << max_s << " , t=" << max_t << std::endl;

        for (int bin = 0; bin < num_bins; ++bin) {
            std::cout << "approx. ratio histo: " << histo_min + bin * ((histo_max - histo_min) / num_bins) << " : " << histo[bin] << std::endl;
        }
    }

    return WspResult(mesh_vtk.str(), steiner_graph_vtk.str(), shortest_path_tree_vtk.str(), shortest_path_from_to_vtk.str(), shortest_path_cells_from_to_vtk.str(), approx_ratio);
}

EMSCRIPTEN_BINDINGS(wsp3dovm) {
    emscripten::function("wsp", &wsp);
}

#endif
