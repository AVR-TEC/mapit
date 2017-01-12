#include <iostream>
#include "upns.h"
#include "services.pb.h"
#include "versioning/repository.h"
#include "versioning/repositoryfactorystandard.h"
#include "upns_errorcodes.h"
#include <log4cplus/configurator.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    log4cplus::BasicConfigurator logconfig;
    logconfig.configure();

    po::options_description program_options_desc(std::string("Usage: ") + argv[0] + " <checkout name> <commitmessage>");
    program_options_desc.add_options()
            ("help,h", "print usage")
            ("checkout,co", po::value<std::string>()->required(), "")
            ("commitmessage,m", po::value<std::string>()->required(), "");
    po::positional_options_description pos_options;
    pos_options.add("checkout",  1)
               .add("commitmessage",  1);

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

    upns::upnsSharedPointer<upns::Checkout> co = repo->getCheckout( vars["checkout"].as<std::string>() );

    if(co)
    {
        const upns::CommitId ciid = repo->commit(co, vars["commitmessage"].as<std::string>() );
        std::cout << "commit " << ciid << std::endl;
    }
    else
    {
        std::cout << "failed to commit checkout " << vars["commitmessage"].as<std::string>() << std::endl;
    }
    return co == NULL;
}
