// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class PrototypeCity : ModuleRules
{
	public PrototypeCity(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "ProceduralMeshComponent", "RenderCore", "MoviePlayer", "MediaAssets" });

        PublicDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });

        //PublicDependencyModuleNames.AddRange(new string[] { "AudioMixer" });

        PublicDependencyModuleNames.AddRange(new string[] {
             "OnlineSubsystem",
             "OnlineSubsystemUtils",
        });
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

        PublicDependencyModuleNames.AddRange(new string[] {
            //"KantanChartsSlate",
            "KantanChartsDatasource",
            "KantanChartsUMG",
        });

        AddEngineThirdPartyPrivateStaticDependencies(Target,
            "UEOgg",
            "Vorbis",
            "VorbisFile"
        );

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // 4.25 problem??
        bLegacyPublicIncludePaths = false;

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
