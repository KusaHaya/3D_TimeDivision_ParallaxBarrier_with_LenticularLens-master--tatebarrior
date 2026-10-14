using System;
using System.Collections;
using System.IO;
using UnityEngine;

namespace ParallaxSimulation
{
    public sealed partial class SimulationController
    {
        [Header("Hardware output / EyeTracker-RS (OSC metres)")]
        public string serialPort="COM3";
        public int eyePort=8000;
        public float cameraXSign=1,cameraYSign=-1,cameraTiltDegrees=8;
        public float horizontalGain=.7f;
        public float legacyCameraZOffsetMm=110;
        public int requestedRefreshRate=144;
        public string imageDirectory="images/IDW";
        HardwareOutputCamera hardwareCamera;
        ArduinoLink arduino=new ArduinoLink();
        EyeOscReceiver eyeReceiver=new EyeOscReceiver();
        bool hardwareMode,hardwareRunning,hardwareTracking,eyesCalibrated,holdPending;
        int outputPhase=-1;
        float serialReadyTime;
        Vector3 calibrationCenter,calibrationTarget,measuredLeft,measuredRight;
        string hardwareStatus="Disconnected. F1: settings / t: run-hold / q w e r: phase / h: tracking / Esc: quit";

        void InitHardware(Camera camera)
        {
            hardwareCamera=camera.gameObject.AddComponent<HardwareOutputCamera>();
            camera.allowHDR=false;camera.allowMSAA=false;camera.allowDynamicResolution=false;
            LoadHardwareSettings();
            string port=Environment.GetEnvironmentVariable("PROJECTPB_SERIAL_PORT");
            if(!string.IsNullOrEmpty(port))serialPort=port;
            StartCoroutine(HardwareAfterFrame());
            if(!Application.isEditor && Array.IndexOf(Environment.GetCommandLineArgs(),"--simulator")<0) EnterHardware();
        }
        void EnterHardware()
        {
            StopHardware();
            if(cancellation!=null)cancellation.Cancel();
            animate=false;phase=0;outputPhase=-1;hardwareMode=true;hardwareCamera.active=true;
            settings.pixelsX=1920;settings.pixelsY=1080;
            QualitySettings.antiAliasing=0;QualitySettings.vSyncCount=1;Application.targetFrameRate=-1;
#if UNITY_6000_0_OR_NEWER
            Screen.SetResolution(1920,1080,FullScreenMode.ExclusiveFullScreen,new RefreshRate {numerator=(uint)Mathf.Max(1,requestedRefreshRate),denominator=1});
#else
            Screen.SetResolution(1920,1080,FullScreenMode.ExclusiveFullScreen,requestedRefreshRate);
#endif
            Cursor.visible=false;holdPending=arduino.IsOpen;
        }
        bool NativeOutput {get {return Screen.width==1920 && Screen.height==1080 && Screen.fullScreen;}}
        void StopHardware()
        {
            hardwareRunning=false;holdPending=false;
            if(arduino.IsOpen)try {arduino.Send(13);arduino.Send(110);}catch(Exception e) {hardwareStatus=e.Message;arduino.Dispose();}
        }
        void LeaveHardware()
        {
            StopHardware();hardwareMode=false;hardwareCamera.active=false;Cursor.visible=true;
            displayTab=0;
        }
        void ConnectHardware()
        {
            StopHardware();arduino.Dispose();
            try {
                if(Application.platform!=RuntimePlatform.WindowsPlayer && Application.platform!=RuntimePlatform.WindowsEditor)
                    throw new InvalidOperationException("Serial output requires Windows.");
                arduino.Open(serialPort);serialReadyTime=Time.realtimeSinceStartup+2;
                hardwareStatus="Connected: "+serialPort+" / 115200. Wait 2 seconds for Arduino startup, then enter output.";
            } catch(Exception e) {hardwareStatus="Serial: "+e.Message;}
        }
        void UpdateHardware()
        {
            try {eyeReceiver.Poll();}catch(Exception e) {hardwareStatus="OSC: "+e.Message;eyeReceiver.Dispose();hardwareTracking=false;}
            if(hardwareTracking && eyeReceiver.Fresh) {
                measuredLeft=MapEye(eyeReceiver.Left)-calibrationCenter+calibrationTarget;
                measuredRight=MapEye(eyeReceiver.Right)-calibrationCenter+calibrationTarget;
                Vector3 centre=(measuredLeft+measuredRight)*.5f;
                settings.headX=centre.x;settings.headY=centre.y;settings.headZ=centre.z;
                settings.ipd=Vector3.Distance(measuredLeft,measuredRight);
                if(allocationMode!=0) {
                    legacyHaba=Mathf.Max(1,330+(int)((calibrationTarget.z-centre.z)/2));
                    // Original empirical lateral correction, in the calibrated camera frame (metres).
                    float rawZ=(((MapEye(eyeReceiver.Left)+MapEye(eyeReceiver.Right))*.5f).z+legacyCameraZOffsetMm)/1000;
                    if(Mathf.Abs(rawZ-.3245f)>.02f)
                        legacyMiddle=3420+(float)Math.Round(horizontalGain*.895/(rawZ-.3245)*(centre.x-calibrationTarget.x)/.09,MidpointRounding.AwayFromZero);
                    legacyDepthCorrection=false;
                }
            }
            if(!hardwareMode)return;
            if(!NativeOutput) {
                if(hardwareRunning || holdPending || hardwareCamera.output!=null)StopHardware();
                hardwareCamera.output=null;hardwareStatus="Output paused: requires 1920 x 1080 fullscreen. Actual: "+Screen.width+" x "+Screen.height;
                return;
            }
            try {RenderImages(Current(),phase,true);if(outputPhase<0)holdPending=arduino.IsOpen;outputPhase=phase;hardwareCamera.output=imageViews[2];}
            catch(Exception e) {LeaveHardware();hardwareCamera.output=null;hardwareStatus="Output: "+e.Message;}
        }
        Vector3 MapEye(double[] value)
        {
            float y=(float)value[1]*cameraYSign*1000,z=(float)value[2]*1000;
            float angle=cameraTiltDegrees*Mathf.Deg2Rad;
            return new Vector3((float)value[0]*cameraXSign*1000,Mathf.Cos(angle)*y+Mathf.Sin(angle)*z,Mathf.Cos(angle)*z-Mathf.Sin(angle)*y);
        }
        void ToggleHardwareTracking()
        {
            if(hardwareTracking) {hardwareTracking=false;hardwareStatus="Tracking frozen at last eye positions.";return;}
            if(!eyeReceiver.Fresh) {hardwareStatus="No fresh OSC eye pair. Start receiver and EyeTracker-RS first.";return;}
            calibrationCenter=(MapEye(eyeReceiver.Left)+MapEye(eyeReceiver.Right))*.5f;
            calibrationTarget=new Vector3((float)settings.headX,(float)settings.headY,(float)settings.headZ);
            measuredLeft=MapEye(eyeReceiver.Left)-calibrationCenter+calibrationTarget;
            measuredRight=MapEye(eyeReceiver.Right)-calibrationCenter+calibrationTarget;
            eyesCalibrated=hardwareTracking=true;tracking=true;
            hardwareStatus="Tracking calibrated at current manual head position (mm).";
        }
        IEnumerator HardwareAfterFrame()
        {
            while(true) {
                yield return new WaitForEndOfFrame();
                if(!hardwareMode || !NativeOutput || hardwareCamera.renderedFrame!=Time.frameCount || outputPhase!=phase || !arduino.IsOpen || Time.realtimeSinceStartup<serialReadyTime)continue;
                try {
                    // Mirrors ProjectPB's post-render command order. This is NOT a physical display-present acknowledgement.
                    if(hardwareRunning) {arduino.Send((byte)(200+outputPhase));phase=(outputPhase+1)%4;}
                    else if(holdPending) {arduino.Send((byte)(210+outputPhase));holdPending=false;}
                } catch(Exception e) {hardwareStatus="Serial output stopped: "+e.Message;StopHardware();arduino.Dispose();}
            }
        }
        void HardwareKeys()
        {
            Event e=Event.current;if(e.type!=EventType.KeyDown)return;
            if(e.keyCode==KeyCode.F1) {LeaveHardware();e.Use();return;}
            if(e.keyCode==KeyCode.Escape) {StopHardware();Application.Quit();if(Application.isEditor)LeaveHardware();e.Use();return;}
            if(e.keyCode==KeyCode.H)ToggleHardwareTracking();
            if(e.keyCode==KeyCode.T) {
                if(hardwareRunning) {hardwareRunning=false;phase=Math.Max(0,outputPhase);holdPending=true;}
                else if(arduino.IsOpen && Time.realtimeSinceStartup>=serialReadyTime && NativeOutput) {
                    try {arduino.Send(20);arduino.Send(12);phase=0;hardwareRunning=true;holdPending=false;}
                    catch(Exception ex){hardwareStatus=ex.Message;StopHardware();}
                } else hardwareStatus="Cannot run: connect Arduino and use 1920 x 1080 fullscreen.";
            }
            if(e.shift && e.keyCode==KeyCode.R) swapImages=!swapImages;
            else if(!hardwareRunning) {
                if(e.keyCode==KeyCode.Q)phase=0;if(e.keyCode==KeyCode.W)phase=1;
                if(e.keyCode==KeyCode.E)phase=2;if(e.keyCode==KeyCode.R)phase=3;
                if(e.keyCode==KeyCode.K)phase=(phase+1)%4;
                holdPending=true;
            }
            if(e.keyCode==KeyCode.O)legacyShift=(legacyShift+1)%8;
            if(e.keyCode>=KeyCode.Alpha0 && e.keyCode<=KeyCode.Alpha7) {
                int number=(int)e.keyCode-(int)KeyCode.Alpha0+1;
                string root=Path.IsPathRooted(imageDirectory)?imageDirectory:Path.Combine(Path.GetDirectoryName(Application.dataPath),imageDirectory);
                leftPath=Path.Combine(root,number.ToString("000")+"l.ppm");
                rightPath=Path.Combine(root,number.ToString("000")+"r.ppm");LoadPair();hardwareStatus=status;
            }
            if(e.keyCode==KeyCode.LeftBracket)legacyHaba++;
            if(e.keyCode==KeyCode.RightBracket)legacyHaba=Mathf.Max(1,legacyHaba-1);
            if(e.keyCode==KeyCode.F)legacySlant=Mathf.Max(.1f,legacySlant+(e.shift?.02f:-.02f));
            if(e.keyCode==KeyCode.J)horizontalGain=Mathf.Clamp(horizontalGain+(e.shift?.1f:-.1f),.1f,10);
            e.Use();
        }
        void HardwareControls()
        {
            GUILayout.Label("HARDWARE / 1920 x 1080",heading);
            GUILayout.Label("F1: settings (LEDs off) | Esc: quit | t: run/hold | q/w/e/r: phase | h: track",small);
            serialPort=GUILayout.TextField(serialPort);
            GUILayout.Label("Original image directory (keys 0-7 = pairs 001-008)",small);
            imageDirectory=GUILayout.TextField(imageDirectory);
            if(GUILayout.Button(arduino.IsOpen?"Reconnect Arduino":"Connect Arduino (115200)"))ConnectHardware();
            if(GUILayout.Button("Start OSC receiver / localhost:"+eyePort)) {
                try {eyeReceiver.Open(eyePort);hardwareTracking=eyesCalibrated=false;hardwareStatus="OSC ready. Start EyeTracker-RS; press h at the manual reference head position.";}
                catch(Exception e){hardwareStatus="OSC: "+e.Message;}
            }
            GUILayout.Label("OSC pair: "+(eyeReceiver.Fresh?"live":"missing / stale")+" | Tracking: "+hardwareTracking,small);
            if(GUILayout.Button("Calibrate / freeze eye tracking [h]"))ToggleHardwareTracking();
            if(GUILayout.Button("Use manual eye positions")) {hardwareTracking=eyesCalibrated=false;tracking=true;}
            if(GUILayout.Button("Camera X direction: "+cameraXSign))cameraXSign=-cameraXSign;
            GUILayout.Label("Camera tilt (degrees): "+cameraTiltDegrees.ToString("F1"),small);
            cameraTiltDegrees=GUILayout.HorizontalSlider(cameraTiltDegrees,-30,30);
            GUILayout.Label("Requested refresh: "+requestedRefreshRate+" Hz (Windows display must support it)",small);
            int[] rates={60,120,144};
            int selected=Array.IndexOf(rates,requestedRefreshRate);
            requestedRefreshRate=rates[GUILayout.SelectionGrid(selected<0?2:selected,new[]{"60 Hz","120 Hz","144 Hz"},3)];
            GUILayout.Label(hardwareStatus,small);
            if(GUILayout.Button("Save hardware / optical settings"))SaveHardwareSettings();
            if(GUILayout.Button("ENTER FULLSCREEN LCD OUTPUT [F1 to return]"))EnterHardware();
            GUILayout.Space(10);
        }
        void OnApplicationFocus(bool focus) {if(!focus)StopHardware();}
        void OnApplicationQuit() {StopHardware();arduino.Dispose();eyeReceiver.Dispose();}
        void DestroyHardware() {StopHardware();arduino.Dispose();eyeReceiver.Dispose();Cursor.visible=true;}
    }
}
