#ifndef REPOSITORYFACTORY_H
#define REPOSITORYFACTORY_H

#include <upns/typedefs.h>
#include <upns/versioning/repository.h>

namespace upns
{
class RepositoryFactory
{
public:
    /**
     * @brief openLocalRepository. Opens a Repository on disc or creates an empty repository.
     * @param directory for repository
     * It communicates directly to file serializer.
     * @return
     */
    static upns::Repository *openLocalRepository(std::string directory);
};

}
#endif