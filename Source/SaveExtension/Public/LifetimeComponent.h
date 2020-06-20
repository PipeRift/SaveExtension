// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SaveExtensionInterface.h"
#include "SaveManager.h"

#include "LifetimeComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeStartSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeSavedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLifetimeResumeSignature);

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


	// Event called when Save process starts
	virtual void OnSaveBegan(const FSaveFilter& Filter) override;

	// Event called when Load process ends
	virtual void OnLoadFinished(const FSaveFilter& Filter, bool bError);


	FORCEINLINE USaveManager* GetManager() const
	{
		return USaveManager::GetSaveManager(GetWorld());
	}


	/***********************************************************************/
	/* EVENTS															   */
	/***********************************************************************/
protected:

	// Called once when this actor gets created for the first time. Similar to BeginPlay
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeStartSignature Start;

	// Called when game is saved
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeSavedSignature Saved;

	// Called when game loaded
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeResumeSignature Resume;

	// Called when this actor gets destroyed for ever
	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FLifetimeFinishSignature Finish;


public:

	FLifetimeStartSignature& OnStart() { return Start; }
	FLifetimeSavedSignature& OnSaved() { return Saved; }
	FLifetimeResumeSignature& OnResume() { return Resume; }
	FLifetimeFinishSignature& OnFinish() { return Finish; }
};
