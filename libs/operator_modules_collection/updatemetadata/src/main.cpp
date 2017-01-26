#include "module.h"
#include "libs/layertypes_collection/pointcloud2/include/pointcloudlayer.h"
#include "modules/versioning/checkoutraw.h"
#include "modules/operationenvironment.h"
#include <iostream>
#include <pcl/filters/voxel_grid.h>
#include <memory>
#include "module_utility.h"
#include "upns_errorcodes.h"

upns::StatusCode operate(upns::OperationEnvironment* env)
{
//    //// Read Target Map, Layer or Entity ////
//    const OperationParameter* target = env->getParameter("target");

//    upnsSharedPointer<Tree> map = parseMap(env, target);
//    if(map == NULL)
//    {
//        std::stringstream strm;
//        strm << "Map not found: " << target->mapval();
//        log_error(strm.str());
//        return UPNS_STATUS_MAP_NOT_FOUND;
//    }
//    upnsSharedPointer<Tree> layer = parseLayer(env, target);
//    upnsSharedPointer<Entity> entity;
//    if( layer != NULL )
//    {
//        entity = parseEntity(env, target);
//    }

//    //// Read properties to be updated ////
//    std::string name;
//    LayerType layerType;
//    LayerUsageType layerUsageType;
//    bool updateName = false,
//         updateLayerType = false,
//         updateLayerUsageType = false;

//    const OperationParameter* param = env->getParameter("name");
//    if(param != NULL)
//    {
//        name = param->strval();
//        updateName = true;
//    }
//    param = env->getParameter("layertype");
//    if(param != NULL)
//    {
//        //TODO: big if else...
//        //layerType = param->strval();
//        updateLayerType = true;
//    }
//    param = env->getParameter("layerusagetype");
//    if(param != NULL)
//    {
//        //TODO: big if else...
//        //layerUsageType = param->strval();
//        updateLayerUsageType = true;
//    }
//    if(entity != NULL)
//    {
//        // entity->set_
//    }
//    else if(layer != NULL)
//    {
//        if(updateName)
//        {
//            //TODO: set name in parent
//            //layer->set_name(name);
//        }
//        if(updateLayerType)
//        {
//            //TODO
//        }
//        if(updateLayerUsageType)
//        {
//            //TODO
//        }
//    }
//    else if(map != NULL)
//    {
//        if(updateName)
//        {
//            //TODO: set name in parent
//            //map->set_name(name);
//        }
//    }
//    //// Store ////
//    env->getCheckout()->storeTree("//TODO/PATH", map);

//    //TODO: this is filled automatically
//    //// Fill return Info ////
//    OperationDescription out;
//    out.set_operatorname(OPERATOR_NAME);
//    out.set_operatorversion(OPERATOR_VERSION);
//    OperationParameter *outTarget = out.add_params();
//    outTarget->set_key("target");
////    outTarget->set_mapval( map?map->id():0 );
////    outTarget->set_layerval( layer?layer->id():0 );
////    outTarget->set_entityval( entity?entity->id():0 );
//    OperationParameter *outMapname = out.add_params();
//    outMapname->set_key("name");
//    outMapname->set_strval( name );
//    env->setOutputDescription( out );
    return UPNS_STATUS_OK;
}

UPNS_MODULE(OPERATOR_NAME, "updates name of map, layer, ...", "fhac", OPERATOR_VERSION, "none", &operate)
