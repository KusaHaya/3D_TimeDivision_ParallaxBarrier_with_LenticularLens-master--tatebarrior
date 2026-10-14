using System;
using System.IO;
using ParallaxSimulation;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;

[InitializeOnLoad]
public static class SimulationSetup
{
    const string ScenePath="Assets/Scenes/Simulation.unity";
    static SimulationSetup()
    {
        if(!Application.isBatchMode) EditorApplication.delayCall+=()=> { if(!File.Exists(ScenePath)) CreateScene(); else Configure(); };
    }
    [MenuItem("Parallax/Open Simulation")]
    public static void Open()
    {
        if(!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
        if(!File.Exists(ScenePath)) CreateScene();else EditorSceneManager.OpenScene(ScenePath);
    }
    static void CreateScene()
    {
        Directory.CreateDirectory("Assets/Scenes");
        var scene=EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
        new GameObject("Parallax Simulation").AddComponent<SimulationController>();
        EditorSceneManager.SaveScene(scene,ScenePath);
        Configure();
    }
    static void Configure()
    {
        EditorBuildSettings.scenes=new[]{new EditorBuildSettingsScene(ScenePath,true)};
        PlayerSettings.companyName="ParallaxResearch";PlayerSettings.productName="Parallax Optical Simulator";
        PlayerSettings.defaultScreenWidth=1920;PlayerSettings.defaultScreenHeight=1080;
        PlayerSettings.fullScreenMode=FullScreenMode.ExclusiveFullScreen;PlayerSettings.resizableWindow=false;
        PlayerSettings.colorSpace=ColorSpace.Gamma;
        AssetDatabase.SaveAssets();
    }
    [MenuItem("Parallax/Run Model Checks")]
    public static void Checks()
    {
        ModelChecks.Run();
        HardwareChecks.Run();
        Debug.Log("PARALLAX: all geometric model checks passed.");
    }
    [MenuItem("Parallax/Build Windows Hardware App")]
    public static void Build()
    {
        if(!File.Exists(ScenePath)) CreateScene();
        Configure();
        Checks();
        var shader=Resources.Load<Shader>("Visibility");
        if(shader==null || ShaderUtil.ShaderHasError(shader)) throw new Exception("Visibility shader failed");
        shader=Resources.Load<Shader>("EyePreview");
        if(shader==null || ShaderUtil.ShaderHasError(shader)) throw new Exception("Eye preview shader failed");
        Directory.CreateDirectory("Builds/Windows");
        var result=BuildPipeline.BuildPlayer(new BuildPlayerOptions {
            scenes=new[]{ScenePath},locationPathName="Builds/Windows/ParallaxHardware.exe",
            target=BuildTarget.StandaloneWindows64,options=BuildOptions.None});
        if(result.summary.result!=BuildResult.Succeeded) throw new Exception("Build failed: "+result.summary.result);
        Debug.Log("PARALLAX: Windows build succeeded.");
    }
}
