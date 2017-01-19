#include "openvdblayer.h"
#include <sstream>
#include <openvdb/openvdb.h>
#include <openvdb/Grid.h>
#include <openvdb/io/io.h>
#include <openvdb/io/Stream.h>
#include "upns_logging.h"

void readFloatGridFromStream(std::istream &is, openvdb::FloatGrid::Ptr &grid)
{
    openvdb::initialize();
    openvdb::io::Stream strm(is);
    openvdb::GridPtrVecPtr grids(new openvdb::GridPtrVec);
    grids = strm.getGrids();
    if(grids->size() == 0 )
    {
        log_warn("no grids in file. Using empty Float Grid.");
    }
    if(grids->size() > 1 )
    {
        log_warn("multiple grids in file. Using only first Float Grid.");
    }
    openvdb::GridBase::Ptr gridBase(grids->at(0));
    openvdb::FloatGrid::Ptr gridFloat(openvdb::gridPtrCast<openvdb::FloatGrid>(gridBase));
    grid = gridFloat;
}

void writeFloatGridToStream(std::ostream &os, openvdb::FloatGrid::Ptr grid)
{
    openvdb::GridPtrVecPtr grids(new openvdb::GridPtrVec);
    grids->push_back(grid);
    openvdb::io::Stream strm(os);
    strm.write(*grids);
}


FloatGridEntitydata::FloatGridEntitydata(upnsSharedPointer<AbstractEntitydataStreamProvider> streamProvider)
    :m_streamProvider( streamProvider ),
     m_floatGrid( NULL )
{
}

LayerType FloatGridEntitydata::layerType() const
{
    return LayerType::OPENVDB;
}

bool FloatGridEntitydata::hasFixedGrid() const
{
    return true;
}

bool FloatGridEntitydata::canSaveRegions() const
{
    return false;
}

upnsFloatGridPtr FloatGridEntitydata::getData(upnsReal x1, upnsReal y1, upnsReal z1,
                                                upnsReal x2, upnsReal y2, upnsReal z2,
                                                bool clipMode,
                                                int lod)
{
    if(m_floatGrid == NULL)
    {
        upnsIStream *in = m_streamProvider->startRead();
        {
            readFloatGridFromStream( *in, m_floatGrid );
        }
        m_streamProvider->endRead(in);
    }
    return m_floatGrid;
}

int FloatGridEntitydata::setData(upnsReal x1, upnsReal y1, upnsReal z1,
                                 upnsReal x2, upnsReal y2, upnsReal z2,
                                 upnsFloatGridPtr &data,
                                 int lod)
{
    upnsOStream *out = m_streamProvider->startWrite();
    {
        writeFloatGridToStream( *out, data );
    }
    m_streamProvider->endWrite(out);
    return 0;
}

upnsFloatGridPtr FloatGridEntitydata::getData(int lod)
{
    return getData(-std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                   false, lod);
}

int FloatGridEntitydata::setData(upnsFloatGridPtr &data, int lod)
{
    return setData(-std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                   data, lod);
}

void FloatGridEntitydata::gridCellAt(upnsReal   x, upnsReal   y, upnsReal   z,
                                     upnsReal &x1, upnsReal &y1, upnsReal &z1,
                                     upnsReal &x2, upnsReal &y2, upnsReal &z2) const
{
    x1 = -std::numeric_limits<upnsReal>::infinity();
    y1 = -std::numeric_limits<upnsReal>::infinity();
    z1 = -std::numeric_limits<upnsReal>::infinity();
    x2 = +std::numeric_limits<upnsReal>::infinity();
    y2 = +std::numeric_limits<upnsReal>::infinity();
    z2 = +std::numeric_limits<upnsReal>::infinity();
}

int FloatGridEntitydata::getEntityBoundingBox(upnsReal &x1, upnsReal &y1, upnsReal &z1,
                                              upnsReal &x2, upnsReal &y2, upnsReal &z2)
{
    //TODO
    return 0;
}

upnsIStream *FloatGridEntitydata::startReadBytes(upnsuint64 start, upnsuint64 len)
{
    return m_streamProvider->startRead(start, len);
}

void FloatGridEntitydata::endRead(upnsIStream *strm)
{
    m_streamProvider->endRead(strm);
}

upnsOStream *FloatGridEntitydata::startWriteBytes(upnsuint64 start, upnsuint64 len)
{
    return m_streamProvider->startWrite(start, len);
}

void FloatGridEntitydata::endWrite(upnsOStream *strm)
{
    m_streamProvider->endWrite(strm);
}

size_t FloatGridEntitydata::size() const
{
    m_streamProvider->getStreamSize();
}

void deleteEntitydata(AbstractEntitydata *ld)
{
    FloatGridEntitydata *p = static_cast<FloatGridEntitydata*>(ld);
    delete p;
}
void createEntitydata(upnsSharedPointer<AbstractEntitydata> *out, upnsSharedPointer<AbstractEntitydataStreamProvider> streamProvider)
{
    *out = upnsSharedPointer<AbstractEntitydata>(new FloatGridEntitydata( streamProvider ), deleteEntitydata);
}

