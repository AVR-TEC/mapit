#include "upns/layertypes/boundingboxlayer.h"
#include <sstream>
#include <upns/logging.h>

const char *BoundingboxEntitydata::TYPENAME()
{
    return PROJECT_NAME;
}


BoundingboxEntitydata::BoundingboxEntitydata(std::shared_ptr<AbstractEntitydataProvider> streamProvider)
    :m_streamProvider( streamProvider ),
     m_aabb( NULL )
{
}

const char* BoundingboxEntitydata::type() const
{
    return BoundingboxEntitydata::TYPENAME();
}

bool BoundingboxEntitydata::hasFixedGrid() const
{
    return false;
}

bool BoundingboxEntitydata::canSaveRegions() const
{
    return false;
}

BoundingboxPtr BoundingboxEntitydata::getData(upnsReal x1, upnsReal y1, upnsReal z1,
                                                upnsReal x2, upnsReal y2, upnsReal z2,
                                                bool clipMode,
                                                int lod)
{
    if(m_aabb == NULL)
    {
        m_aabb = BoundingboxPtr(new Boundingbox);
        upnsIStream *in = m_streamProvider->startRead();
        {
            if(!m_aabb->ParseFromIstream(in))
            {
                log_warn("Could not read boundingbox from stream. Proceeding with identity aabb (-1,1,1,-1)");
                m_aabb->set_x1(-1); // left
                m_aabb->set_y1(1); // top
                m_aabb->set_x2(1); // right
                m_aabb->set_y2(-1); // bottom
            }
        }
        m_streamProvider->endRead(in);
    }
    return m_aabb;
}

int BoundingboxEntitydata::setData(upnsReal x1, upnsReal y1, upnsReal z1,
                                 upnsReal x2, upnsReal y2, upnsReal z2,
                                 BoundingboxPtr &data,
                                 int lod)
{
    upnsOStream *out = m_streamProvider->startWrite();
    {
        m_aabb->SerializePartialToOstream(out);
    }
    m_streamProvider->endWrite(out);
	return 0; //TODO: MSVC: What to return here?
}

BoundingboxPtr BoundingboxEntitydata::getData(int lod)
{
    return getData(-std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                   false, lod);
}

int BoundingboxEntitydata::setData(BoundingboxPtr &data, int lod)
{
    return setData(-std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                   -std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                    std::numeric_limits<upnsReal>::infinity(),
                   data, lod);
}

void BoundingboxEntitydata::gridCellAt(upnsReal   x, upnsReal   y, upnsReal   z,
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

int BoundingboxEntitydata::getEntityBoundingBox(upnsReal &x1, upnsReal &y1, upnsReal &z1,
                                              upnsReal &x2, upnsReal &y2, upnsReal &z2)
{
    //TODO
    return 0;
}

upnsIStream *BoundingboxEntitydata::startReadBytes(upnsuint64 start, upnsuint64 len)
{
    return m_streamProvider->startRead(start, len);
}

void BoundingboxEntitydata::endRead(upnsIStream *strm)
{
    m_streamProvider->endRead(strm);
}

upnsOStream *BoundingboxEntitydata::startWriteBytes(upnsuint64 start, upnsuint64 len)
{
    return m_streamProvider->startWrite(start, len);
}

void BoundingboxEntitydata::endWrite(upnsOStream *strm)
{
    m_streamProvider->endWrite(strm);
}

size_t BoundingboxEntitydata::size() const
{
    return m_streamProvider->getStreamSize();
}

// Win32 does not like anything but void pointers handled between libraries
// For Unix there would be a hack to use a "custom deleter" which is given to the library to clean up the created memory
// the common denominator is to build pointer with custom deleter in our main programm and just exchange void pointers.
//std::shared_ptr<AbstractEntitydata> createEntitydata(std::shared_ptr<AbstractEntitydataProvider> streamProvider)
//void* createEntitydata(std::shared_ptr<AbstractEntitydataProvider> streamProvider)
void deleteEntitydata(AbstractEntitydata *ld)
{
    BoundingboxEntitydata *p = static_cast<BoundingboxEntitydata*>(ld);
    delete p;
}

void createEntitydata(std::shared_ptr<AbstractEntitydata> *out, std::shared_ptr<AbstractEntitydataProvider> streamProvider)
{
    //return std::shared_ptr<AbstractEntitydata>(new PointcloudEntitydata( streamProvider ), deleteWrappedLayerData);
    *out = std::shared_ptr<AbstractEntitydata>(new BoundingboxEntitydata( streamProvider ), deleteEntitydata);
}

