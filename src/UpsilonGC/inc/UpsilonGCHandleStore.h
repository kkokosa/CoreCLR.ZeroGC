#pragma once
#ifdef __linux__
#include "pal.h"
#endif
#include "debugmacros.h"
#include "gcenv.base.h"
#include "gcinterface.h"

class UpsilonGCHandleStore : public IGCHandleStore
{
public:
    // Inherited via IGCHandleStore
    virtual void Uproot() override;
    virtual bool ContainsHandle(OBJECTHANDLE handle) override;
    virtual OBJECTHANDLE CreateHandleOfType(Object * object, HandleType type) override;
    virtual OBJECTHANDLE CreateHandleOfType(Object * object, HandleType type, int heapToAffinitizeTo) override;
    virtual OBJECTHANDLE CreateHandleWithExtraInfo(Object * object, HandleType type, void * pExtraInfo) override;
    virtual OBJECTHANDLE CreateDependentHandle(Object * primary, Object * secondary) override;
    virtual void RelocateAsyncPinnedHandles(IGCHandleStore* pTarget, void(*clearIfComplete)(Object*), void(*setHandle)(Object*, OBJECTHANDLE));
    virtual bool EnumerateAsyncPinnedHandles(async_pin_enum_fn callback, void * context);
    
	void ScanHandles(promote_func* pf, ScanContext* sc);
    void DestroyHandleOfType(OBJECTHANDLE handle, HandleType type);
    void DestroyHandleOfUnknownType(OBJECTHANDLE handle);
};
