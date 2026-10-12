using System;
using System.IO;
using UnityEngine;

namespace ParallaxSimulation
{
    public sealed partial class SimulationController
    {
        [Header("Optional stereo pair: 1920 x 1080 per eye")]
        public Texture2D leftImage, rightImage;
        Material eyePreview;
        RenderTexture[] imageViews;
        RenderTexture[] phaseDrives;
        string driveCacheKey="";
        int imagePhase=-1;
        bool imageCyclePresented;
        float imageGainPresented;
        Texture2D ownedLeft,ownedRight;
        string leftPath="",rightPath="";
        int displayTab, imagePattern, conflictPolicy;
        bool integrateCycle=true;
        float imageGain=4;
        void InitImages()
        {
            eyePreview=new Material(Resources.Load<Shader>("EyePreview"));
            imageViews=new RenderTexture[5];
            phaseDrives=new RenderTexture[4];
        }
        void EnsureImageTargets(OpticalSettings s)
        {
            for(int i=0;i<imageViews.Length;i++)
            {
                if(imageViews[i]!=null && imageViews[i].width==s.pixelsX && imageViews[i].height==s.pixelsY) continue;
                if(imageViews[i]!=null) {imageViews[i].Release();Destroy(imageViews[i]);}
                imageViews[i]=new RenderTexture(s.pixelsX,s.pixelsY,0,RenderTextureFormat.ARGB32,RenderTextureReadWrite.Linear);
                imageViews[i].filterMode=FilterMode.Point;imageViews[i].Create();
            }
            for(int i=0;i<4;i++) {
                if(phaseDrives[i]!=null && phaseDrives[i].width==s.pixelsX && phaseDrives[i].height==s.pixelsY) continue;
                if(phaseDrives[i]!=null) {phaseDrives[i].Release();Destroy(phaseDrives[i]);}
                phaseDrives[i]=new RenderTexture(s.pixelsX,s.pixelsY,0,RenderTextureFormat.ARGBHalf,RenderTextureReadWrite.Linear);
                phaseDrives[i].filterMode=FilterMode.Point;phaseDrives[i].Create();driveCacheKey="";
            }
        }
        void RenderImages(OpticalSettings s,int framePhase)
        {
            s.Validate();EnsureImageTargets(s);
            var targets=Allocation();
            eyePreview.SetVector("_EyeL",EyeVector(s.Eye(false)));eyePreview.SetVector("_EyeR",EyeVector(s.Eye(true)));
            eyePreview.SetVector("_TargetL",EyeVector(targets.Eye(false)));eyePreview.SetVector("_TargetR",EyeVector(targets.Eye(true)));
            eyePreview.SetVector("_Panel",new Vector4((float)s.panelWidth,(float)s.panelHeight,s.pixelsX,s.pixelsY));
            eyePreview.SetFloat("_Gap",(float)s.Gap);eyePreview.SetFloat("_Period",(float)s.sourcePeriod);
            eyePreview.SetFloat("_Width",(float)s.sourceWidth);eyePreview.SetFloat("_Step",(float)s.rawPhaseStep);
            eyePreview.SetFloat("_Offset",(float)s.sourceOffset);eyePreview.SetFloat("_Slope",(float)s.slope);
            eyePreview.SetFloat("_Direction",s.phaseDirection);
            eyePreview.SetTexture("_LeftImage",leftImage!=null?leftImage:Texture2D.whiteTexture);
            eyePreview.SetTexture("_RightImage",rightImage!=null?rightImage:Texture2D.whiteTexture);
            eyePreview.SetFloat("_HasLeft",leftImage!=null?1:0);eyePreview.SetFloat("_HasRight",rightImage!=null?1:0);
            eyePreview.SetFloat("_Pattern",imagePattern);eyePreview.SetFloat("_Conflict",conflictPolicy);
            eyePreview.SetFloat("_Cycle",integrateCycle?1:0);eyePreview.SetFloat("_Gain",imageGain);
            string key=JsonUtility.ToJson(targets)+"/"+imagePattern+"/"+conflictPolicy+"/"
                +(leftImage!=null?leftImage.GetInstanceID():0)+"/"+(rightImage!=null?rightImage.GetInstanceID():0);
            if(key!=driveCacheKey) {
                // Unbind old SRVs before writing these same buffers. The drive pass
                // does not sample them, but D3D resource hazards must still be avoided.
                for(int k=0;k<4;k++) eyePreview.SetTexture("_Drive"+k,Texture2D.blackTexture);
                eyePreview.SetFloat("_Output",2);
                for(int k=0;k<4;k++) {
                    eyePreview.SetFloat("_Phase",k);
                    Graphics.Blit(Texture2D.whiteTexture,phaseDrives[k],eyePreview);
                }
                driveCacheKey=key;
            }
            for(int k=0;k<4;k++) eyePreview.SetTexture("_Drive"+k,phaseDrives[k]);
            eyePreview.SetFloat("_Phase",framePhase);
            for(int i=0;i<5;i++) {eyePreview.SetFloat("_Output",i==2?5:i);Graphics.Blit(Texture2D.whiteTexture,imageViews[i],eyePreview);}
            imagePhase=framePhase;imageCyclePresented=integrateCycle;imageGainPresented=imageGain;
        }
        void ImageControls()
        {
            GUILayout.Label("IMAGE TEST",heading);
            GUILayout.Label("Input pair / diagnostic pattern",small);
            imagePattern=GUILayout.SelectionGrid(imagePattern,new[]{"Images / L-R targets","Left white / Right black","Left black / Right white","Both white"},1);
            GUILayout.Label("PNG / JPG paths (or Inspector texture slots)",small);
            leftPath=GUILayout.TextField(leftPath);rightPath=GUILayout.TextField(rightPath);
            if(GUILayout.Button("Load image pair from paths")) LoadPair();
#if UNITY_EDITOR
            GUILayout.BeginHorizontal();
            if(GUILayout.Button("Choose left...")) ChooseImage(false);
            if(GUILayout.Button("Choose right...")) ChooseImage(true);
            GUILayout.EndHorizontal();
#endif
            if(GUILayout.Button("Use built-in L / R targets")) {leftImage=rightImage=null;imagePattern=0;}
            GUILayout.Label("When both eyes see the same subpixel",small);
            conflictPolicy=GUILayout.SelectionGrid(conflictPolicy,new[]{"Black (avoid mixing)","Mix L + R (show crosstalk)","Left priority (diagnostic)"},1);
            integrateCycle=GUILayout.Toggle(integrateCycle,"Average all 4 phases (static eyes / images)");
            if(GUILayout.Button("Export synchronized phases 0-3")) ExportPhaseSequence();
            GUILayout.Label("Received-image brightness gain: "+imageGain.ToString("F1"),small);
            imageGain=GUILayout.HorizontalSlider(imageGain,1,8);
            GUILayout.Label("Fixed gain; no automatic correction of dark/missing areas.",small);
            GUILayout.Space(8);
        }
#if UNITY_EDITOR
        void ChooseImage(bool right)
        {
            string path=UnityEditor.EditorUtility.OpenFilePanel(right?"Right-eye image":"Left-eye image","","");
            if(string.IsNullOrEmpty(path)) return;
            try {SetLoadedImage(path,right);if(right)rightPath=path;else leftPath=path;imagePattern=0;}
            catch(Exception e) {status="Image load failed: "+e.Message;}
        }
#endif
        Texture2D ReadImage(string path)
        {
            if(string.IsNullOrWhiteSpace(path)) throw new ArgumentException("Choose both image paths.");
            var texture=new Texture2D(2,2,TextureFormat.RGB24,false);
            try {
                if(!texture.LoadImage(File.ReadAllBytes(path.Trim().Trim('"')))) throw new ArgumentException("Use a PNG or JPG image.");
                texture.name=Path.GetFileName(path);texture.wrapMode=TextureWrapMode.Clamp;texture.filterMode=FilterMode.Point;
                return texture;
            } catch {Destroy(texture);throw;}
        }
        void SetLoadedImage(string path,bool right)
        {
            var t=ReadImage(path);
            if(right) {if(ownedRight!=null)Destroy(ownedRight);ownedRight=t;rightImage=t;}
            else {if(ownedLeft!=null)Destroy(ownedLeft);ownedLeft=t;leftImage=t;}
            status="Loaded "+t.name+" ("+t.width+" x "+t.height+"). Mapped to the full LCD.";
        }
        void LoadPair()
        {
            Texture2D l=null,r=null;
            try {
                l=ReadImage(leftPath);r=ReadImage(rightPath);
                if(ownedLeft!=null)Destroy(ownedLeft);if(ownedRight!=null)Destroy(ownedRight);
                ownedLeft=l;ownedRight=r;leftImage=l;rightImage=r;imagePattern=0;
                status="Image pair loaded. Different input sizes are resampled to the LCD grid.";
            } catch(Exception e) {if(l!=null)Destroy(l);if(r!=null)Destroy(r);status="Image load failed: "+e.Message;}
        }
        void PreviewTile(Rect rect,RenderTexture texture,string label)
        {
            GUI.Label(new Rect(rect.x,rect.y-25,rect.width,24),label,heading);
            GUI.Box(rect,"");if(texture!=null)GUI.DrawTexture(rect,texture,ScaleMode.ScaleToFit,false);
        }
        void DrawImagePanels()
        {
            PreviewTile(new Rect(390,157,500,281.25f),imageViews[3],"INPUT - LEFT");
            PreviewTile(new Rect(914,157,500,281.25f),imageViews[4],"INPUT - RIGHT");
            string sample=imageCyclePresented?" / 4-PHASE MEAN":" / PHASE "+imagePhase;
            PreviewTile(new Rect(390,475,500,281.25f),imageViews[0],"RECEIVED - LEFT"+sample);
            PreviewTile(new Rect(914,475,500,281.25f),imageViews[1],"RECEIVED - RIGHT"+sample);
            PreviewTile(new Rect(390,790,276,155.25f),imageViews[2],"LCD DRIVE - PHASE "+imagePhase);
            GUILayout.BeginArea(new Rect(690,785,722,165),GUI.skin.box);
            GUILayout.Label("SYNCHRONIZED: light "+imagePhase+" = LCD "+imagePhase+"   |   Gain x"+imageGainPresented.ToString("F1"),heading);
            GUILayout.Label(tracking?"Allocation follows eyes":"Allocation frozen: move eyes to test loss of alignment",small);
            GUILayout.Label("Received images are shown in LCD coordinates (front-rectified), not perspective camera images. All RGB centres are computed at the native LCD resolution. On-screen thumbnails are reduced; exported PNGs preserve full resolution.\nOverlap is one shared LCD value: black / mixed / left priority. Dark areas are not filled in. This is a geometric simulation, not a measured optical image.",small);
            GUILayout.Label(status,small);
            GUILayout.EndArea();
        }
        void SaveRenderTexture(RenderTexture rt,string path)
        {
            var old=RenderTexture.active;Texture2D tex=null;
            try {RenderTexture.active=rt;tex=new Texture2D(rt.width,rt.height,TextureFormat.RGB24,false);tex.ReadPixels(new Rect(0,0,rt.width,rt.height),0,0);tex.Apply();File.WriteAllBytes(path,tex.EncodeToPNG());}
            finally {RenderTexture.active=old;if(tex!=null)Destroy(tex);}
        }
        void ExportEyeViews(string prefix)
        {
            string[] names={"received-left","received-right","lcd-phase-"+imagePhase,"input-left","input-right"};
            for(int i=0;i<5;i++)SaveRenderTexture(imageViews[i],prefix+"-"+names[i]+".png");
            File.WriteAllText(prefix+"-allocation-settings.json",JsonUtility.ToJson(Allocation(),true));
            File.WriteAllText(prefix+"-image-test.txt","pattern="+imagePattern+"\nconflictPolicy="+conflictPolicy+" (0 black, 1 mix, 2 left priority)\ncycleMean="+integrateCycle+"\nfixedGain="+imageGain+"\ntracking="+tracking+"\nphase="+phase+"\nInputs are saved as PNG; received views use LCD coordinates, not perspective projection.\n");
        }
        void ExportPhaseSequence()
        {
            int savedPhase=phase;bool savedCycle=integrateCycle;
            try {
                Current().Validate();
                Directory.CreateDirectory(OutputDirectory);
                string prefix=Path.Combine(OutputDirectory,DateTime.Now.ToString("yyyyMMdd-HHmmss")+"-sequence");
                // No time/pose advancement: compare all four phases at one fixed pose.
                integrateCycle=false;
                for(int k=0;k<4;k++) {phase=k;DrawPreviews(true);ExportEyeViews(prefix+"-phase"+k);}
                integrateCycle=true;phase=0;DrawPreviews(true);ExportEyeViews(prefix+"-mean");
                File.WriteAllText(prefix+"-observer-settings.json",JsonUtility.ToJson(Current(),true));
                status="Exported synchronized phase sequence: "+OutputDirectory;
            } catch(Exception e) {status="Sequence export failed: "+e.Message;}
            finally {
                phase=savedPhase;integrateCycle=savedCycle;
                try {DrawPreviews(true);} catch(Exception e) {status="Preview restore failed: "+e.Message;}
            }
        }
        void DestroyImages()
        {
            Destroy(eyePreview);Destroy(ownedLeft);Destroy(ownedRight);
            if(imageViews!=null)foreach(var rt in imageViews)if(rt!=null){rt.Release();Destroy(rt);}
            if(phaseDrives!=null)foreach(var rt in phaseDrives)if(rt!=null){rt.Release();Destroy(rt);}
        }
    }
}
