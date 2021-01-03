#include "basicVTKwritter.hpp"

using std::endl;

bool basicVTKwritter::open(const char* filename, const char* comment)
{
	file.open(filename, std::ios_base::out);
	if (!file) {
		std::cerr << "Cannot open file [" << filename << "]" << endl;
		return false;
	}

	// Header
	file << "# vtk DataFile Version 3.0" << endl;
	file << comment << endl;
	file << "ASCII" << endl;
	file << "DATASET UNSTRUCTURED_GRID" << endl;
	file << endl;
	return true;
}

bool basicVTKwritter::close()
{
	file.close();
	return true;
}

void basicVTKwritter::begin_vertices() { file << "POINTS " << nbVertices << " float" << endl; }

void basicVTKwritter::begin_cells() { file << "CELLS " << nbCells << " " << 5 * nbCells << endl; }

void basicVTKwritter::write_point(const WriteType& x, const WriteType& y, const WriteType& z) { file << conv(x) << " " << conv(y) << " " << conv(z) << endl; }

// Note that identifiers must be defined with 0-offset
void basicVTKwritter::write_cell(unsigned int id1, unsigned int id2, unsigned int id3, unsigned int id4)
{
	file << "4 " << id1 << " " << id2 << " " << id3 << " " << id4 << endl;
}

void basicVTKwritter::end_vertices() { file << endl; }

void basicVTKwritter::end_cells()
{
	file << "CELL_TYPES " << nbCells << endl;
	for (unsigned int n = 0; n < nbCells; ++n) {
		file << "10" << endl;
	}
	file << endl;
}

void basicVTKwritter::begin_data(const char* dataname, DataPosition pos, DataName name, DataType type)
{
	switch (pos) {
		case POINT_DATA:
			if (!hasPointData) {
				file << "POINT_DATA " << nbVertices << endl;
				hasPointData = true;
			}
			break;
		case CELL_DATA:
			if (!hasCellData) {
				file << "CELL_DATA " << nbCells << endl;
				hasCellData = true;
			}
			break;
	}

	switch (name) {
		case SCALARS: file << "SCALARS " << dataname; break;
		case VECTORS: file << "VECTORS " << dataname; break;
		case TENSORS: file << "TENSORS " << dataname; break;
	}

	switch (type) {
		case INT: file << " int"; break;
		case FLOAT: file << " float"; break;
	}

	if (name == SCALARS) {
		file << " 1" << endl;
		file << "LOOKUP_TABLE default" << endl;
	} else
		file << endl;
}

void basicVTKwritter::write_data(const WriteType& value) { file << conv(value) << endl; }

void basicVTKwritter::write_data(const WriteType& x, const WriteType& y, const WriteType& z) { file << conv(x) << " " << conv(y) << " " << conv(z) << endl; }

void basicVTKwritter::write_data(
        const WriteType& t11,
        const WriteType& t12,
        const WriteType& t13,
        const WriteType& t21,
        const WriteType& t22,
        const WriteType& t23,
        const WriteType& t31,
        const WriteType& t32,
        const WriteType& t33)
{
	file << conv(t11) << " " << conv(t12) << " " << conv(t13) << endl;
	file << conv(t21) << " " << conv(t22) << " " << conv(t23) << endl;
	file << conv(t31) << " " << conv(t32) << " " << conv(t33) << endl;
	file << endl;
}

void basicVTKwritter::end_data() { file << endl; }
