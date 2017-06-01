#include <upns/operators/module.h>
#include <upns/logging.h>
#include <upns/layertypes/lastype.h>
#include <upns/layertypes/lasentitydatawriter.h>
#include <upns/layertypes/pointcloudlayer.h>
#include <pcl/point_cloud.h>
#include <pcl/conversions.h>
#include <upns/operators/versioning/checkoutraw.h>
#include <upns/operators/operationenvironment.h>
#include <upns/operators/versioning/checkoutraw.h>
#include <iostream>
#include <memory>
#include <upns/errorcodes.h>
#include "liblas/liblas.hpp"
#include <fstream>
#include <iomanip>
#include <QJsonDocument>
#include <QJsonObject>

upns::StatusCode operate_pcd2las(upns::OperationEnvironment* env)
{
    QJsonDocument paramsDoc = QJsonDocument::fromJson( QByteArray(env->getParameters().c_str(), env->getParameters().length()) );
    QJsonObject params(paramsDoc.object());

    std::string input =  params["input"].toString().toStdString();
    std::string output = params["output"].toString().toStdString();
    if(input.empty())
    {
        input = params["target"].toString().toStdString();
        if(input.empty())
        {
            log_error("no input specified");
            return UPNS_STATUS_INVALID_ARGUMENT;
        }
    }
    if(output.empty())
    {
        output = params["target"].toString().toStdString();
        if(output.empty())
        {
            log_error("no output specified");
            return UPNS_STATUS_INVALID_ARGUMENT;
        }
    }

    bool demean = params["demean"].toBool();
    bool normalize = params["normalize"].toBool();
    double normalizeScale = params["normalizeScale"].toDouble();
    if(!normalize)
    {
        if(normalizeScale != 1.0)
        {
            log_warn("normalizeScale was set, but normalizization was not active");
        }
        normalizeScale = 1.0; // Not used
    }
    else if(normalizeScale < 0.001)
    {
        if(normalizeScale < 0.0)
        {
            log_error("normalizeScale was negative");
            return UPNS_STATUS_INVALID_ARGUMENT;
        }
        normalizeScale = 10.0; // 10m is default
    }

    std::cout << "normalize: " << normalizeScale << std::endl;

    std::shared_ptr<AbstractEntitydata> abstractEntitydataInput = env->getCheckout()->getEntitydataReadOnly( input );
    if(!abstractEntitydataInput)
    {
        log_error("input does not exist or is not readable.");
        return UPNS_STATUS_INVALID_ARGUMENT;
    }
    pcl::PointCloud<pcl::PointXYZINormal> pc;
    {
        std::shared_ptr<LASEntitydata> entityDataLASInput = std::dynamic_pointer_cast<LASEntitydata>( abstractEntitydataInput );
        if(entityDataLASInput == nullptr)
        {
            log_error("Wrong type");
            return UPNS_STATUS_ERR_DB_INVALID_ARGUMENT;
        }
        std::unique_ptr<LASEntitydataReader> reader = entityDataLASInput->getReader();

        const liblas::Header header = reader->GetHeader();
        uint32_t points = header.GetPointRecordsCount();

        pc.points.reserve(points);
        uint32_t i=0;
        if(demean || normalize)
        {
            double minX = header.GetMinX();
            double minY = header.GetMinY();
            double minZ = header.GetMinZ();
            double maxX = header.GetMaxX();
            double maxY = header.GetMaxY();
            double maxZ = header.GetMaxZ();

            double sx = maxX-minX;
            double sy = maxY-minY;
            double sz = maxZ-minZ;
            double maxDim = std::max(std::max(sx, sy), sz);
            double scale = normalize ? normalizeScale / maxDim : 1.0;

            double cx = minX+sx*0.5;
            double cy = minY+sy*0.5;
            double cz = minZ+sz*0.5;
            while(reader->ReadNextPoint())
            {
                liblas::Point current(reader->GetPoint());
                pcl::PointXYZINormal pclPoint;
                pclPoint.x = (current.GetX()-cx)*scale;
                pclPoint.y = (current.GetY()-cy)*scale;
                pclPoint.z = (current.GetZ()-cz)*scale;
                if(current.GetData().size() >= 3)
                {
                    pclPoint.normal_x = current.GetData().at(0);
                    pclPoint.normal_y = current.GetData().at(1);
                    pclPoint.normal_z = current.GetData().at(2);
                }
                pclPoint.intensity = current.GetIntensity();
                pc.points.push_back(pclPoint);
                ++i;
            }
        }
        else
        {
            while(reader->ReadNextPoint())
            {
                liblas::Point const& point = reader->GetPoint();
                pcl::PointXYZINormal pclPoint;
                pclPoint.x = point.GetX();
                pclPoint.y = point.GetY();
                pclPoint.z = point.GetZ();
                if(point.GetData().size() >= 3)
                {
                    pclPoint.normal_x = point.GetData().at(0);
                    pclPoint.normal_y = point.GetData().at(1);
                    pclPoint.normal_z = point.GetData().at(2);
                }
                pclPoint.intensity = point.GetIntensity();
                pc.points.push_back(pclPoint);
                i++;
            }
        }
        log_info("Looped " + std::to_string(i) + "points. Resulting PCD has " + std::to_string(points) + " points.");
        //dispose reader
    }
    std::shared_ptr<mapit::msgs::Entity> pclEntity(new mapit::msgs::Entity);
    pclEntity->set_type(PointcloudEntitydata::TYPENAME());
    StatusCode s = env->getCheckout()->storeEntity(output, pclEntity);
    if(!upnsIsOk(s))
    {
        log_error("Failed to create entity.");
    }
    std::shared_ptr<AbstractEntitydata> abstractEntitydataOutput = env->getCheckout()->getEntitydataForReadWrite( output );
    std::shared_ptr<PointcloudEntitydata> entityDataPCLOutput = std::dynamic_pointer_cast<PointcloudEntitydata>( abstractEntitydataOutput );
    if(entityDataPCLOutput == nullptr)
    {
        log_error("Wrong type");
        return UPNS_STATUS_ERR_DB_INVALID_ARGUMENT;
    }
    std::shared_ptr<pcl::PCLPointCloud2> cloud(new pcl::PCLPointCloud2);
    pcl::toPCLPointCloud2(pc, *cloud);

    entityDataPCLOutput->setData(cloud);

    return UPNS_STATUS_OK;
}

UPNS_MODULE(OPERATOR_NAME, "Loads a Las File", "fhac", OPERATOR_VERSION, LASEntitydata_TYPENAME, &operate_pcd2las)
