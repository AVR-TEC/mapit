/*******************************************************************************
 *
 * Copyright 2015-2018 Daniel Bulla	<d.bulla@fh-aachen.de>
 *           2015-2017 Tobias Neumann	<t.neumann@fh-aachen.de>
 *
******************************************************************************/

/*  This file is part of mapit.
 *
 *  Mapit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mapit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with mapit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <qqml.h>
#include <QQmlContext>
#include <QResource>
#include <QDebug>
#include <mapit/ui/bindings/qmlmapsrenderviewport.h>
#include <mapit/ui/bindings/qmlentitydata.h>
#include <mapit/logging.h>
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
#include <mapit/ui/bindings/renderdata.h>
#include <mapit/ui/bindings/qmlrepository.h>
#include <mapit/ui/bindings/qmlworkspace.h>
#include <mapit/ui/bindings/qmlcommit.h>
#include <mapit/ui/bindings/qmltree.h>
#include <mapit/ui/bindings/qmlentity.h>
#include <mapit/ui/bindings/qmlentitydata.h>
#include <mapit/ui/bindings/qmltransform.h>
#include <mapit/ui/bindings/qmlbranch.h>
#include <mapit/ui/bindings/qmlrepositoryserver.h>
#include "qpointcloud.h"
#include "qpointcloudgeometry.h"
#include "qpointfield.h"
#include <mapit/ui/bindings/qmlentitydatarenderer.h>
#include <mapit/ui/models/qmlroottreemodel.h>
#include <mapit/versioning/repositoryfactorystandard.h>
#include <mapit/ui/bindings/qmlpointcloudcoordinatesystem.h>
#include <qmlraycast.h>

#include <mapit/msgs/services.pb.h>
#include <mapit/errorcodes.h>

#include "inputcontrols/editorcameracontroller.h"

#include <boost/program_options.hpp>
#include "iconimageprovider.h"
#include "fileio.h"

#include <Qt3DQuick/Qt3DQuick>
#include <Qt3DInput/QInputSettings>

#include "imageupdater.h"

#ifndef NO_GRAPHBLOCKS
#include <graphblocks.h>
#endif

#include "mouseeventfilter.h"


namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    mapit_init_logging();
    //TODO: Use QGuiApplication when this bug is fixed: https://bugreports.qt.io/browse/QTBUG-39437
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(mapit_visualization);
    Q_INIT_RESOURCE(mapit_visualization_standalone);
    app.setOrganizationName("Fachhochschule Aachen");
    app.setOrganizationDomain("fh-aachen.de");
    app.setApplicationName("Mapit Viewer");

    po::options_description program_options_desc(std::string("Usage: ") + argv[0] + "");
    program_options_desc.add_options()
            ("help,h", "print usage");

    mapit::RepositoryFactoryStandard::addProgramOptions(program_options_desc);
    po::variables_map vars;
    po::store(po::command_line_parser(argc, argv).options(program_options_desc).run(), vars);
    if(vars.count("help"))
    {
        std::cout << program_options_desc << std::endl;
        return 1;
    }
    po::notify(vars);
    bool specified;
    std::shared_ptr<mapit::Repository> repo( mapit::RepositoryFactoryStandard::openRepository( vars, &specified ) );

    if(repo == nullptr)
    {
        log_error("Could not load Repository.");
        return 1;
    }

#ifndef NO_GRAPHBLOCKS
    GraphblocksContext* graphblockCtx = initializeGraphBlocks();
#endif

    qmlRegisterType<QmlRaycast>("fhac.mapit", 1, 0, "Raycast");
//    qmlRegisterType<XBoxController>("fhac.mapit", 1, 0, "XBoxController");

    qmlRegisterType<QmlRepository>("fhac.mapit", 1, 0, "Repository");
    qmlRegisterType<QmlWorkspace>("fhac.mapit", 1, 0, "Workspace");
    qmlRegisterType<QmlCommit>("fhac.mapit", 1, 0, "Commit");
    qmlRegisterType<QmlTree>("fhac.mapit", 1, 0, "Tree");
    qmlRegisterType<QmlEntity>("fhac.mapit", 1, 0, "Entity");
    qmlRegisterType<QmlEntitydata>("fhac.mapit", 1, 0, "Entitydata");
    qmlRegisterType<QmlBranch>("fhac.mapit", 1, 0, "Branch");
    qmlRegisterType<QmlRepositoryServer>("fhac.mapit", 1, 0, "RepositoryServer");
    qmlRegisterUncreatableType<MouseEventFilter>("fhac.mapit", 1, 0, "MouseEventFilter", "Use globalMouseEventFilter to blur popups");

    qmlRegisterType<QmlTransform>("fhac.mapit", 1, 0, "TfTransform");
    qmlRegisterUncreatableType<QmlStamp>("fhac.mapit", 1, 0, "MapitStamp", "Can not use MapitTime in Qml because it uses bytes of long unsigned int which are not available in script.");

    qmlRegisterType<QmlRootTreeModel>("fhac.mapit", 1, 0, "RootTreeModel");

    qmlRegisterType<QmlEntitydataRenderer>("fhac.mapit", 1, 0, "EntitydataRenderer");
    qmlRegisterUncreatableType<QPointcloud>("pcl", 1, 0, "Pointcloud", "Please use factory method (not yet available).");
    qmlRegisterType<QPointcloudGeometry>("pcl", 1, 0, "PointcloudGeometry");
    qmlRegisterUncreatableType<QPointfield>("pcl", 1, 0, "Pointfield", "Please use factory method (not yet available).");

    qmlRegisterType<EditorCameraController>("qt3deditorlib", 1, 0, "EditorCameraController");

    qmlRegisterType<QmlPointcloudCoordinatesystem>("fhac.mapit", 1, 0, "PointcloudCoordinatesystem");
    qmlRegisterType<FileIO, 1>("FileIO", 1, 0, "FileIO");

    QQmlApplicationEngine engine;
    QmlRepository *exampleRepo = new QmlRepository(repo, engine.rootContext());
    engine.rootContext()->setContextProperty("globalRepository", exampleRepo);
    engine.rootContext()->setContextProperty("globalRepositoryExplicitlySpecifiedCommandline", specified);

#ifndef NO_GRAPHBLOCKS
    engine.rootContext()->setContextProperty("globalGraphBlocksEnabled", true);
#else
    engine.rootContext()->setContextProperty("globalGraphBlocksEnabled", false);
#endif
    //IconImageProvider *imgProviderDummy = new IconImageProvider("");
    IconImageProvider *imgProviderIcon = new IconImageProvider(":/icon/");
    IconImageProvider *imgProviderMaterialDesign = new IconImageProvider(":/icon/material");
    IconImageProvider *imgProviderOperator = new IconImageProvider(":/qml/operators", false, true);
    IconImageProvider *imgProviderPrimitive = new IconImageProvider(":/icon/", false, false);
    //engine.addImageProvider("", imgProviderDummy);
    engine.addImageProvider("icon", imgProviderIcon);
    engine.addImageProvider("material", imgProviderMaterialDesign);
    engine.addImageProvider("operator", imgProviderOperator);
    engine.addImageProvider("primitive", imgProviderPrimitive);
    MouseEventFilter filter1;
    engine.rootContext()->setContextProperty("globalMouseEventFilter", &filter1);
    engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));

    if(!engine.rootObjects().empty())
    {
        QObject *appStyle = engine.rootObjects().first()->findChild<QObject *>("appStyle");
        if(appStyle)
        {
            ImageUpdater *updater(new ImageUpdater(appStyle, &engine, appStyle));
            const QMetaObject *appStyleMeta = appStyle->metaObject();
            int isDarkIndex = appStyleMeta->indexOfProperty("isDark");
            QMetaMethod isDarkChanged = appStyleMeta->property(isDarkIndex).notifySignal();

            const QMetaObject *imgUpdMeta = &ImageUpdater::staticMetaObject;
            int updateAllIdx = imgUpdMeta->indexOfSlot("updateAllImages()");
            QMetaMethod updateAll = imgUpdMeta->method(updateAllIdx);

            QObject::connect(appStyle, isDarkChanged, updater, updateAll);
            updater->updateAllImages();
        }
        //Qt3DCore::Quick::QQmlAspectEngine * test = engine.findChild<Qt3DCore::Quick::QQmlAspectEngine *>();
        QObject *mainWindow = engine.rootObjects().first();//->findChild<QObject *>("mainWindow");
        Qt3DInput::QInputSettings *inputSettings = engine.rootObjects().first()->findChild<Qt3DInput::QInputSettings *>();
        if (inputSettings) {
            inputSettings->setEventSource(mainWindow);
        } else {
            qDebug() << "No Input Settings found, keyboard and mouse events won't be handled";
        }
        mainWindow->installEventFilter(&filter1);
    }

    int result = app.exec();
#ifndef NO_GRAPHBLOCKS
    shutdownGraphBlocks(graphblockCtx);
#endif
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
