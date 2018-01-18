#include <upns/operators/module.h>
#include <upns/logging.h>
#include <upns/layertypes/pointcloudlayer.h>
#include <upns/operators/versioning/checkoutraw.h>
#include <upns/operators/operationenvironment.h>
#include <iostream>
#include <pcl/filters/voxel_grid.h>
#include <memory>
#include <upns/errorcodes.h>
#include <upns/operators/versioning/checkoutraw.h>
#include <upns/depthfirstsearch.h>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

upns::StatusCode executeVoxelgrid(upns::OperationEnvironment* env, const std::string& target, const double& leafSize)
{
    std::shared_ptr<AbstractEntitydata> abstractEntitydata = env->getCheckout()->getEntitydataForReadWrite( target );
    std::shared_ptr<PointcloudEntitydata> entityData = std::dynamic_pointer_cast<PointcloudEntitydata>( abstractEntitydata );
    if(entityData == nullptr)
    {
        log_error("Wrong type");
        return UPNS_STATUS_ERR_DB_INVALID_ARGUMENT;
    }
    upnsPointcloud2Ptr pc2 = entityData->getData();

    pcl::VoxelGrid<pcl::PCLPointCloud2> sor;
    pcl::PCLPointCloud2ConstPtr stdPc2( pc2.get(), [](pcl::PCLPointCloud2*){});
    sor.setInputCloud(stdPc2);
    sor.setLeafSize (leafSize, leafSize, leafSize);

    upnsPointcloud2Ptr cloud_filtered(new pcl::PCLPointCloud2 ());
    sor.filter (*cloud_filtered);
    std::stringstream strstr;
    strstr << "new pointcloudsize " << cloud_filtered->width << "(leafsize: " << leafSize << ")";
    log_info( strstr.str() );

    entityData->setData(cloud_filtered);

    return UPNS_STATUS_OK;
}

upns::StatusCode operate_vxg(upns::OperationEnvironment* env)
{
    QJsonDocument paramsDoc = QJsonDocument::fromJson( QByteArray(env->getParameters().c_str(), env->getParameters().length()) );
    log_info( "Voxelgrid params:" + env->getParameters() );
    QJsonObject params(paramsDoc.object());
    double leafSize = params["leafsize"].toDouble();

    if(leafSize == 0.0)
    {
        log_info( "Leafsize was 0, using 0.01" );
        leafSize = 0.01f;
    }

    std::string target = params["target"].toString().toStdString();

    if ( env->getCheckout()->getEntity(target) ) {
        // execute on entity
        return executeVoxelgrid(env, target, leafSize);
    } else if ( env->getCheckout()->getTree(target) ) {
        // execute on tree
        upns::StatusCode status = UPNS_STATUS_OK;
        env->getCheckout()->depthFirstSearch(
                      target
                    , depthFirstSearchAll(mapit::msgs::Tree)
                    , depthFirstSearchAll(mapit::msgs::Tree)
                    , [&](std::shared_ptr<mapit::msgs::Entity> obj, const ObjectReference& ref, const upns::Path &path)
                        {
                            status = executeVoxelgrid(env, path, leafSize);
                            if ( ! upnsIsOk(status) ) {
                                return false;
                            }
                            return true;
                        }
                    , depthFirstSearchAll(mapit::msgs::Entity)
                    );
        return status;
    } else {
        log_error("operator voxelgrid: target is neither a tree nor entity");
        return UPNS_STATUS_ERR_DB_INVALID_ARGUMENT;
    }
}

UPNS_MODULE(OPERATOR_NAME, "use pcl voxelgrid filter on a pointcloud", "fhac", OPERATOR_VERSION, PointcloudEntitydata_TYPENAME, &operate_vxg)
