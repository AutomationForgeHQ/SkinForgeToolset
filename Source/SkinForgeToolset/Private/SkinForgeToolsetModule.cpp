#include "Modules/ModuleManager.h"
#include "SkinForgeToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

DEFINE_LOG_CATEGORY_STATIC(LogSkinForgeToolset, Log, All);

class FSkinForgeToolsetModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (!UToolsetRegistry::IsAvailable())
		{
			// Expected outside the editor, and whenever the experimental plugins are off. Not an error.
			UE_LOG(LogSkinForgeToolset, Log, TEXT("Toolset registry unavailable - SkinForge tools not registered."));
			return;
		}
		if (!UToolsetRegistry::IsToolsetClassRegistered(USkinForgeToolset::StaticClass()))
		{
			UToolsetRegistry::RegisterToolsetClass(USkinForgeToolset::StaticClass());
		}
		bRegistered = UToolsetRegistry::IsToolsetClassRegistered(USkinForgeToolset::StaticClass());

		// Worth logging either way. A toolset nobody registers is invisible and says nothing about it.
		UE_LOG(LogSkinForgeToolset, Log, TEXT("SkinForge toolset %s."), bRegistered ? TEXT("registered") : TEXT("failed to register"));
	}

	virtual void ShutdownModule() override
	{
		if (bRegistered && UToolsetRegistry::IsAvailable())
		{
			UToolsetRegistry::UnregisterToolsetClass(USkinForgeToolset::StaticClass());
			bRegistered = false;
		}
	}

private:
	bool bRegistered = false;
};

IMPLEMENT_MODULE(FSkinForgeToolsetModule, SkinForgeToolset)
