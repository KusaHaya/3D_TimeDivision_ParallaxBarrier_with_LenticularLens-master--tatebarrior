using System;
using System.Collections;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

namespace ParallaxSimulation
{
    public sealed partial class SimulationController : MonoBehaviour
    {
        public OpticalSettings settings = new OpticalSettings();
        public int phase;
        bool animate, cycleView, spreadLayers=true, tracking=true;
        float zoomWidth=24, mapX, mapY, orbit=35;
        Vector2 scroll;
        Material visibility, lineMaterial;
        RenderTexture mapTexture, diagramTexture;
        Camera diagramCamera;
        LineRenderer[] lines;
        Task<CoverageReport> analysis;
        CancellationTokenSource cancellation;
        volatile float progress;
        CoverageReport report;
        OpticalSettings reportSettings, jobSettings;
        string status="Ready. Manual eyes; no camera or Arduino connection.", reportJson="";
        string capturePath;
        bool captureQueued;
        float nextFrame, phaseClock;
        int presentedPhase=-1;
        long presentationId;
        OpticalSettings activeSettings;
        GUIStyle title, small, heading;
        string OutputDirectory { get { return Path.Combine(Application.persistentDataPath,"Results"); } }

        void Awake()
        {
            Application.targetFrameRate=60;
            QualitySettings.vSyncCount=1;
            var background=new GameObject("Screen background");background.transform.SetParent(transform);
            var screenCamera=background.AddComponent<Camera>();screenCamera.cullingMask=0;
            screenCamera.clearFlags=CameraClearFlags.SolidColor;screenCamera.backgroundColor=new Color(.02f,.03f,.05f);
            visibility=new Material(Resources.Load<Shader>("Visibility"));
            lineMaterial=new Material(Resources.Load<Shader>("Diagram"));
            mapTexture=new RenderTexture(1024,432,0,RenderTextureFormat.ARGB32,RenderTextureReadWrite.Linear);
            mapTexture.filterMode=FilterMode.Point; mapTexture.Create();
            diagramTexture=new RenderTexture(900,600,24); diagramTexture.Create();
            var go=new GameObject("Geometry camera"); go.transform.SetParent(transform);
            diagramCamera=go.AddComponent<Camera>(); diagramCamera.enabled=false;
            diagramCamera.clearFlags=CameraClearFlags.SolidColor;
            diagramCamera.backgroundColor=new Color(.025f,.035f,.06f);
            diagramCamera.nearClipPlane=.1f; diagramCamera.farClipPlane=5000;
            diagramCamera.cullingMask=1<<30; diagramCamera.targetTexture=diagramTexture;
            lines=new LineRenderer[32];
            for(int n=0;n<lines.Length;n++)
            {
                var l=new GameObject("Diagram line "+n); l.layer=30; l.transform.SetParent(transform);
                lines[n]=l.AddComponent<LineRenderer>(); lines[n].sharedMaterial=new Material(lineMaterial);
                lines[n].useWorldSpace=true; lines[n].widthMultiplier=1.5f;
            }
            activeSettings=settings.Copy();
            InitImages();
            string[] args=Environment.GetCommandLineArgs();
            for(int i=0;i+1<args.Length;i++) if(args[i]=="--capture") capturePath=args[i+1];
            InitHardware(screenCamera);
            if(!hardwareMode) StartAnalysis();
        }
        void Update()
        {
            UpdateHardware();
            if(hardwareMode)return;
            if(animate) { phaseClock+=Time.unscaledDeltaTime; if(phaseClock>=.5f) { phase=(phase+1)%4;phaseClock=0; } }
            if(analysis!=null && analysis.IsCompleted)
            {
                if(analysis.IsFaulted) status="Analysis failed: "+analysis.Exception.GetBaseException().Message;
                else if(analysis.IsCanceled) status="Analysis cancelled.";
                else { report=analysis.Result; reportSettings=jobSettings;reportJson=JsonUtility.ToJson(reportSettings); status="Scan complete. Results are for a static eye pose, RGB centres only."; }
                analysis=null; cancellation.Dispose(); cancellation=null;
            }
            // A phase change always renders a complete matching set immediately.
            // Never update the light phase while keeping an old LCD/eye preview.
            if(displayTab!=2 && (phase!=presentedPhase || Time.unscaledTime>=nextFrame)) { nextFrame=Time.unscaledTime+.1f; DrawPreviews(); }
            if(capturePath!=null && !captureQueued && report!=null)
            { captureQueued=true; StartCoroutine(CaptureAndQuit()); }
        }
        void ToggleTracking() { tracking=!tracking; if(!tracking) activeSettings=settings.Copy(); }
        OpticalSettings Current()
        {
            settings.useLens=true;settings.SyncLens();var copy=settings.Copy();copy.lens.phase=0;
            copy.useMeasuredEyes=eyesCalibrated;
            if(eyesCalibrated) {
                copy.measuredLeft=new Point3(measuredLeft.x,measuredLeft.y,measuredLeft.z);
                copy.measuredRight=new Point3(measuredRight.x,measuredRight.y,measuredRight.z);
            }
            return copy;
        }
        OpticalSettings Allocation()
        {
            var s=Current();
            if(!tracking) { s.headX=activeSettings.headX;s.headY=activeSettings.headY;s.headZ=activeSettings.headZ;s.ipd=activeSettings.ipd;s.yawDegrees=activeSettings.yawDegrees; }
            return s;
        }
        IEnumerator CaptureAndQuit()
        {
            yield return new WaitForSeconds(1);
            yield return new WaitForEndOfFrame();
            Export();
            ScreenCapture.CaptureScreenshot(capturePath);
            yield return new WaitForSeconds(1);
            Application.Quit(0);
        }
        static Vector4 EyeVector(Point3 p) { return new Vector4((float)p.x,(float)p.y,(float)p.z,0); }
        void DrawPreviews(bool forceImages=false)
        {
            var s=Current();
            int framePhase=((phase%4)+4)%4;
            try { s.Validate(); } catch(Exception e) { status=e.Message; return; }
            SetLensUniforms(visibility,s);
            visibility.SetVector("_EyeL",EyeVector(s.Eye(false)));visibility.SetVector("_EyeR",EyeVector(s.Eye(true)));
            visibility.SetVector("_Region",new Vector4(mapX,mapY,zoomWidth,zoomWidth*432/1024));
            visibility.SetVector("_Panel",new Vector4((float)s.panelWidth,(float)s.panelHeight,s.pixelsX,s.pixelsY));
            visibility.SetFloat("_Gap",(float)s.Gap);visibility.SetFloat("_Period",(float)s.sourcePeriod);
            visibility.SetFloat("_Width",(float)s.sourceWidth);visibility.SetFloat("_Step",(float)s.rawPhaseStep);
            visibility.SetFloat("_Offset",(float)s.sourceOffset);visibility.SetFloat("_Slope",(float)s.slope);
            visibility.SetFloat("_Direction",s.phaseDirection);visibility.SetFloat("_Phase",framePhase);visibility.SetFloat("_Mode",cycleView?1:0);
            Graphics.Blit(Texture2D.whiteTexture,mapTexture,visibility);
            DrawDiagram(s,framePhase);
            if(displayTab==0 || forceImages) RenderImages(s,framePhase);
            presentedPhase=framePhase;
            presentationId++;
        }
        Vector3 V(Point3 p) { return new Vector3((float)p.x,(float)p.y,(float)p.z); }
        void Line(int index,Color color,params Vector3[] points)
        { var l=lines[index];l.enabled=true;l.positionCount=points.Length;l.SetPositions(points);l.sharedMaterial.SetColor("_Color",color); }
        void Rectangle(int index,float w,float h,float z,Color c)
        { Line(index,c,new Vector3(-w/2,-h/2,z),new Vector3(w/2,-h/2,z),new Vector3(w/2,h/2,z),new Vector3(-w/2,h/2,z),new Vector3(-w/2,-h/2,z)); }
        void DrawDiagram(OpticalSettings s,int framePhase)
        {
            foreach(var l in lines) l.enabled=false;
            var lens=s.lens;
            float vertexZ=spreadLayers?-80:-(float)lens.screenDistance;
            float flatZ=spreadLayers?-105:-(float)(lens.screenDistance+lens.thickness);
            float ledZ=spreadLayers?-220:flatZ-(float)lens.ledDistance;
            float imageZ=spreadLayers?-50:(float)(lens.ImageDistance-lens.screenDistance);
            Rectangle(0,(float)s.panelWidth,(float)s.panelHeight,0,Color.white);
            Rectangle(1,(float)s.panelWidth,(float)s.panelHeight,vertexZ,new Color(.4f,.4f,.6f));
            Rectangle(2,(float)s.panelWidth,(float)s.panelHeight,imageZ,new Color(1,.65f,.15f));
            Rectangle(3,(float)s.panelWidth,(float)s.panelHeight,flatZ,new Color(.3f,.3f,.5f));
            int index=4;
            for(int e=0;e<2;e++) {
                Point3 eye=s.Eye(e==1);Color color=e==0?new Color(.22f,.9f,.5f):new Color(.22f,.52f,1);
                Vector3 ev=V(eye);Line(index++,color,ev+Vector3.left*7,ev+Vector3.right*7);
                for(int n=-2;n<=2;n++) {
                    double px=n*40,py=0,tx=(eye.x-px)/eye.z,ty=(eye.y-py)/eye.z;
                    double vx=px-lens.screenDistance*tx,vy=py-lens.screenDistance*ty;
                    double local=OpticalModel.Wrap(vx+lens.slope*vy,lens.HorizontalPitch);
                    double power=local/(lens.FocalLength*(1+lens.slope*lens.slope));
                    double ax=tx+power,ay=ty+power*lens.slope;
                    double fx=vx-lens.thickness*ax/lens.refractiveIndex,fy=vy-lens.thickness*ay/lens.refractiveIndex;
                    double lx=fx-lens.ledDistance*ax,ly=fy-lens.ledDistance*ay;
                    bool lit=lens.IsLit(lx+lens.slope*ly,framePhase);
                    Line(index++,lit?color:new Color(.2f,.2f,.2f),
                        new Vector3((float)lx,(float)ly,ledZ),new Vector3((float)fx,(float)fy,flatZ),
                        new Vector3((float)vx,(float)vy,vertexZ),new Vector3((float)px,(float)py,0),ev);
                }
            }
            float a=orbit*Mathf.Deg2Rad;
            var target=new Vector3(0,0,220);
            diagramCamera.transform.position=target+new Vector3(Mathf.Sin(a)*1050,380,Mathf.Cos(a)*1050);
            diagramCamera.transform.LookAt(target); diagramCamera.fieldOfView=48;
            diagramCamera.Render();
        }
        void Slider(string label,ref double value,float min,float max,string format="F3")
        {
            GUILayout.Label(label+": "+value.ToString(format,System.Globalization.CultureInfo.InvariantCulture));
            float old=(float)value;
            float changed=GUILayout.HorizontalSlider(old,min,max);
            if(changed!=old) value=changed;
        }
        void OnGUI()
        {
            if(hardwareMode) {HardwareKeys();return;}
            // IMGUI key event also works when Unity 6 uses the new Input System only.
            // Do not toggle while a path text field is being edited.
            if(Event.current.type==EventType.KeyDown && Event.current.keyCode==KeyCode.H && GUIUtility.keyboardControl==0)
            { ToggleTracking();Event.current.Use(); }
            if(title==null)
            { title=new GUIStyle(GUI.skin.label){fontSize=24,fontStyle=FontStyle.Bold};heading=new GUIStyle(GUI.skin.label){fontSize=16,fontStyle=FontStyle.Bold};small=new GUIStyle(GUI.skin.label){fontSize=12,wordWrap=true}; }
            float scale=Mathf.Min(Screen.width/1440f,Screen.height/960f);
            GUI.matrix=Matrix4x4.Scale(new Vector3(scale,scale,1));
            GUI.Box(new Rect(0,0,1440,960),"");
            GUI.Label(new Rect(20,12,1390,35),"TIME-DIVISION PARALLAX  /  OPTICAL SIMULATOR",title);
            GUI.Label(new Rect(20,50,1380,40),"Fixed oblique source images  >  LCD allocation  >  tracked eyes    |    F1 returns from hardware output to these controls",small);
            displayTab=GUI.Toolbar(new Rect(390,92,1024,28),displayTab,new[]{"IMAGE TEST","GEOMETRY","LED / LENS TEST"});
            if(displayTab==2) {
                GUILayout.BeginArea(new Rect(15,95,350,845),GUI.skin.box);
                scroll=GUILayout.BeginScrollView(scroll);LensControls();GUILayout.EndScrollView();GUILayout.EndArea();
                DrawLensPanels();return;
            }
            GUILayout.BeginArea(new Rect(15,95,350,845),GUI.skin.box);
            scroll=GUILayout.BeginScrollView(scroll);
            HardwareControls();
            ImageControls();
            GUILayout.Label("1  OBSERVER",heading);
            bool next=GUILayout.Toggle(tracking,"Tracking ON [h] - update LCD allocation"); if(next!=tracking) ToggleTracking();
            Slider("Head X (mm)",ref settings.headX,-250,250,"F1");Slider("Head Y (mm)",ref settings.headY,-150,150,"F1");
            Slider("Eye distance from LCD (mm)",ref settings.headZ,150,1500,"F1");Slider("Eye separation (mm)",ref settings.ipd,40,80,"F1");
            Slider("Head yaw / eye depth difference (deg)",ref settings.yawDegrees,-40,40,"F1");
            GUILayout.Label("2  OPTICS",heading);
            LensOpticsControls();
            GUILayout.Label("Image distance from vertex: "+settings.lens.ImageDistance.ToString("F4")+" mm");
            GUILayout.Label("Image -> LCD: "+(settings.lens.screenDistance-settings.lens.ImageDistance).ToString("F4")+" mm");
            GUILayout.Label("3  PHASE / PREVIEW",heading);
            phase=GUILayout.Toolbar(phase,new[]{"0","1","2","3"});
            animate=GUILayout.Toggle(animate,"Animate phases slowly (2 phases/sec)");
            if(GUILayout.Button("Next phase - light + LCD together")) {animate=false;phase=(phase+1)%4;}
            GUILayout.Label("Presented: light="+presentedPhase+" / LCD="+presentedPhase+"  (#"+presentationId+")",small);
            cycleView=GUILayout.Toggle(cycleView,"Show exclusive coverage over all 4 phases");
            GUILayout.Label("Preview width: "+zoomWidth.ToString("F1")+" mm");zoomWidth=GUILayout.HorizontalSlider(zoomWidth,4,528);
            GUILayout.BeginHorizontal();if(GUILayout.Button("Detail 24mm")) zoomWidth=24;if(GUILayout.Button("Full LCD")) zoomWidth=528;GUILayout.EndHorizontal();
            GUILayout.Label("Map centre X / Y (mm)");mapX=GUILayout.HorizontalSlider(mapX,-250,250);mapY=GUILayout.HorizontalSlider(mapY,-140,140);
            spreadLayers=GUILayout.Toggle(spreadLayers,"Separate planes in 3D diagram (visual only)");
            GUILayout.Label("3D orbit");orbit=GUILayout.HorizontalSlider(orbit,-130,130);
            GUILayout.Label("4  VERIFY / SAVE",heading);
            GUI.enabled=analysis==null;
            if(GUILayout.Button("Scan all 1920 x 1080 RGB centres")) StartAnalysis();
            GUI.enabled=true;
            if(analysis!=null) { GUILayout.Label("Scanning: "+(progress*100).ToString("F0")+"%");if(GUILayout.Button("Cancel scan")) cancellation.Cancel(); }
            if(GUILayout.Button("Export current preview + settings + last scan")) Export();
            GUILayout.BeginHorizontal();
            if(GUILayout.Button("Save settings")) SaveSettings();if(GUILayout.Button("Load settings")) LoadSettings();
            GUILayout.EndHorizontal();
            if(GUILayout.Button("Reset theory defaults")) { settings=new OpticalSettings();lensKey="";tracking=true;phase=0;zoomWidth=24;mapX=mapY=0; }
            GUILayout.EndScrollView();GUILayout.EndArea();

            if(displayTab==0) {DrawImagePanels();return;}
            GUI.Label(new Rect(390,126,1000,28),cycleView?"FOUR-PHASE EXCLUSIVE COVERAGE":"LCD SUBPIXEL VISIBILITY  /  PHASE "+presentedPhase,heading);
            GUI.DrawTexture(new Rect(390,157,1024,402),mapTexture,ScaleMode.ScaleToFit,false);
            GUI.Label(new Rect(390,565,1020,42),cycleView?
                "GREEN: left only has a slot    BLUE: right only has a slot    WHITE: both have separate slots    DARK: neither\nA slot must be exclusive; overlap is not counted as correct stereo delivery.":
                "GREEN: left eye only    BLUE: right eye only    PINK: both eyes (conflict)    DARK: neither eye\nPreview samples RGB centres. Use detail view: the full-panel preview can alias.",small);
            GUI.DrawTexture(new Rect(390,620,490,290),diagramTexture,ScaleMode.ScaleToFit,false);
            GUI.Label(new Rect(390,912,490,35),"3D: sparse representative lines/rays; "+(spreadLayers?"layer spacing exaggerated":"physical layer spacing"),small);
            GUILayout.BeginArea(new Rect(900,620,510,320),GUI.skin.box);
            GUILayout.Label("MODEL / VERIFICATION",heading);
            if(report!=null)
            {
                bool stale=JsonUtility.ToJson(Current())!=reportJson;
                GUILayout.Label(stale?"PARAMETERS CHANGED - scan again":"Scan matches current parameters",heading);
                GUILayout.Label(report.Summary(),small);
            }
            GUILayout.Space(8);
            GUILayout.Label("LCD: 528 x 297 mm, 1920 x 1080, assumed RGB stripe.\nReverse rays through flat-entry / convex-exit PMMA, paraxial model. Uniform LED radiance; no diffraction, aberration, pupil aperture, LCD response or LED timing model.\nCoverage is necessary, not sufficient, for full-resolution stereo.",small);
            GUILayout.Label(status,small);
            GUILayout.EndArea();
        }
        void StartAnalysis()
        {
            if(analysis!=null) return;
            jobSettings=Current();
            try {jobSettings.Validate();} catch(Exception e) {status="Cannot scan: "+e.Message;return;}
            progress=0;cancellation=new CancellationTokenSource();
            var token=cancellation.Token;var snapshot=jobSettings;
            analysis=Task.Run(()=>CoverageAnalyzer.Analyze(snapshot,p=>progress=p,token),token);
            status="Scanning 6,220,800 RGB centres across four phases...";
        }
        void SaveSettings()
        { try { Directory.CreateDirectory(OutputDirectory);File.WriteAllText(Path.Combine(OutputDirectory,"settings.json"),JsonUtility.ToJson(settings,true));status="Saved: "+OutputDirectory; } catch(Exception e) {status=e.Message;} }
        void LoadSettings()
        { try { var s=JsonUtility.FromJson<OpticalSettings>(File.ReadAllText(Path.Combine(OutputDirectory,"settings.json")));s.useLens=true;s.Validate();settings=s;lensKey="";status="Settings loaded (shared lens optics)."; } catch(Exception e) {status=e.Message;} }
        void Export()
        {
            try
            {
                Directory.CreateDirectory(OutputDirectory);
                string prefix=Path.Combine(OutputDirectory,DateTime.Now.ToString("yyyyMMdd-HHmmss"));
                var s=Current(); DrawPreviews(true);
                File.WriteAllText(prefix+"-preview-settings.json",JsonUtility.ToJson(s,true));
                File.WriteAllText(prefix+"-preview.txt","phase="+phase+"\ncycleView="+cycleView+"\nregionCentreMm="+mapX+","+mapY+"\nregionWidthMm="+zoomWidth+"\n");
                var old=RenderTexture.active;RenderTexture.active=mapTexture;
                var tex=new Texture2D(mapTexture.width,mapTexture.height,TextureFormat.RGB24,false);
                tex.ReadPixels(new Rect(0,0,tex.width,tex.height),0,0);tex.Apply();RenderTexture.active=old;
                File.WriteAllBytes(prefix+"-preview.png",tex.EncodeToPNG());Destroy(tex);
                ExportEyeViews(prefix);
                if(report!=null) { report.WriteCsv(prefix+"-scan.csv",reportSettings);File.WriteAllText(prefix+"-scan-settings.json",JsonUtility.ToJson(reportSettings,true)); }
                status="Exported: "+OutputDirectory;
            }
            catch(Exception e) { status="Export failed: "+e.Message; }
        }
        void OnDestroy()
        {
            if(cancellation!=null) cancellation.Cancel();
            if(mapTexture!=null) {mapTexture.Release();Destroy(mapTexture);} if(diagramTexture!=null) {diagramTexture.Release();Destroy(diagramTexture);}
            if(lines!=null) foreach(var l in lines) if(l!=null) Destroy(l.sharedMaterial);
            Destroy(visibility);Destroy(lineMaterial);
            DestroyHardware();
            DestroyImages();
            if(illumination!=null)Destroy(illumination);
        }
    }
}
