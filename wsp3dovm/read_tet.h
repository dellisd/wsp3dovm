#include "common.h"

/*
	read tetgen output. filename must not include extensions
*/
void read_tet(Mesh &mesh, std::string filename);

// Read tetgen output from a set of streams
void read_tet(Mesh& mesh, std::istream& node, std::istream& ele);
