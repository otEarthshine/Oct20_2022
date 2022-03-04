// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class PrototypeCityTarget : TargetRules
{
	public PrototypeCityTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

        bUsesSteam = true;

        //Use these config options in shipping to enable logs, and to enable debugger.
        if (Configuration == UnrealTargetConfiguration.Shipping)
        {
            BuildEnvironment = TargetBuildEnvironment.Unique;
            bUseChecksInShipping = true;
            //bUseLoggingInShipping = true;
        }

        //bUseLoggingInShipping = true;

        //Target.bUseLoggingInShipping = true;

        //if (bBuildEditor)
        //{
        //    bUseLoggingInShipping = true;
        //}

        //bUseLoggingInShipping = true;

        ExtraModuleNames.AddRange( new string[] { "PrototypeCity" } );

        //GlobalDefinitions.Add("MEMPRO_ENABLED=1");
        //GlobalDefinitions.Add("MEMPRO_ENABLED=1");

        //DefaultBuildSettings = BuildSettingsVersion.V2;
    }
}
