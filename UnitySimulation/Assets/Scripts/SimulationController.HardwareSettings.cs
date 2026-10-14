using System;
using System.IO;
using UnityEngine;

namespace ParallaxSimulation
{
    public sealed partial class SimulationController
    {
        [Serializable] sealed class HardwareSettings
        {
            public int version=1;
            public OpticalSettings optics=new OpticalSettings();
            public string port="COM3",images="images/IDW",left="",right="";
            public int refresh=144,udpPort=8000,allocation=1,pattern,conflict;
            public float xSign=1,ySign=-1,tilt=8,gain=.7f,zOffset=110,haba=330,middle=3420,shift=1,slant=1;
            public bool swap;
        }
        string HardwareSettingsPath {get {return Path.Combine(Application.persistentDataPath,"hardware-settings.json");}}
        void SaveHardwareSettings()
        {
            try {
                var saved=new HardwareSettings {
                    optics=settings.Copy(),port=serialPort,images=imageDirectory,left=leftPath,right=rightPath,
                    refresh=requestedRefreshRate,udpPort=eyePort,allocation=allocationMode,pattern=imagePattern,conflict=conflictPolicy,
                    xSign=cameraXSign,ySign=cameraYSign,tilt=cameraTiltDegrees,gain=horizontalGain,zOffset=legacyCameraZOffsetMm,
                    haba=legacyHaba,middle=legacyMiddle,shift=legacyShift,slant=legacySlant,swap=swapImages};
                saved.optics.useMeasuredEyes=false;
                Directory.CreateDirectory(Application.persistentDataPath);
                File.WriteAllText(HardwareSettingsPath,JsonUtility.ToJson(saved,true));
                hardwareStatus="Saved hardware settings: "+HardwareSettingsPath;
            } catch(Exception e) {hardwareStatus="Save: "+e.Message;}
        }
        void LoadHardwareSettings()
        {
            if(!File.Exists(HardwareSettingsPath))return;
            try {
                var saved=JsonUtility.FromJson<HardwareSettings>(File.ReadAllText(HardwareSettingsPath));
                if(saved==null || saved.version!=1 || saved.optics==null)throw new FormatException("Unsupported settings file.");
                saved.optics.Validate();saved.optics.useMeasuredEyes=false;
                settings=saved.optics;activeSettings=settings.Copy();
                serialPort=saved.port;imageDirectory=saved.images;leftPath=saved.left;rightPath=saved.right;
                requestedRefreshRate=saved.refresh==60 || saved.refresh==120 || saved.refresh==144?saved.refresh:144;eyePort=saved.udpPort;
                allocationMode=Mathf.Clamp(saved.allocation,0,2);imagePattern=Mathf.Clamp(saved.pattern,0,3);conflictPolicy=Mathf.Clamp(saved.conflict,0,2);
                cameraXSign=saved.xSign;cameraYSign=saved.ySign;cameraTiltDegrees=saved.tilt;
                horizontalGain=saved.gain;legacyCameraZOffsetMm=saved.zOffset;
                legacyHaba=saved.haba;legacyMiddle=saved.middle;legacyShift=saved.shift;legacySlant=saved.slant;swapImages=saved.swap;
                if(!string.IsNullOrEmpty(leftPath) && !string.IsNullOrEmpty(rightPath)) {int pattern=imagePattern;LoadPair();imagePattern=pattern;hardwareStatus=status;}
                eyesCalibrated=hardwareTracking=false;
            } catch(Exception e) {hardwareStatus="Load: "+e.Message;}
        }
    }
}
