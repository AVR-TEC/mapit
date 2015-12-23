#ifndef __ABSTRACTLAYERDATA_H
#define __ABSTRACTLAYERDATA_H

#include "upns_globals.h"
#include "libs/upns_interface/services.pb.h"
#include <limits>

namespace upns
{

class AbstractEntityData;
class AbstractEntityDataStreamProvider;

extern "C"
{
/**
 * @brief The only method a layert_type-module must define. It must have the name "createLayerData".
 * NOTE: Deletion of an object must be done by the library implementing the concrete LayerData type. For this it is very
 * important that the returned shared pointer has a custom deleter. In most cases this deleter will only call "delete"
 * for the previously allocated LayerData. See \sa PointcloudLayerdata for an example.
 */
typedef upnsSharedPointer<AbstractEntityData> (*WrapLayerTypeFunc)(upnsSharedPointer<AbstractEntityDataStreamProvider> streamProvider);
}

/**
 * @brief The AbstractLayerData class is interface between a concrete layerdata implementation and layer. Basically an LayerData-Implementation will
 * translate/delegate requests of "getData" to LayerDataStreamProvider \sa AbstractEntityDataStreamProvider.
 * "setData" does not contain logic (e.g. registration)
 * This abstract interface can be used to query metainformation from any type of layer. For reading/writing data, see \sa LayerData
 */

class AbstractEntityData
{
public:
    virtual ~AbstractEntityData() { }
    /**
     * @brief layerType return specialized type
     * @return
     */
    virtual LayerType layerType() const = 0;

    /**
     * @brief hasFixedGrid indicated if the underlying datastructure likes to be read in predefined chunks
     * @return
     */
    virtual bool hasFixedGrid() const = 0;

    /**
     * @brief canSaveRegions indicates if the underlying datastrucuture is capable of saving specific regions without rewriting the whole layer.
     * If the datastructure does not support this, the boundingbox in "setData" might be ignored.
     * @return
     */
    virtual bool canSaveRegions() const = 0;

    /**
     * @brief gridCellAt if the datastructure "hasFixedGrid", this method retrieves the optimal regions to read at once for a given point
     * @param x input point x
     * @param y input point y
     * @param z input point z
     * @param x1 output axis aligned bounding box lower x
     * @param y1 output axis aligned bounding box lower y
     * @param z1 output axis aligned bounding box lower z
     * @param x2 output axis aligned bounding box upper x
     * @param y2 output axis aligned bounding box upper y
     * @param z2 output axis aligned bounding box upper z
     */
    virtual void gridCellAt(upnsReal x, upnsReal y, upnsReal z, upnsReal &x1, upnsReal &y1, upnsReal &z1,upnsReal &x2, upnsReal &y2, upnsReal &z2) const = 0;
};

}
#endif