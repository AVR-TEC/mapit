#ifndef TESTREPOSITORIESCOM_H
#define TESTREPOSITORIESCOM_H

#include <QTest>

#include "versioning/repository.h"

namespace upns {
class RepositoryServer;
}

class TestRepositoriesCommunication : public QObject
{
    Q_OBJECT
private slots:

    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();
    void testReadLocalComputeRemoteWriteLocal();
    void testReadRemoteComputeLocalWriteRemote();
    void testReadRemoteComputeRemoteWriteLocal();
private:
    upns::upnsSharedPointer<upns::Repository> initEmptyLocal();
    upns::upnsSharedPointer<upns::Repository> initNetwork(bool computeLocal);

    //// functions to execute on the repositories
    void readPcd(upns::Checkout *checkout);
    void voxelgrid(upns::Checkout* checkout);
    void writePcdLocalOnly(upns::Checkout* checkout, std::string filename);
    void compareFileToExpected(const char* filename);



    constexpr static float LEAF_SIZE = 0.01;
    constexpr static const char* FILENAME = "data/bunny.pcd";
    constexpr static const char* FILENAME_OUT1 = "bunny_voxelgrid_locally_read_remote_computed.pcd";
    constexpr static const char* FILENAME_OUT2 = "bunny_voxelgrid_remote_read_locally_computed.pcd";
    constexpr static const char* FILENAME_OUT3 = "bunny_voxelgrid_remote_read_remote_computed.pcd";
};

#endif