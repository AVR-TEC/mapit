#include "qmlentitydatarenderer.h"
#include "abstractentitydata.h"
#include "libs/layertypes_collection/pointcloud2/include/pointcloudlayer.h"
#include "qpointcloudgeometry.h"
#include "qpointcloud.h"

QmlEntitydataRenderer::QmlEntitydataRenderer(Qt3DCore::QNode *parent)
    : QGeometryRenderer(parent)
{
    QPointcloudGeometry *geometry = new QPointcloudGeometry(this);
    QGeometryRenderer::setGeometry(geometry);
}

QmlEntitydata *QmlEntitydataRenderer::entitydata() const
{
    return m_entitydata;
}

void QmlEntitydataRenderer::setEntitydata(QmlEntitydata *entitydata)
{
    if (m_entitydata != entitydata)
    {
        if(m_entitydata)
        {
            disconnect(m_entitydata, &QmlEntitydata::internalEntitydataChanged, this, &QmlEntitydataRenderer::setEntitydata);
        }
        m_entitydata = entitydata;
        if(m_entitydata)
        {
            connect(m_entitydata, &QmlEntitydata::internalEntitydataChanged, this, &QmlEntitydataRenderer::setEntitydata);
        }
    }
    updateGeometry();
    Q_EMIT entitydataChanged(entitydata);
}

void QmlEntitydataRenderer::updateGeometry()
{
    upns::upnsSharedPointer<upns::AbstractEntityData> ed = m_entitydata->getEntityData();

    if(!ed) return;

    if(geometry() != NULL)
    {
        geometry()->deleteLater();
    }

    switch(ed->layerType())
    {
    case upns::POINTCLOUD2:
    {
        QGeometryRenderer::setGeometry(new QPointcloudGeometry(this));
        QPointcloud *pointcloud(new QPointcloud(this));
        *pointcloud->pointcloud() = *upns::static_pointer_cast< PointcloudEntitydata >(ed)->getData();
        static_cast<QPointcloudGeometry *>(geometry())->setPointcloud(pointcloud);
        QGeometryRenderer::setPrimitiveType(QGeometryRenderer::Points);
        break;
    }
    case upns::OCTOMAP:
        break;
    case upns::OPENVDB:
        break;
    case upns::POSES:
        break;
    case upns::NONE:
    default:
        break;
    }
}