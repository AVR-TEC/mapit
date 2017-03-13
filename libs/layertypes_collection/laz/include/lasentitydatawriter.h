#ifndef LASENTITYWRITER_H
#define LASENTITYWRITER_H

#include "liblas/liblas.hpp"
#include "laztype.h"
#include "modules/serialization/abstractentitydatastreamprovider.h"

class LASEntitydataWriterPrivate;
class LASEntitydataWriter /* acts like : public liblas::Writer */
{
public:
    liblas::Header const& GetHeader() const;
    void SetHeader(liblas::Header const& header);
    bool WritePoint(liblas::Point const& point);
    void WriteHeader();
    void SetFilters(std::vector<liblas::FilterPtr> const& filters);
    std::vector<liblas::FilterPtr> GetFilters() const;
    void SetTransforms(std::vector<liblas::TransformPtr> const& transforms);
    std::vector<liblas::TransformPtr> GetTransforms() const;
private:
    LASEntitydataWriterPrivate *m_pimpl;
    LASEntitydataWriter(std::shared_ptr<upns::AbstractEntitydataStreamProvider> prov, liblas::Header const& header);
    friend class LASEntitydata;
};

#endif