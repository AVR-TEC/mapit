#include "versioning/repository.h"
#include "yaml-cpp/yaml.h"
#include "serialization/abstractmapserializer.h"
#include "serialization/leveldb/leveldbserializer.h"
#include "versioning/checkoutimpl.h"
#include "serialization/entitystreammanager.h"
#include <QHash>
#include <QMap>

namespace upns
{
class RepositoryPrivate
{
    RepositoryPrivate():m_serializer(NULL){}
    AbstractMapSerializer* m_serializer;

    void initialize()
    {
        // Check if anything exists in the database
        // Note: There might be commits or objects which are not recognized here.
        // TODO: forbid to delete last branch for this to work. Checkouts might all be deleted.
        size_t numElems = m_serializer->listBranches().size();
        if(numElems) return;
//        numElems = m_serializer->listCheckoutNames().size();
//        if(numElems) return;
        upnsSharedPointer<Branch> master(new Branch());
        master->set_commitid(""); //< InitialCommit
        m_serializer->createBranch(master, "master");
    }
    friend class Repository;
};

Repository::Repository(const YAML::Node &config)
    :m_p(new RepositoryPrivate)
{
    if(const YAML::Node mapsource = config["mapsource"])
    {
        if(const YAML::Node mapsourceName = mapsource["name"])
        {
            std::string mapsrcnam = mapsourceName.as<std::string>();
            std::transform(mapsrcnam.begin(), mapsrcnam.end(), mapsrcnam.begin(), ::tolower);
            AbstractMapSerializer *mser = NULL;
            if(mapsrcnam == "mapfileservice")
            {
                m_p->m_serializer = new LevelDBSerializer(mapsource);
            } else {
                log_error("mapsource '" + mapsrcnam + "' was not found.");
            }
        } else {
            log_error("'mapsource' has no 'name' in config");
        }
    } else {
        log_error("Key 'mapsource' not given in config");
    }
    assert(m_p->m_serializer);
    m_p->initialize();
}

Repository::~Repository()
{
    delete m_p;
}

upnsSharedPointer<Checkout> Repository::createCheckout(const CommitId &commitIdOrBranchname, const upnsString &name)
{
    upnsSharedPointer<CheckoutObj> co(m_p->m_serializer->getCheckoutCommit(name));
    if(co != NULL)
    {
        log_info("Checkout with this name already exist: " + name);
        return NULL;
    }
    upnsSharedPointer<Branch> branch(m_p->m_serializer->getBranch(commitIdOrBranchname));
    CommitId commitId;
    upnsString branchName;
    if(branch != NULL)
    {
        // assert: empty, if this is the inial commit and "master"
        assert( branch->commitid().empty() || m_p->m_serializer->getCommit(branch->commitid()) != NULL );
        if(branch->commitid().empty())
        {
            log_info("empty repository. checking out initial master");
        }
        commitId = branch->commitid();
        branchName = commitIdOrBranchname;
    }
    else
    {
        upnsSharedPointer<Commit> commit(m_p->m_serializer->getCommit(commitIdOrBranchname));
        if(commit != NULL)
        {
            commitId = commitIdOrBranchname;
            branchName = "";
        }
        else
        {
            log_info("given commitIdOrBranchname was not a commitId or branchname.");
            return NULL;
        }
    }
    co = upnsSharedPointer<CheckoutObj>(new CheckoutObj());
    co->mutable_rollingcommit()->add_parentcommitids(commitId);
    StatusCode s = m_p->m_serializer->createCheckoutCommit( co, name );
    if(!upnsIsOk(s))
    {
        log_error("Could not create checkout.");
    }
    return upnsSharedPointer<Checkout>(new CheckoutImpl(m_p->m_serializer, co, name, branchName));
}

upnsVec<upnsString> Repository::listCheckoutNames()
{
    return m_p->m_serializer->listCheckoutNames();
}

upnsSharedPointer<Tree> Repository::getTree(const ObjectId &oid)
{
    return m_p->m_serializer->getTree(oid);
}

upnsSharedPointer<Entity> Repository::getEntity(const ObjectId &oid)
{
    return m_p->m_serializer->getEntity(oid);
}

upnsSharedPointer<Commit> Repository::getCommit(const ObjectId &oid)
{
    return m_p->m_serializer->getCommit(oid);
}

upnsSharedPointer<CheckoutObj> Repository::getCheckoutObj(const upnsString &name)
{
    return m_p->m_serializer->getCheckoutCommit(name);
}

upnsSharedPointer<Branch> Repository::getBranch(const upnsString &name)
{
    return m_p->m_serializer->getBranch(name);
}

MessageType Repository::typeOfObject(const ObjectId &oid)
{
    return m_p->m_serializer->typeOfObject(oid);
}

upnsSharedPointer<AbstractEntityData> Repository::getEntityDataReadOnly(const ObjectId &oid)
{
    // For entitydata it is not enough to call serializer directly.
    // Moreover special classes need to be created by layertype plugins.
    return EntityStreamManager::getEntityDataImpl(m_p->m_serializer, oid, true, false);
}

upnsSharedPointer<Checkout> Repository::getCheckout(const upnsString &checkoutName)
{
    upnsSharedPointer<CheckoutObj> co(m_p->m_serializer->getCheckoutCommit(checkoutName));
    if(co == NULL)
    {
        log_info("Checkout does not exist: " + checkoutName);
        return NULL;
    }
    return upnsSharedPointer<Checkout>(new CheckoutImpl(m_p->m_serializer, co, checkoutName));
}

StatusCode Repository::deleteCheckoutForced(const upnsString &checkoutName)
{
    upnsSharedPointer<CheckoutObj> co(m_p->m_serializer->getCheckoutCommit(checkoutName));
    if(co == NULL)
    {
        log_info("Checkout with this name does not exist: " + checkoutName);
        return UPNS_STATUS_ENTITY_NOT_FOUND;
    }
    //TODO: Get Checkout, remove its inner Commit and objects (objects only if not referenced)!
    return m_p->m_serializer->removeCheckoutCommit(checkoutName);
}

CommitId Repository::commit(const upnsSharedPointer<Checkout> checkout, const upnsString msg)
{
    CheckoutImpl *co = static_cast<CheckoutImpl*>(checkout.get());
    QMap< ::std::string, ::std::string> oldToNewIds;
    CommitId ret;
    StatusCode s = co->depthFirstSearch([&](upnsSharedPointer<Commit> obj){return true;}, [&](upnsSharedPointer<Commit> obj)
    {
        ::std::string rootId(obj->root());
        assert(oldToNewIds.contains(rootId));
        obj->set_root(oldToNewIds.value(rootId));
        //TODO: Lots of todos here (Metadata)
        //ci->add_parentcommitids(co->m_checkout->mutable_);
        m_p->m_serializer->createCommit(obj);
        ret = obj->commitid();
        return true;
    },
    [&](upnsSharedPointer<Tree> obj){return true;}, [&](upnsSharedPointer<Tree> obj)
    {
        assert(obj != NULL);
        ::google::protobuf::Map< ::std::string, ::upns::ObjectReference > &refs = *obj->mutable_refs();
        ::google::protobuf::Map< ::std::string, ::upns::ObjectReference >::iterator iter(refs.begin());
        while(iter != refs.end())
        {
            ::std::string id(iter->second.id());
            assert(oldToNewIds.contains(id));
            iter->second.set_id(oldToNewIds.value(id));
            iter++;
        }
        ::std::string prevId(obj->id());
        //TODO: may fail if object already exists. If object with same hash already exits, skip.
        m_p->m_serializer->createTree(obj);
        oldToNewIds.insert(prevId, obj->id());
        return true;
    },
    [&](upnsSharedPointer<Entity> obj){return true;}, [&](upnsSharedPointer<Entity> obj)
    {
        ::std::string prevId(obj->id());
        m_p->m_serializer->createEntity(obj);
        oldToNewIds.insert(prevId, obj->id());
        return true;
    });
    if(!upnsIsOk(s))
    {
        log_error("error while commiting");
    }
    return ret;
}

upnsVec<upnsSharedPointer<Branch> > Repository::getBranches()
{
    return upnsVec<upnsSharedPointer<Branch> >();
}

StatusCode Repository::push(Repository &repo)
{
    return UPNS_STATUS_OK;
}

StatusCode Repository::pull(Repository &repo)
{
    return UPNS_STATUS_OK;
}

CommitId Repository::parseCommitRef(const upnsString &commitRef)
{
    return "";
}

upnsSharedPointer<Checkout> Repository::merge(const CommitId mine, const CommitId theirs, const CommitId base)
{
    return NULL;
}

upnsVec<upnsPair<CommitId, ObjectId> > Repository::ancestors(const CommitId &commitId, const ObjectId &objectId, const int level)
{
    return upnsVec<upnsPair<CommitId, ObjectId> >();
}

StatusCode Repository::init()
{
    upnsVec< upnsSharedPointer<Branch> > branches(m_p->m_serializer->listBranches());
    if(!branches.empty())
    {
        return UPNS_STATUS_REPOSITORY_NOT_EMPTY;
    }
    upnsVec< upnsString > cos(m_p->m_serializer->listCheckoutNames());
    if(!cos.empty())
    {
        return UPNS_STATUS_REPOSITORY_NOT_EMPTY;
    }
//    upnsSharedPointer<Commit> initialCommit(new Commit);
//    initialCommit->set_commitid(""); //TODO: make sure this is traditionally really 0x0
//    m_p->m_serializer->createBranch(masterBranch, "master");
    upnsSharedPointer<Branch> masterBranch(new Branch);
    masterBranch->set_commitid(""); //TODO: make sure this is traditionally really 0x0
    m_p->m_serializer->createBranch(masterBranch, "master");
}

bool Repository::canRead()
{
    return true;
}

bool Repository::canWrite()
{
    return true;
}

}
