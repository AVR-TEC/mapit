#include "qmlentitydatarenderer.h"
#include <upns/abstractentitydata.h>
#include "libs/layertypes_collection/pointcloud2/include/pointcloudlayer.h"
#include "libs/layertypes_collection/tf/include/tflayer.h"
#include "qpointcloudgeometry.h"
#include "qpointcloud.h"
#include <QMatrix4x4>

QmlEntitydataRenderer::QmlEntitydataRenderer(Qt3DCore::QNode *parent)
    : QGeometryRenderer(parent),
      m_entitydata(NULL)
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
    upns::upnsSharedPointer<upns::AbstractEntitydata> ed = m_entitydata->getEntitydata();

    if(!ed) return;

    if(geometry() != NULL)
    {
        geometry()->deleteLater();
    }

    if(strcmp(ed->type(), PointcloudEntitydata::TYPENAME()) == 0)
    {
        QGeometryRenderer::setGeometry(new QPointcloudGeometry(this));
        QPointcloud *pointcloud(new QPointcloud(this));
        *pointcloud->pointcloud() = *upns::static_pointer_cast< PointcloudEntitydata >(ed)->getData();
        static_cast<QPointcloudGeometry *>(geometry())->setPointcloud(pointcloud);
        QGeometryRenderer::setPrimitiveType(QGeometryRenderer::Points);
    }
    else if(strcmp(ed->type(), PointcloudEntitydata::TYPENAME()) == 0)
    {
    }
//    else if(strcmp(ed->type(), OctomapEntitydata::TYPENAME()) == 0)
//    {
//    }
    else if(strcmp(ed->type(), TfEntitydata::TYPENAME()) == 0)
    {
        QMatrix4x4 mat( upns::static_pointer_cast< TfEntitydata >(ed)->getData()->data() );
        qDebug() << mat;
    }
//    else if(strcmp(ed->type(), AssetEntitydata::TYPENAME()) == 0)
//    {
//    }
    else
    {
        qDebug() << "unknown entitytype for visualization";
    }

}
