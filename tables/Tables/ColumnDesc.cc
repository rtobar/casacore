//# ColumnDesc.cc: Envelope class for description of a table column
//# Copyright (C) 1994,1995,1996,1997,1998,2001,2016
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

#include <casacore/tables/Tables/ColumnDesc.h>
#include <casacore/tables/Tables/ScaColDesc.h>
#include <casacore/tables/Tables/ScaRecordColDesc.h>
#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/SubTabDesc.h>
#include <casacore/tables/Tables/TableRecord.h>
#include <casacore/tables/Tables/TableAttr.h>
#include <casacore/casa/Containers/RecordDesc.h>
#include <casacore/casa/BasicSL/Complex.h>
#include <casacore/casa/Utilities/DataType.h>
#include <casacore/casa/IO/AipsIO.h>
#include <casacore/tables/Tables/TableError.h>
#include <casacore/tables/Tables/TableRecord.h>
#include <casacore/casa/Utilities/Assert.h>
#include <casacore/casa/iostream.h>


namespace casacore { //# NAMESPACE CASACORE - BEGIN

ColumnDesc::ColumnDesc (const BaseColumnDesc& cold)
: colPtr_p   (cold.clone()),
  allocated_p(True)
{}

ColumnDesc::ColumnDesc (const ColumnDesc& that)
: colPtr_p   (that.colPtr_p),
  allocated_p(True)
{
  if (colPtr_p != 0) {
      colPtr_p = colPtr_p->clone();
  }
}

ColumnDesc::ColumnDesc (BaseColumnDesc* bcdp)
: colPtr_p   (bcdp),
  allocated_p(False)
{}

ColumnDesc::~ColumnDesc()
{
    if (allocated_p) {
	delete colPtr_p;
    }
}


ColumnDesc& ColumnDesc::operator= (const ColumnDesc& that)
{
    if (this != &that) {
	if (allocated_p) {
	    delete colPtr_p;
	}
	colPtr_p = that.colPtr_p;
	if (colPtr_p != 0) {
	    colPtr_p = colPtr_p->clone();
	}
	allocated_p = True;
    }
    return *this;
}


Bool ColumnDesc::operator== (const ColumnDesc& that) const
{
    if (dataType() != that.dataType())
	return False;
    if (options() != that.options())
	return False;
    if (ndim() != that.ndim())
	return False;
    if (isScalar() && that.isScalar())
	return True;
    if (isArray() && that.isArray())
	return True;
    if (isTable() && that.isTable())
	return True;
    return False;
}

Bool ColumnDesc::operator!= (const ColumnDesc& that) const
{
    return !(*this == that);
}


Bool ColumnDesc::isFixedShape() const
{
    if (isScalar()) {
	return True;
    }
    if ((options() & ColumnDesc::FixedShape)  ==  ColumnDesc::FixedShape) {
	return True;
    }
    return False;
}


DataType ColumnDesc::trueDataType() const
{
    DataType dtype = dataType();
    if (! isArray()) {
        return dtype;
    }
    switch (dtype) {
    case TpBool:
	return TpArrayBool;
    case TpChar:
	return TpArrayChar;
    case TpUChar:
	return TpArrayUChar;
    case TpShort:
	return TpArrayShort;
    case TpUShort:
	return TpArrayUShort;
    case TpInt:
	return TpArrayInt;
    case TpUInt:
	return TpArrayUInt;
    case TpInt64:
	return TpArrayInt64;
    case TpFloat:
	return TpArrayFloat;
    case TpDouble:
	return TpArrayDouble;
    case TpComplex:
	return TpArrayComplex;
    case TpDComplex:
	return TpArrayDComplex;
    case TpString:
	return TpArrayString;
    default:
	AlwaysAssert (False, AipsError);
    }
    return TpOther;
}


// Return the column name.
const String& ColumnDesc::name() const
    { return colPtr_p->name(); }


AipsIO& operator<< (AipsIO& ios, const ColumnDesc& cd)
{
    cd.putFile (ios, TableAttr());
    return ios;
}

AipsIO& operator>> (AipsIO& ios, ColumnDesc& cd)
{
    cd.getFile(ios, TableAttr());
    return ios;
}

//# Put into AipsIO.
//# It was felt that putstart takes too much space, so therefore
//# the version is put "manually".
void ColumnDesc::putFile (AipsIO& ios, const TableAttr& parentAttr) const
{
    ios << (uInt)1;                  // class version 1
    //# First write the exact column type, then its data.
    ios << colPtr_p->className();
    colPtr_p->putFile (ios, parentAttr);
}

//# Get from AipsIO.
void ColumnDesc::getFile (AipsIO& ios, const TableAttr& parentAttr)
{
    uInt version;
    ios >> version;
    String tp;
    ios >> tp;
    if (allocated_p) {
	delete colPtr_p;
    }
    allocated_p = True;

    // If tp is not in the map, (tp, unknownColumnDesc) is added and called (throws).
    ColumnDesc::ColumnDescCtor cdFunc;
    {
        ScopedMutexLock lock(theirMutex);
        cdFunc = theirRegisterMap(tp);
    }
    colPtr_p = (*cdFunc)(tp);
    colPtr_p->getFile (ios, parentAttr);
}


ostream& operator<< (ostream& ios, const ColumnDesc& cd)
{
    cd.show (ios);
    return ios;
}

void ColumnDesc::show() const
{
    show (cout);
}

void ColumnDesc::show (ostream& os) const
{
    if (colPtr_p) {
        colPtr_p->show (os);
        os << "   #keywords=" << keywordSet().nfields() << endl;
        os << keywordSet().description();
    } else {
        os << "ColumnDesc is empty" << endl;
    }
}

//# Initialize the static variables for the class registration.
SimpleOrderedMap<String, ColumnDesc::ColumnDescCtor>
        ColumnDesc::theirRegisterMap(initRegisterMap());
Mutex ColumnDesc::theirMutex;

//# The default "ctor" function for unknown types.
BaseColumnDesc* ColumnDesc::unknownColumnDesc (const String& name)
{
    throw (TableUnknownDesc(name));
    return 0;
}

//# Register a mapping.
void ColumnDesc::registerCtor (const String& name,
                               ColumnDesc::ColumnDescCtor func)
{
    ScopedMutexLock lock(theirMutex);
    theirRegisterMap.define(name, func);
}

//# Get a ColumnDesc constructor.
//# Return default function if undefined.
ColumnDesc::ColumnDescCtor ColumnDesc::getCtor (const String& name)
{
    ScopedMutexLock lock(theirMutex);
    return *(theirRegisterMap.isDefined (name));
}

//# Register the main "static constructors" of all XColumnDesc classes.
// No locking since private and only called by ctor of static member init.
SimpleOrderedMap<String, ColumnDesc::ColumnDescCtor> ColumnDesc::initRegisterMap()
{
  SimpleOrderedMap<String, ColumnDesc::ColumnDescCtor> regMap(ColumnDesc::unknownColumnDesc);

  ScalarColumnDesc<Bool>     scdb("x");
  regMap.define(scdb.className(), scdb.makeDesc);
  ScalarColumnDesc<uChar>    scduc("x");
  regMap.define(scduc.className(), scduc.makeDesc);
  ScalarColumnDesc<Short>    scds("x");
  regMap.define(scds.className(), scds.makeDesc);
  ScalarColumnDesc<uShort>   scdus("x");
  regMap.define(scdus.className(), scdus.makeDesc);
  ScalarColumnDesc<Int>      scdi("x");
  regMap.define(scdi.className(), scdi.makeDesc);
  ScalarColumnDesc<uInt>     scdui("x");
  regMap.define(scdui.className(), scdui.makeDesc);
  ScalarColumnDesc<Int64>    scdi64("x");
  regMap.define(scdi64.className(), scdi64.makeDesc);
  ScalarColumnDesc<float>    scdf("x");
  regMap.define(scdf.className(), scdf.makeDesc);
  ScalarColumnDesc<double>   scdd("x");
  regMap.define(scdd.className(), scdd.makeDesc);
  ScalarColumnDesc<Complex>  scdcx("x");
  regMap.define(scdcx.className(), scdcx.makeDesc);
  ScalarColumnDesc<DComplex> scddx("x");
  regMap.define(scddx.className(), scddx.makeDesc);
  ScalarColumnDesc<String>   scdst("x");
  regMap.define(scdst.className(), scdst.makeDesc);

  ScalarRecordColumnDesc     srcd ("x");
  regMap.define(srcd.className(), srcd.makeDesc);

  ArrayColumnDesc<Bool>     acdb("x");
  regMap.define(acdb.className(), acdb.makeDesc);
  ArrayColumnDesc<uChar>    acduc("x");
  regMap.define(acduc.className(), acduc.makeDesc);
  ArrayColumnDesc<Short>    acds("x");
  regMap.define(acds.className(), acds.makeDesc);
  ArrayColumnDesc<uShort>   acdus("x");
  regMap.define(acdus.className(), acdus.makeDesc);
  ArrayColumnDesc<Int>      acdi("x");
  regMap.define(acdi.className(), acdi.makeDesc);
  ArrayColumnDesc<uInt>     acdui("x");
  regMap.define(acdui.className(), acdui.makeDesc);
  ArrayColumnDesc<Int64>    acdi64("x");
  regMap.define(acdi64.className(), acdi64.makeDesc);
  ArrayColumnDesc<float>    acdf("x");
  regMap.define(acdf.className(), acdf.makeDesc);
  ArrayColumnDesc<double>   acdd("x");
  regMap.define(acdd.className(), acdd.makeDesc);
  ArrayColumnDesc<Complex>  acdcx("x");
  regMap.define(acdcx.className(), acdcx.makeDesc);
  ArrayColumnDesc<DComplex> acddx("x");
  regMap.define(acddx.className(), acddx.makeDesc);
  ArrayColumnDesc<String>   acdst("x");
  regMap.define(acdst.className(), acdst.makeDesc);

  SubTableDesc std("x", "", TableDesc());
  regMap.define(std.className(), std.makeDesc);

  return regMap;
}

} //# NAMESPACE CASACORE - END
