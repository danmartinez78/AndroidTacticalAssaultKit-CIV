
#include "core/ProjectionFactory2.h"

#include <map>

#include "core/LegacyAdapters.h"
#include "core/ProjectionFactory3.h"
#include "thread/Lock.h"
#include "util/Memory.h"

using namespace TAK::Engine::Thread;
using namespace TAK::Engine::Core;
using namespace TAK::Engine::Util;

namespace
{
    bool sdkPreferred = true;

    Mutex &mutex() NOTHROWS;
    std::map<std::shared_ptr<ProjectionSpi2>, ProjectionSpi3 *> &adaptedSpis() NOTHROWS;

}


/******************************************************************************/
// ProjectionSpi2 definition

ProjectionSpi2::~ProjectionSpi2() { }

ProjectionPtr2 ProjectionSpi2::nullProjectionPtr() {
    return ProjectionPtr2(NULL, NULL);
}

/******************************************************************************/
// Projection Factory definition

ProjectionPtr2 TAK::Engine::Core::ProjectionFactory2_getProjection(int srid)
{
    // obtain the projection
    Projection2Ptr value(NULL, NULL);
    if (ProjectionFactory3_create(value, srid) != TE_Ok)
        return ProjectionSpi2::nullProjectionPtr();

    // adapt the projection back to legacy
    ProjectionPtr2 lvalue(NULL, NULL);
    LegacyAdapters_adapt(lvalue, std::move(value));

    return lvalue;
}

void TAK::Engine::Core::ProjectionFactory2_registerSpi(std::shared_ptr<ProjectionSpi2> *lspi)
{
    LockPtr lock(NULL, NULL);
    Lock_create(lock, mutex());

    std::map<std::shared_ptr<ProjectionSpi2>, ProjectionSpi3 *> &spis = adaptedSpis();
    std::map<std::shared_ptr<ProjectionSpi2>, ProjectionSpi3 *>::iterator entry;
    entry = spis.find(*lspi);
    if (entry != spis.end())
        return;

    ProjectionSpi3Ptr spi(NULL, NULL);
    if (LegacyAdapters_adapt(spi, std::unique_ptr<ProjectionSpi2, void(*)(const ProjectionSpi2 *)>(lspi->get(), Memory_leaker_const<ProjectionSpi2>)) != TE_Ok)
        return;

    spis[*lspi] = spi.get();
    lock.reset();

    ProjectionFactory3_registerSpi(std::move(spi), 0);
}

void TAK::Engine::Core::ProjectionFactory2_unregisterSpi(std::shared_ptr<ProjectionSpi2> *lspi)
{
    LockPtr lock(NULL, NULL);
    Lock_create(lock, mutex());

    std::map<std::shared_ptr<ProjectionSpi2>, ProjectionSpi3 *> &spis = adaptedSpis();
    std::map<std::shared_ptr<ProjectionSpi2>, ProjectionSpi3 *>::iterator entry;
    entry = spis.find(*lspi);
    if (entry == spis.end())
        return;

    ProjectionSpi3 &spi = *entry->second;
    spis.erase(entry);
    lock.reset();

    ProjectionFactory3_unregisterSpi(spi);
}

void TAK::Engine::Core::ProjectionFactory2_setPreferSdkProjections(bool sdk)
{
    ProjectionFactory3_setPreferSdkProjections(sdk);
}

namespace
{
    Mutex &mutex() NOTHROWS
    {
        static Mutex m;
        return m;
    }

    std::map<std::shared_ptr<ProjectionSpi2>, ProjectionSpi3 *> &adaptedSpis() NOTHROWS
    {
        static std::map<std::shared_ptr<ProjectionSpi2>, ProjectionSpi3 *> s;
        return s;
    }
} // end unnamed namespace
