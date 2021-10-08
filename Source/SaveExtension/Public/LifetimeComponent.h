// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SaveExtensionInterface.h"
#include "SaveManager.h"

#include "LifetimeComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeStartSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeSaveSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeSavedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeResumeSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeResumedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeFinishSignature);



/**
 * Controls the complete saving and loading process
 */
UCLASS(BlueprintType, ClassGroup = SaveExtension, meta = (BlueprintSpawnableComponent))
class SAVEEXTENSION_API ULifetimeComponent : public UActorComponent, public ISaveExtensionInterface
{
	GENERATED_BODY()

	/************************************************************************/
	/* METHODS											     			    */
	/************************************************************************/
public:

	ULifetimeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;


	// Event called when Save process
	virtual void OnSaveBegan(const FSELevelFilter& Filter) override;
	virtual void OnSaveFinished(const FSELevelFilter& Filter, bool bError) override;

	// Event called when Load process
	virtual void OnLoadBegan(const FSELevelFilter& Filter) override;
	virtual void OnLoadFinished(const FSELevelFilter& Filter, bool bError) override;


	USaveManager* GetManager() const
	{
		return USaveManager::Get(GetWorld());
	}


	/***********************************************************************/
	/* EVENTS															   */
	/***********************************************************************/
protected:

	// Called once when this actor gets created for the first time. Similar to BeginPlay
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeStartSignature OnStart;

	// Called when begin save
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeSaveSignature OnSave;

	// Called when after save
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeSavedSignature OnSaved;

	// Called when before load
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeResumeSignature OnResume;

	// Called when after load
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeResumedSignature OnResumed;

	// Called when this actor gets destroyed for ever
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeFinishSignature OnFinish;


public:

	// FLifetimeStartSignature& OnStart() { return Start; }
	// FLifetimeSaveSignature& OnSave() { return Save; }
	// FLifetimeSavedSignature& OnSaved() { return Saved; }
	// FLifetimeResumeSignature& OnResume() { return Resume; }
	// FLifetimeResumedSignature& OnResumed() { return Resumed; }
	// FLifetimeFinishSignature& OnFinish() { return Finish; }
};
