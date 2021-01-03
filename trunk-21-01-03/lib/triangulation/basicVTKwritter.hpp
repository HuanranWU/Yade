#pragma once
#include <lib/high-precision/RealIO.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>

enum DataPosition { POINT_DATA, CELL_DATA };
enum DataName { SCALARS, VECTORS, TENSORS };
enum DataType { INT, FLOAT };

// Really simplistic struct for vtk file creation
struct basicVTKwritter {
	using WriteType = ::yade::Real; // maybe later turn this class into a template class

	std::ofstream file;
	unsigned int  nbVertices, nbCells;
	bool          hasPointData;
	bool          hasCellData;


	basicVTKwritter(unsigned int nV, unsigned int nC)
	        : nbVertices(nV)
	        , nbCells(nC)
	        , hasPointData(false)
	        , hasCellData(false)
	{
	}
	void setNums(unsigned int nV, unsigned int nC)
	{
		nbVertices = nV;
		nbCells    = nC;
	}
	bool open(const char* filename, const char* comment);
	bool close();

	void begin_vertices();
	void write_point(const WriteType& x, const WriteType& y, const WriteType& z);

	void end_vertices();

	void begin_cells();
	void write_cell(unsigned int id1, unsigned int id2, unsigned int id3, unsigned int id4);
	void end_cells();

	void begin_data(const char* dataname, DataPosition pos, DataName name, DataType type);
	void write_data(const WriteType& value);
	void write_data(const WriteType& x, const WriteType& y, const WriteType& z);
	void write_data(
	        const WriteType& t11,
	        const WriteType& t12,
	        const WriteType& t13,
	        const WriteType& t21,
	        const WriteType& t22,
	        const WriteType& t23,
	        const WriteType& t31,
	        const WriteType& t32,
	        const WriteType& t33);
	void end_data();

private:
	std::string conv(const WriteType& v) { return ::yade::math::toString(v); }
};
