#include "checkoutimpl.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <string>
#include <sstream>
#include <algorithm>
#include <log4cplus/logger.h>
#include "module.h"
#include "operationenvironmentimpl.h"
#include "serialization/entitystreammanager.h"
#include <QDir>
#include <QDateTime>

namespace upns
{
typedef ModuleInfo* (*GetModuleInfo)();

CheckoutImpl::CheckoutImpl(AbstractMapSerializer *serializer, upnsSharedPointer<CheckoutObj>  checkoutCommit, upnsString name, const upnsString branchname)
    :m_serializer(serializer),
     m_branchname( branchname ),
     m_name(name),
     m_checkout(checkoutCommit),
     m_nextTransientOid("0")
{

//    if(m_serializer->isCheckout(checkoutCommit))
//    {
//        m_checkoutId = commitOrCheckoutId;
//    }
//    else if(m_serializer->isCommit(commitOrCheckoutId))
//    {
//        upnsSharedPointer<Commit> co(new Commit());
//        co->add_parentcommitids(commitOrCheckoutId);
//        m_checkoutId = m_serializer->createCheckoutCommit( co );
//    }
}

//CheckoutImpl::CheckoutImpl(AbstractMapSerializer *serializer, const upnsSharedPointer<Branch> &branch)
//    :m_serializer(serializer),
//      m_branch( branch )
//{
////    if(m_serializer->isCheckout(branch->commitid()))
////    {
////        m_checkoutId = branch->commitid();
////    }
////    else
////    {
////        upnsSharedPointer<Commit> co(new Commit());
////        if(m_serializer->isCommit(branch->commitid()))
////        {
////            co->add_parentcommitids(branch->commitid());
////        }
////        else
////        {
////            log_info("Initial Commit created, branch: " + branch->name());
////        }
////        m_checkoutId = m_serializer->createCheckoutCommit( co );
////    }
//}

CheckoutImpl::~CheckoutImpl()
{
}

bool CheckoutImpl::isInConflictMode()
{
    return false;
}

upnsVec<upnsSharedPointer<Conflict> > CheckoutImpl::getPendingConflicts()
{
    return upnsVec<upnsSharedPointer<Conflict> >();
}

upnsSharedPointer<Tree> CheckoutImpl::getRoot()
{
    return m_serializer->getTree(m_checkout->rollingcommit().root());
}

upnsSharedPointer<Tree> CheckoutImpl::getTree(const Path &path)
{
    ObjectId oid = oidForPath(path);
    return m_serializer->getTree(oid);
}

upnsSharedPointer<Entity> CheckoutImpl::getEntity(const Path &path)
{
    ObjectId oid = oidForPath(path);
    return m_serializer->getEntity(oid);
}

upnsSharedPointer<Tree> CheckoutImpl::getTreeConflict(const ObjectId &objectId)
{
    return NULL;
}

upnsSharedPointer<Entity> CheckoutImpl::getEntityConflict(const ObjectId &objectId)
{
    return NULL;
}

OperationResult CheckoutImpl::doOperation(const OperationDescription &desc)
{
    //TODO: This code my belong to a class which handles operation-modules. A "listOperations" might be needed outside of "checkout".
    OperationEnvironmentImpl env(desc);
    env.setCheckout( this );
#ifndef NDEBUG
    upnsString debug = DEBUG_POSTFIX;
#else
    upnsString debug = "";
#endif

#ifdef _WIN32
    upnsString prefix = "";
    upnsString postfix = ".dll";
#else
    upnsString prefix = "lib";
    upnsString postfix = ".so";
#endif
    std::stringstream filename;
    filename << "./libs/operator_modules_collection/" << desc.operatorname() << "/" << prefix << desc.operatorname() << debug << postfix;
    if(desc.operatorversion())
    {
        filename << "." << desc.operatorversion();
    }
#ifdef _WIN32
    HMODULE handle = LoadLibrary(filename.str().c_str());
#else
    void* handle = dlopen(filename.str().c_str(), RTLD_NOW);
#endif
    if (!handle) {
#ifdef _WIN32
#else
        std::cerr << "Cannot open library: " << dlerror() << '\n';
#endif
        return OperationResult(UPNS_STATUS_ERR_MODULE_OPERATOR_NOT_FOUND, OperationDescription());
    }
#ifdef _WIN32
    //FARPROC getModInfo = GetProcAddress(handle,"getModuleInfo");
    GetModuleInfo getModInfo = (GetModuleInfo)GetProcAddress(handle,"getModuleInfo");
#else
    GetModuleInfo getModInfo = (GetModuleInfo)dlsym(handle, "getModuleInfo");
#endif
    ModuleInfo* info = getModInfo();
    StatusCode result = info->operate( &env );
    if(!upnsIsOk(result))
    {
        std::stringstream strm;
        strm << "operator '" << desc.operatorname() << "' reported an error. (code:" << result << ")";
        log_error(strm.str());
    }
    return OperationResult(result, env.outputDescription());
}

upnsSharedPointer<AbstractEntityData> CheckoutImpl::getEntityDataReadOnly(const Path &path)
{
    return EntityStreamManager::getEntityDataImpl(m_serializer, oidForPath(path), true, false);
}

upnsSharedPointer<AbstractEntityData> CheckoutImpl::getEntityDataReadOnlyConflict(const ObjectId &entityId)
{
    return EntityStreamManager::getEntityDataImpl(m_serializer, entityId, false, true);
}

upnsSharedPointer<AbstractEntityData> CheckoutImpl::getEntityDataForReadWrite(const Path &path)
{
    return EntityStreamManager::getEntityDataImpl(m_serializer, oidForPath(path), true, true);
}

StatusCode CheckoutImpl::storeTree(const Path &path, upnsSharedPointer<Tree> tree)
{

    //return m_serializer->storeTree();
    return createPath(path, tree);
}

StatusCode CheckoutImpl::storeEntity(const Path &path, upnsSharedPointer<Entity> entity)
{
    return createPath(path, entity);
}

void CheckoutImpl::setConflictSolved(const Path &path, const ObjectId &oid)
{

}

StatusCode CheckoutImpl::depthFirstSearch(std::function<bool (upnsSharedPointer<Commit>)> beforeCommit, std::function<bool (upnsSharedPointer<Commit>)> afterCommit,
                                          std::function<bool (upnsSharedPointer<Tree>)> beforeTree, std::function<bool (upnsSharedPointer<Tree>)> afterTree,
                                          std::function<bool (upnsSharedPointer<Entity>)> beforeEntity, std::function<bool (upnsSharedPointer<Entity>)> afterEntity)
{
    upnsSharedPointer<Commit> rootCommit(new Commit(m_checkout->rollingcommit()));
    return depthFirstSearch(rootCommit, beforeCommit, afterCommit, beforeTree, afterTree, beforeEntity, afterEntity);
}

//upnsSharedPointer<CheckoutObj> CheckoutImpl::getCheckoutObj()
//{
//    return m_checkout;
//}

ObjectId CheckoutImpl::oidForChild(upnsSharedPointer<Tree> tree, const ::std::string &name)
{
    assert(tree != NULL);
    const ::google::protobuf::Map< ::std::string, ::upns::ObjectReference > &refs = tree->refs();
    ::google::protobuf::Map< ::std::string, ::upns::ObjectReference >::const_iterator iter(refs.cbegin());
    while(iter != refs.cend())
    {
        if(iter->first == name)
        {
            return iter->second.id();
        }
        iter++;
    }
    return "";
}

ObjectId CheckoutImpl::oidForPath(const Path &path)
{
    Path p = preparePath(path);
    ObjectId oid(m_checkout->rollingcommit().root());
    upnsSharedPointer<Tree> current = m_serializer->getTree(oid);
    upnsSharedPointer<Entity> currentEntity;
    forEachPathSegment(p,
    [&](upnsString seg, size_t idx, bool isLast)
    {
        if(current == NULL) return false; // can not go futher
        if(seg.empty()) return false; // "//" not allowed
        oid = oidForChild(current, seg);
        if(oid.empty()) return false; // path invalid
        if(isLast) return false; // we are done
        current = m_serializer->getTree(oid);
        return true; // continue thru path
    },
    [](upnsString seg, size_t idx, bool isLast)
    {
        assert(false);
        return false;
    });
    return oid;
}

Path CheckoutImpl::preparePath(const Path &path)
{
    // path p has no beginning / and always trailing /
    Path p = path;
    while(p[0] == '/')
    {
        p = p.substr(1);
    }
    if(p.length() != 0 && p[p.length()-1] != '/')
    {
        p += "/";
    }
    return p;
}

bool CheckoutImpl::forEachPathSegment(const Path &path,
                                      std::function<bool (upnsString, size_t, bool)> before, std::function<bool (upnsString, size_t, bool)> after, const int start)
{
    size_t nextSlash = path.find_first_of('/', start);
    upnsString segment(path.substr(start, nextSlash-start));
    bool isLast = nextSlash == std::string::npos || nextSlash == path.length()-1;
    if(!before(segment, nextSlash+1, isLast))
    {
        return false;
    }
    if(!isLast)
    {
        if(!forEachPathSegment(path, before, after, nextSlash+1))
        {
            return false;
        }
    }
    if(!after(segment, nextSlash+1, isLast))
    {
        return false;
    }
    return true;
}

upnsString CheckoutImpl::transientOid(const upnsString &path)
{
    //TODO: escape "_" in name
    return "path_"+m_name+"_"+path;
}

// TODO: Object-classes are seperated (entity, tree, ...) and are here unified again.
// On the lowest level they are now unified. TODO: provide unified interface to them (template).
template <>
StatusCode CheckoutImpl::createObject<Tree>(upnsSharedPointer<Tree> leafObject)
{
    return m_serializer->createTree(leafObject, true);
}

template <>
StatusCode CheckoutImpl::createObject<Entity>(upnsSharedPointer<Entity> leafObject)
{
    return m_serializer->createEntity(leafObject, true);
}

template <>
StatusCode CheckoutImpl::storeObject<Tree>(upnsSharedPointer<Tree> leafObject)
{
    return m_serializer->storeTree(leafObject, true);
}

template <>
StatusCode CheckoutImpl::storeObject<Entity>(upnsSharedPointer<Entity> leafObject)
{
    return m_serializer->storeEntity(leafObject, true);
}

StatusCode CheckoutImpl::depthFirstSearch(upnsSharedPointer<Entity> obj,
                                                  std::function<bool(upnsSharedPointer<Commit>)> beforeCommit, std::function<bool(upnsSharedPointer<Commit>)> afterCommit,
                                                  std::function<bool(upnsSharedPointer<Tree>)> beforeTree, std::function<bool(upnsSharedPointer<Tree>)> afterTree,
                                                  std::function<bool(upnsSharedPointer<Entity>)> beforeEntity, std::function<bool(upnsSharedPointer<Entity>)> afterEntity)
{
    assert(obj != NULL);
    if(!beforeEntity(obj)) return UPNS_STATUS_OK;
    if(!afterEntity(obj)) return UPNS_STATUS_OK;
    return UPNS_STATUS_OK;
}

StatusCode CheckoutImpl::depthFirstSearch(upnsSharedPointer<Tree> obj,
                                                std::function<bool(upnsSharedPointer<Commit>)> beforeCommit, std::function<bool(upnsSharedPointer<Commit>)> afterCommit,
                                                std::function<bool(upnsSharedPointer<Tree>)> beforeTree, std::function<bool(upnsSharedPointer<Tree>)> afterTree,
                                                std::function<bool(upnsSharedPointer<Entity>)> beforeEntity, std::function<bool(upnsSharedPointer<Entity>)> afterEntity)
{
    assert(obj != NULL);
    if(!beforeTree(obj)) return UPNS_STATUS_OK;
    ::google::protobuf::Map< ::std::string, ::upns::ObjectReference > &refs = *obj->mutable_refs();
    ::google::protobuf::Map< ::std::string, ::upns::ObjectReference >::iterator iter(refs.begin());
    while(iter != refs.cend())
    {
        const ObjectId &oid = iter->second.id();
        MessageType t = m_serializer->typeOfObject(oid);
        if(t == MessageType::MessageCommit)
        {
            upnsSharedPointer<Commit> commit(m_serializer->getCommit(oid));
            StatusCode s = depthFirstSearch(commit, beforeCommit, afterCommit, beforeTree, afterTree, beforeEntity, afterEntity);
            if(!upnsIsOk(s)) return s;
        }
        else if(t == MessageType::MessageTree)
        {
            upnsSharedPointer<Tree> tree(m_serializer->getTree(oid));
            StatusCode s = depthFirstSearch(tree, beforeCommit, afterCommit, beforeTree, afterTree, beforeEntity, afterEntity);
            if(!upnsIsOk(s)) return s;
        }
        else if(t == MessageType::MessageEntity)
        {
            upnsSharedPointer<Entity> entity(m_serializer->getEntity(oid));
            StatusCode s = depthFirstSearch(entity, beforeCommit, afterCommit, beforeTree, afterTree, beforeEntity, afterEntity);
            if(!upnsIsOk(s)) return s;
        }
        else
        {
            log_error("Unsupported type during depth search " + iter->first);
        }
        iter++;
    }
    if(!afterTree(obj)) return UPNS_STATUS_OK;
    return UPNS_STATUS_OK;
}

StatusCode CheckoutImpl::depthFirstSearch(upnsSharedPointer<Commit> obj,
                                                  std::function<bool(upnsSharedPointer<Commit>)> beforeCommit, std::function<bool(upnsSharedPointer<Commit>)> afterCommit,
                                                  std::function<bool(upnsSharedPointer<Tree>)> beforeTree, std::function<bool(upnsSharedPointer<Tree>)> afterTree,
                                                  std::function<bool(upnsSharedPointer<Entity>)> beforeEntity, std::function<bool(upnsSharedPointer<Entity>)> afterEntity)
{
    assert(obj != NULL);
    if(!beforeCommit(obj)) return UPNS_STATUS_OK;
    upnsSharedPointer<Tree> tree(m_serializer->getTree(obj->root()));
    StatusCode s = depthFirstSearch(tree, beforeCommit, afterCommit, beforeTree, afterTree, beforeEntity, afterEntity);
    if(!upnsIsOk(s)) return s;
    if(!afterCommit(obj)) return UPNS_STATUS_OK;
    return UPNS_STATUS_OK;
}

}
