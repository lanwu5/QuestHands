// Copyright(c) 2020 Sheffer Online Services

#include "QuestHandsComponent.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/CapsuleComponent.h"

//---------------------------------------------------------------------------------------------------------------------
/**
*/
UQuestHandsComponent::UQuestHandsComponent() :
      CreateHandMeshComponents(true)
    , UpdateHandMeshComponents(true)
    , LeftHandMesh(nullptr)
    , RightHandMesh(nullptr)
    , UpdateHandScale(true)
    , UpdatePhysicsCapsules(true)
    , LeftHandBoneRotationOffset(0.0f, 90.0f, 90.0f)
    , RightHandBoneRotationOffset(0.0f, 90.0f, 90.0f)
    , leftPoseable(nullptr)
    , rightPoseable(nullptr)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    FString defaultLeftHandPath = TEXT("/QuestHands/Meshes/hand_left");
    LeftHandMesh = ConstructorHelpersInternal::FindOrLoadObject<USkeletalMesh>(defaultLeftHandPath);

    FString defaultRightHandPath = TEXT("/QuestHands/Meshes/hand_right");
    RightHandMesh = ConstructorHelpersInternal::FindOrLoadObject<USkeletalMesh>(defaultRightHandPath);
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::BeginPlay()
{
    UpdateHandTrackingData();

    if(CreateHandMeshComponents)
    {
        if(LeftHandMesh)
        {
            leftPoseable = NewObject<UPoseableMeshComponent>(GetOwner(), UPoseableMeshComponent::StaticClass());
            if(leftPoseable)
            {
                leftPoseable->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                leftPoseable->SetSkeletalMesh(LeftHandMesh);
                leftPoseable->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create PoseableMeshComponent!"));
            }
        }
        else
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent has no valid LeftHandMesh assigned to create a poseable component for!"));
        }

        if(RightHandMesh)
        {
            rightPoseable = NewObject<UPoseableMeshComponent>(GetOwner(), UPoseableMeshComponent::StaticClass());
            if(rightPoseable)
            {
                rightPoseable->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                rightPoseable->SetSkeletalMesh(RightHandMesh);
                rightPoseable->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create PoseableMeshComponent!"));
            }
        }
        else
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent has no valid RightHandMesh assigned to create a poseable component for!"));
        }
    }
    else if(LeftHandMeshComponentName.Len() != 0 && RightHandMeshComponentName.Len() != 0)
    {
        TArray<USceneComponent*> Children;
        GetChildrenComponents(false, Children);
        for(USceneComponent* child : Children)
        {
            if(!child)
                continue;

            if(child->GetName().Contains(LeftHandMeshComponentName))
            {
                leftPoseable = Cast<UPoseableMeshComponent>(child);
            }
            else if(child->GetName().Contains(RightHandMeshComponentName))
            {
                rightPoseable = Cast<UPoseableMeshComponent>(child);
            }
        }

        if(!leftPoseable)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent wasn't able to find left hand mesh component named: %s"), *LeftHandMeshComponentName);
        }
        if(!rightPoseable)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent wasn't able to find right hand mesh component named: %s"), *RightHandMeshComponentName);
        }
    }

    if(UpdateHandMeshComponents && UpdatePhysicsCapsules)
    {
        // Create the capsules now
        leftCapsules.SetNum(LeftHandSkeletonData.BoneCapsules.Num());
        for(int32 capsuleIndex = 0; capsuleIndex < leftCapsules.Num(); ++capsuleIndex)
        {
            UCapsuleComponent* capsuleComp = NewObject<UCapsuleComponent>(GetOwner(), UCapsuleComponent::StaticClass());
            if(capsuleComp)
            {
                leftCapsules[capsuleIndex] = capsuleComp;
                capsuleComp->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                capsuleComp->BodyInstance = CapsuleBodyData;
                capsuleComp->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create UCapsuleComponent!"));
            }
        }

        rightCapsules.SetNum(RightHandSkeletonData.BoneCapsules.Num());
        for(int32 capsuleIndex = 0; capsuleIndex < rightCapsules.Num(); ++capsuleIndex)
        {
            UCapsuleComponent* capsuleComp = NewObject<UCapsuleComponent>(GetOwner(), UCapsuleComponent::StaticClass());
            if(capsuleComp)
            {
                rightCapsules[capsuleIndex] = capsuleComp;
                capsuleComp->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
                capsuleComp->BodyInstance = CapsuleBodyData;
                capsuleComp->RegisterComponent();
            }
            else
            {
                UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent unable to create UCapsuleComponent!"));
            }
        }
    }

    Super::BeginPlay();
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if(!UQuestHandsFunctions::IsHandTrackingEnabled())
    {
        return;
    }

    UpdateHandTrackingData();

    // Do we have a poseable mesh to update? Do so!
    if(UpdateHandMeshComponents)
    {
        if(leftPoseable)
        {
            if(UpdateHandScale)
            {
                leftPoseable->SetRelativeScale3D(FVector(LeftHandTrackingData.HandScale));
            }

            FTransform rootPose(LeftHandTrackingData.RootPose.Orientation, LeftHandTrackingData.RootPose.Position, FVector::OneVector);
            leftPoseable->SetRelativeTransform(rootPose);
            UpdatePoseableWithBoneTransforms(leftPoseable, leftHandBones);
        }
        if(rightPoseable)
        {
            if(UpdateHandScale)
            {
                rightPoseable->SetRelativeScale3D(FVector(RightHandTrackingData.HandScale));
            }

            FTransform rootPose(RightHandTrackingData.RootPose.Orientation, RightHandTrackingData.RootPose.Position, FVector::OneVector);
            rightPoseable->SetRelativeTransform(rootPose);
            UpdatePoseableWithBoneTransforms(rightPoseable, rightHandBones);
        }

        if(UpdatePhysicsCapsules)
        {
            UpdateCapsules(leftCapsules, LeftHandSkeletonData);
            UpdateCapsules(rightCapsules, RightHandSkeletonData);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
bool UQuestHandsComponent::IsHandTrackingAvailable()
{
    return UQuestHandsFunctions::IsHandTrackingEnabled();
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::SaveHandDataDump()
{
    UQuestHandsDataDump *tmpTracking = GetMutableDefault<UQuestHandsDataDump>();
    tmpTracking->LeftHandSkeletonData = LeftHandSkeletonData;
    tmpTracking->LeftHandTrackingData = LeftHandTrackingData;
    tmpTracking->RightHandSkeletonData = RightHandSkeletonData;
    tmpTracking->RightHandTrackingData = RightHandTrackingData;
    tmpTracking->SaveConfig(CPF_Config, *(FPaths::ProjectSavedDir() / TEXT("HandTrackingDump.txt")));
    UE_LOG(LogQuestHands, Log, TEXT("Saving Hand Data Dump to %s"), *(FPaths::ProjectSavedDir() / TEXT("HandTrackingDump.txt")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::LoadHandDataDump()
{
    FString dataPath = FPaths::ProjectSavedDir() / TEXT("HandTrackingDump.txt");
    if(FPaths::FileExists(dataPath))
    {
        UQuestHandsDataDump *tmpTracking = GetMutableDefault<UQuestHandsDataDump>();
        tmpTracking->LoadConfig(NULL, *dataPath);

        LeftHandSkeletonData = tmpTracking->LeftHandSkeletonData;
        LeftHandTrackingData = tmpTracking->LeftHandTrackingData;
        RightHandSkeletonData = tmpTracking->RightHandSkeletonData;
        RightHandTrackingData = tmpTracking->RightHandTrackingData;

        SetupBoneTransforms(LeftHandSkeletonData, LeftHandTrackingData, leftHandBones, true);
        SetupBoneTransforms(RightHandSkeletonData, RightHandTrackingData, rightHandBones, false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::UpdateHandTrackingData()
{
    checkf(GetWorld(), TEXT("UQuestHandsComponent : Invalid world!?"));

    if(!UQuestHandsFunctions::IsHandTrackingEnabled())
    {
        return;
    }

    float worldToMeters = 100.0f;
    AWorldSettings* worldSettings = GetWorld()->GetWorldSettings();
    if(worldSettings)
    {
        worldToMeters = worldSettings->WorldToMeters;
    }

    // Get the latest hand skeleton and tracking data
    UQuestHandsFunctions::GetHandSkeleton_Internal(EControllerHand::Left, LeftHandSkeletonData, worldToMeters);
    UQuestHandsFunctions::GetHandSkeleton_Internal(EControllerHand::Right, RightHandSkeletonData, worldToMeters);
    UQuestHandsFunctions::GetTrackingState_Internal(EControllerHand::Left, LeftHandTrackingData, worldToMeters);
    UQuestHandsFunctions::GetTrackingState_Internal(EControllerHand::Right, RightHandTrackingData, worldToMeters);

    // Update our cached skeleton bone transforms
    SetupBoneTransforms(LeftHandSkeletonData, LeftHandTrackingData, leftHandBones, true);
    SetupBoneTransforms(RightHandSkeletonData, RightHandTrackingData, rightHandBones, false);
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::SetupBoneTransforms(const FQHandSkeleton& skeleton, const FQHandTrackingState& trackingState, TArray<FTransform>& boneTransforms, bool leftHand)
{
    // Don't do anything if we aren't tracked, but make sure we at least have a base line if it hasn't been established yet.
    if(!trackingState.IsTracked && boneTransforms.Num() == skeleton.Bones.Num())
        return;

    // Ensure the array size is correct
    boneTransforms.SetNum(skeleton.Bones.Num());

    for(int32 boneIndex = 0; boneIndex < skeleton.Bones.Num(); ++boneIndex)
    {
        const FQHandBone& bone = skeleton.Bones[boneIndex];
        if((int32)bone.BoneId != boneIndex)
        {
            UE_LOG(LogQuestHands, Warning, TEXT("UQuestHandsComponent::SetupBoneTransforms - bone ID mismatch!"));
        }

        FQuat rotationIn = bone.Pose.Orientation;
        if(trackingState.IsTracked)
        {
            rotationIn = trackingState.BoneRotations[boneIndex];
        }

        boneTransforms[boneIndex].SetComponents(rotationIn, 
                                                bone.Pose.Position, 
                                                FVector::OneVector);
        if(bone.ParentBoneIndex != -1)
        {
            // Apply our parents transform which converts us into world space and applies the root scale, etc
            boneTransforms[boneIndex] *= boneTransforms[bone.ParentBoneIndex];
        }
        else
        {
            // Apply our VR root transform AND our rootPose transform
            FTransform rootTransform(trackingState.RootPose.Orientation, 
                                     trackingState.RootPose.Position, 
                                     FVector(UpdateHandScale ? trackingState.HandScale : 1.0f));
            rootTransform *= GetComponentTransform();
            boneTransforms[boneIndex] *= rootTransform;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::UpdatePoseableWithBoneTransforms(UPoseableMeshComponent* poseable, const TArray<FTransform>& boneTransforms)
{
    if(!poseable)
    {
        return;
    }

    bool isLeftHand = poseable == leftPoseable;
    FQuat rotationOffset = isLeftHand ? LeftHandBoneRotationOffset.Quaternion() : RightHandBoneRotationOffset.Quaternion();

    for(int32 boneIndex = 0; boneIndex < boneTransforms.Num(); ++boneIndex)
    {
        FString boneName = UQuestHandsFunctions::GetHandBoneName((EQHandBones)boneIndex, isLeftHand);

        FTransform transSet = boneTransforms[boneIndex];
        transSet.SetRotation(transSet.GetRotation() * rotationOffset);

        poseable->SetBoneTransformByName(FName(*boneName), transSet, EBoneSpaces::WorldSpace);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
*/
void UQuestHandsComponent::UpdateCapsules(TArray<UCapsuleComponent*>& capsules, const FQHandSkeleton& skeleton)
{

}
