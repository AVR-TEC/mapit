#include "module.h"
#include <iostream>

int operate(void* layer)
{
    std::cout << "Works";
    return 0;
}

UPNS_MODULE("loadPointcloud", "Loads a Pcd File", "fhac", 1, upns::LayerType::POINTCLOUD2, &operate)
