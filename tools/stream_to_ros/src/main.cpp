#include <mapit/msgs/services.pb.h>
#include <upns/versioning/repository.h>
#include <upns/versioning/repositoryfactorystandard.h>
#include <upns/logging.h>
#include <boost/program_options.hpp>

#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>

#include "publishclock.h"
#include "publishpointclouds.h"

namespace po = boost::program_options;

enum PublishType { pointcloud };

int main(int argc, char *argv[])
{
    upns_init_logging();

    // get parameter
    po::options_description program_options_desc(std::string("Usage: ") + argv[0] + " <checkout name> <destination>");
    program_options_desc.add_options()
        ("help,h", "print usage")
        ("checkout,c", po::value<std::string>()->required(), "checkout to work with")
        ("use_sim_time,s", po::value<bool>()->default_value(false), "whenever the clock should be published or not")
        ("map,m", po::value<std::string>()->required(), "")
        ("layer,l", po::value<std::string>()->required(), "")
        ("entity,e", po::value<std::string>()->default_value(""), "")
        ("type,t", po::value<std::string>()->required(), "the type of what to be published,\nsupported options are:\npointcloud\n...");
    po::positional_options_description pos_options;
    pos_options.add("checkout",  1);

    upns::RepositoryFactoryStandard::addProgramOptions(program_options_desc);
    po::variables_map vars;
    po::store(po::command_line_parser(argc, argv).options(program_options_desc).positional(pos_options).run(), vars);
    if(vars.count("help"))
    {
        std::cout << program_options_desc << std::endl;
        return 1;
    }
    po::notify(vars);

    std::unique_ptr<upns::Repository> repo( upns::RepositoryFactoryStandard::openRepository( vars ) );

    // evaluate parameter
    PublishType publish_type;
    if ( 0 == vars["type"].as<std::string>().compare("pointcloud") ) {
      publish_type = PublishType::pointcloud;
    } else {
      log_error("unknown type to publish is given");
      return 1;
    }

    bool single_entity = true;
    if ( 0 == vars["entity"].as<std::string>().compare("") ) {
      single_entity = false;
    }

    // get mapit data
    std::shared_ptr<upns::Checkout> co = repo->getCheckout( vars["checkout"].as<std::string>() );
    if(co == nullptr)
    {
        log_error("Checkout \"" + vars["checkout"].as<std::string>() + "\" not found");
        std::vector<std::string> possibleCheckouts = repo->listCheckoutNames();
        if (possibleCheckouts.size() == 0) {
            log_info("No possible checkout");
        }
        for ( std::string checkout : possibleCheckouts ) {
            log_info("Possible checkout: " + checkout);
        }
        return 1;
    }

    std::shared_ptr<mapit::Map> map = co->getMap(vars["map"].as<std::string>());
    if (map == nullptr) {
      // TODO error
      return 1;
    }
    std::shared_ptr<mapit::Layer> layer = co->getLayer(map, vars["layer"].as<std::string>());
    if (layer == nullptr) {
      // TODO error
      return 1;
    }
    std::shared_ptr<mapit::Entity> entity;
    if ( single_entity ) {
      entity = co->getEntity(layer, vars["entity"].as<std::string>());
      if (entity == nullptr) {
        // TODO error
        return 1;
      }
    }

    // connect to ROS
    ros::init (argc, argv, "mapit_stream_to_ros__" + vars["map"].as<std::string>() + "_" + vars["layer"].as<std::string>() + "_" + vars["entity"].as<std::string>());

    std::shared_ptr<ros::NodeHandle> node_handle = std::shared_ptr<ros::NodeHandle>( new ros::NodeHandle("~") );

    // create clock when parameter is given
    // TODO where to start/end (map time, layer time????)
    std::shared_ptr<PublishClock> publish_clock = nullptr;
    if ( vars["use_sim_time"].as<bool>() ) {
      // set simulated time
      node_handle->setParam("/use_sim_time", true);

      std::unique_ptr<ros::Publisher> pub = std::make_unique<ros::Publisher>(node_handle->advertise<rosgraph_msgs::Clock>("/clock", 1));
      publish_clock = std::make_shared<PublishClock>( std::move(pub) );
    }

    // create pubblisher
    std::string pub_name = "/mapit/" + vars["map"].as<std::string>() + "/" + vars["layer"].as<std::string>();
    if ( single_entity ) {
      pub_name += "/" + vars["entity"].as<std::string>();
    }

    std::shared_ptr<PublishToROS> publish_manager;
    switch (publish_type) {
      case PublishType::pointcloud:
        std::unique_ptr<ros::Publisher> pub = std::make_unique<ros::Publisher>(node_handle->advertise<sensor_msgs::PointCloud2>(pub_name, 10, true));
        if ( single_entity ) {
          publish_manager = std::make_shared<PublishPointClouds>(
                              co, node_handle, std::move(pub)
                              );
          publish_manager->publish_entity(entity);
        } else {
          publish_manager = std::make_shared<PublishPointClouds>(
                              co, node_handle, std::move(pub), layer
                              );
        }
        break;
//      default:
//        log_error("Type is not implemented compleatly???");
    }

    ros::Rate r( 100 ); // publish the clock with 100 Hz
    while (node_handle->ok()) {

      ros::spinOnce();
      r.sleep();
    }

    return 0;
}
