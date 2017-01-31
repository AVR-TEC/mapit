#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <qqml.h>
#include <QQmlContext>
#include <QResource>
#include <QDebug>
#include "bindings/qmlmapsrenderviewport.h"
#include "bindings/qmlentitydata.h"
#include "upns_logging.h"
#include "stubs/qmlstubentitydatapointcloud2.h"
#include <pcl/io/ply_io.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/io/pcd_io.h>
#include <pcl/conversions.h>
#include <pcl/common/centroid.h>
#include <pcl/point_types.h>
#include <pcl/features/normal_3d.h>
#include <pcl/search/kdtree.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/octree/octree_pointcloud_pointvector.h>
#include <pcl/octree/octree_pointcloud.h>
#include <math.h>
#include <pcl/octree/octree_impl.h>
#include <QDir>
#include "controls/xboxcontroller.h"
#include "bindings/renderdata.h"
#include "bindings/qmlrepository.h"
#include "bindings/qmlcheckout.h"
#include "bindings/qmlcommit.h"
#include "bindings/qmltree.h"
#include "bindings/qmlentity.h"
#include "bindings/qmlentitydata.h"
#include "bindings/qmlentitydatatransform.h"
#include "bindings/qmlbranch.h"
#include "qpointcloud.h"
#include "qpointcloudgeometry.h"
#include "qpointfield.h"
#include "bindings/qmlentitydatarenderer.h"
#include "models/qmlroottreemodel.h"
#include "versioning/repositoryfactorystandard.h"

#include "services.pb.h"
#include "upns_errorcodes.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    upns_init_logging();
    //TODO: Use QGuiApplication when this bug is fixed: https://bugreports.qt.io/browse/QTBUG-39437
    QApplication app(argc, argv);

    po::options_description program_options_desc(std::string("Usage: ") + argv[0] + "");
    program_options_desc.add_options()
            ("help,h", "print usage");

    upns::RepositoryFactoryStandard::addProgramOptions(program_options_desc);
    po::variables_map vars;
    po::store(po::command_line_parser(argc, argv).options(program_options_desc).run(), vars);
    if(vars.count("help"))
    {
        std::cout << program_options_desc << std::endl;
        return 1;
    }
    po::notify(vars);

    upns::upnsSharedPointer<upns::Repository> repo( upns::RepositoryFactoryStandard::openRepository( vars ) );

    if(repo == nullptr)
    {
        log_error("Could not load Repository.");
        return 1;
    }
    qmlRegisterType<QmlMapsRenderViewport>("fhac.upns", 1, 0, "MapsRenderViewport");
    qmlRegisterUncreatableType<Renderdata>("fhac.upns", 1, 0, "Renderdata", "Can not create Renderdata");
    qmlRegisterType<QmlEntitydata>("fhac.upns", 1, 0, "Entitydata");
    //qmlRegisterType<QmlEntitydataPointcloud2>("fhac.upns", 1, 0, "EntitydataPointcloud2");
    qmlRegisterType<XBoxController>("fhac.upns", 1, 0, "XBoxController");
    //qmlRegisterUncreatableType<QmlEntity>("fhac.upns", 1, 0, "UpnsEntity", "Please add entities by using layer.addEntity()");

    qmlRegisterUncreatableType<QmlRepository>("fhac.upns", 1, 0, "Repository", "Not yet implemented. Uses RAII and must be wrapped to set all program_options for repo. Use global \"globalRepository\" for now.");
    //qmlRegisterType<QmlRepository>("fhac.upns", 1, 0, "Repository");
    qmlRegisterType<QmlCheckout>("fhac.upns", 1, 0, "Checkout");
    qmlRegisterType<QmlCommit>("fhac.upns", 1, 0, "Commit");
    qmlRegisterType<QmlTree>("fhac.upns", 1, 0, "Tree");
    qmlRegisterType<QmlEntity>("fhac.upns", 1, 0, "Entity");
    qmlRegisterType<QmlEntitydata>("fhac.upns", 1, 0, "Entitydata");
    qmlRegisterType<QmlBranch>("fhac.upns", 1, 0, "Branch");

    qmlRegisterType<QmlEntitydataTransform>("fhac.upns", 1, 0, "EntitydataTransform");

    qmlRegisterType<QmlRootTreeModel>("fhac.upns", 1, 0, "RootTreeModel");

    qmlRegisterType<QmlEntitydataRenderer>("fhac.upns", 1, 0, "EntitydataRenderer");
    qmlRegisterUncreatableType<QPointcloud>("pcl", 1, 0, "Pointcloud", "Please use factory method (not yet available).");
    qmlRegisterType<QPointcloudGeometry>("pcl", 1, 0, "PointcloudGeometry");
    qmlRegisterUncreatableType<QPointfield>("pcl", 1, 0, "Pointfield", "Please use factory method (not yet available).");
    QQmlApplicationEngine engine;
    QmlRepository *exampleRepo = new QmlRepository(repo, engine.rootContext());
    engine.rootContext()->setContextProperty("globalRepository", exampleRepo);
    engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));

    int result = app.exec();
    return result;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR argv, int argc)
{
//    int argc=1;
//    char *argv[] = {"temp"};
    return main(argc, &argv);
}
#endif
