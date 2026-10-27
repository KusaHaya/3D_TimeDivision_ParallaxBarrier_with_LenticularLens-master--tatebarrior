using System;
using System.IO;
using UnityEngine;

namespace ParallaxSimulation
{
    public sealed partial class SimulationController
    {
        Material muraMaterial;
        RenderTexture[] muraViews;
        int muraObserver = 1;
        float muraPupilDiameter = 4.0f;
        float muraGain = 1.0f;
        bool muraCycle;
        int muraPhase = -1;

        void InitMura()
        {
            var shader = Resources.Load<Shader>("BacklightMura");
            if(shader != null) muraMaterial = new Material(shader);
            muraViews = new RenderTexture[3];
        }

        void EnsureMuraTargets()
        {
            for(int i = 0; i < muraViews.Length; i++)
            {
                if(muraViews[i] != null) continue;
                muraViews[i] = new RenderTexture(1024, 576, 0,
                    RenderTextureFormat.ARGB32, RenderTextureReadWrite.Linear);
                muraViews[i].filterMode = FilterMode.Bilinear;
                muraViews[i].Create();
            }
        }

        void RenderMura(OpticalSettings s, int framePhase)
        {
            if(muraMaterial == null)
            {
                status = "BacklightMura shader could not be loaded.";
                return;
            }
            EnsureMuraTargets();
            SetLensUniforms(muraMaterial, s);
            muraMaterial.SetVector("_EyeL", EyeVector(s.Eye(false)));
            muraMaterial.SetVector("_EyeR", EyeVector(s.Eye(true)));
            muraMaterial.SetVector("_Panel", new Vector4((float)s.panelWidth,
                (float)s.panelHeight, s.pixelsX, s.pixelsY));
            muraMaterial.SetVector("_Region", new Vector4(mapX, mapY, zoomWidth,
                zoomWidth * 576f / 1024f));
            muraMaterial.SetFloat("_Gap", (float)s.Gap);
            muraMaterial.SetFloat("_Period", (float)s.sourcePeriod);
            muraMaterial.SetFloat("_Width", (float)s.sourceWidth);
            muraMaterial.SetFloat("_Step", (float)s.rawPhaseStep);
            muraMaterial.SetFloat("_Offset", (float)s.sourceOffset);
            muraMaterial.SetFloat("_Slope", (float)s.slope);
            muraMaterial.SetFloat("_Direction", s.phaseDirection);
            muraMaterial.SetFloat("_Phase", framePhase);
            muraMaterial.SetFloat("_Observer", muraObserver);
            muraMaterial.SetFloat("_PupilDiameter", muraPupilDiameter);
            muraMaterial.SetFloat("_Cycle", muraCycle ? 1 : 0);
            muraMaterial.SetFloat("_Gain", muraGain);
            for(int i = 0; i < muraViews.Length; i++)
            {
                muraMaterial.SetFloat("_Output", i);
                Graphics.Blit(Texture2D.whiteTexture, muraViews[i], muraMaterial);
            }
            muraPhase = framePhase;
        }

        void MuraControls()
        {
            GUILayout.Label("BACKLIGHT MURA", heading);
            GUILayout.Label("Reverse rays from the selected observer. Direct view checks whether each ray reaches an active physical LED. Diffuser view checks the focused bright band on the image plane.", small);
            GUILayout.Space(8);
            HardwareControls();
            GUILayout.Label("1  OBSERVER", heading);
            bool next = GUILayout.Toggle(tracking, "Tracking ON [h]");
            if(next != tracking) ToggleTracking();
            Slider("Head X (mm)", ref settings.headX, -250, 250, "F1");
            Slider("Head Y (mm)", ref settings.headY, -150, 150, "F1");
            Slider("Eye distance from LCD (mm)", ref settings.headZ, 150, 1500, "F1");
            Slider("Eye separation (mm)", ref settings.ipd, 40, 80, "F1");
            Slider("Head yaw (deg)", ref settings.yawDegrees, -40, 40, "F1");
            muraObserver = GUILayout.SelectionGrid(muraObserver,
                new[]{"Left eye", "Head centre", "Right eye"}, 1);
            GUILayout.Label("Pupil diameter: " + muraPupilDiameter.ToString("F1") + " mm", small);
            muraPupilDiameter = GUILayout.HorizontalSlider(muraPupilDiameter, 0.1f, 8.0f);
            GUILayout.Label("Nine pupil samples turn binary LED hits into relative brightness.", small);
            GUILayout.Space(8);
            GUILayout.Label("2  OPTICS", heading);
            LensOpticsControls();
            GUILayout.Label("Image plane from vertex: " + settings.lens.ImageDistance.ToString("F4") + " mm", small);
            GUILayout.Label("Image plane -> LCD: " + (settings.lens.screenDistance-settings.lens.ImageDistance).ToString("F4") + " mm", small);
            GUILayout.Space(8);
            GUILayout.Label("3  PHASE", heading);
            phase = GUILayout.Toolbar(phase, new[]{"0", "1", "2", "3"});
            animate = GUILayout.Toggle(animate, "Animate phases slowly");
            muraCycle = GUILayout.Toggle(muraCycle, "Average all four phases");
            GUILayout.Label("Brightness gain: " + muraGain.ToString("F1"), small);
            muraGain = GUILayout.HorizontalSlider(muraGain, 0.25f, 4.0f);
            GUILayout.Label("Preview width: " + zoomWidth.ToString("F1") + " mm", small);
            zoomWidth = GUILayout.HorizontalSlider(zoomWidth, 4, 528);
            GUILayout.BeginHorizontal();
            if(GUILayout.Button("Detail 24 mm")) zoomWidth = 24;
            if(GUILayout.Button("Full LCD")) zoomWidth = 528;
            GUILayout.EndHorizontal();
            GUILayout.Label("Map centre X / Y (mm)", small);
            mapX = GUILayout.HorizontalSlider(mapX, -250, 250);
            mapY = GUILayout.HorizontalSlider(mapY, -140, 140);
            GUILayout.Label("Use detail view to inspect sub-millimetre bands; full LCD can alias in the reduced preview.", small);
            if(GUILayout.Button("Export current mura comparison")) ExportMuraViews();
            GUILayout.Space(8);
            GUILayout.Label("WHITE: the pupil receives light\nBLACK: no active LED / bright image band\nGRAY: only part of the pupil receives light", small);
            GUILayout.Label("The diffuser comparison is an ideal thin screen placed at the calculated image plane. Scattering angle, sheet thickness and measured transmission are not yet included.", small);
            GUILayout.Label(status, small);
        }

        void MuraTile(Rect rect, RenderTexture texture, string label)
        {
            GUI.Label(new Rect(rect.x, rect.y - 25, rect.width, 24), label, heading);
            GUI.Box(rect, "");
            if(texture != null) GUI.DrawTexture(rect, texture, ScaleMode.ScaleToFit, false);
        }

        void DrawMuraPanels()
        {
            string observer = muraObserver == 0 ? "LEFT EYE" : muraObserver == 2 ? "RIGHT EYE" : "HEAD CENTRE";
            string suffix = muraCycle ? " / FOUR-PHASE MEAN" : " / PHASE " + muraPhase;
            MuraTile(new Rect(390, 157, 500, 281.25f), muraViews == null ? null : muraViews[0],
                "DIRECT LED THROUGH LENS - " + observer + suffix);
            MuraTile(new Rect(914, 157, 500, 281.25f), muraViews == null ? null : muraViews[1],
                "IMAGE-PLANE DIFFUSER - " + observer + suffix);
            MuraTile(new Rect(390, 485, 500, 281.25f), muraViews == null ? null : muraViews[2],
                "DIFFERENCE  /  ORANGE=DIRECT, CYAN=DIFFUSER");
            GUILayout.BeginArea(new Rect(914, 475, 500, 291), GUI.skin.box);
            GUILayout.Label("WHAT THIS CHECKS", heading);
            GUILayout.Label("DIRECT traces each observer ray through the convex vertex, PMMA and flat face to the physical LED plane. Brightness is the fraction of nine pupil samples that land on an active LED.", small);
            GUILayout.Space(8);
            GUILayout.Label("DIFFUSER intersects the same observer ray with the calculated sharp image plane. It therefore shows the luminance plane that would hide direct views of individual LEDs while retaining the oblique phase bands.", small);
            GUILayout.Space(8);
            GUILayout.Label("Under an ideal lossless lens these two maps can match exactly. That means LED-present / LED-absent geometry alone predicts the mura, but not its reduction. A real improvement requires angular redistribution, lens-edge loss, aberration or LED-radiance variation not present in the ideal model.", small);
            GUILayout.Space(8);
            GUILayout.Label("Relative geometric brightness only: uniform LED radiance; no measured diffuser BSDF, Fresnel loss, LCD transmission, diffraction or lens aberration.", small);
            GUILayout.EndArea();
            GUI.Label(new Rect(390, 790, 1020, 60),
                "Comparison uses the same observer, pupil, phase and optical dimensions. The diffuser is placed at " +
                settings.lens.ImageDistance.ToString("F4") + " mm from the convex vertex; LCD is at " +
                settings.lens.screenDistance.ToString("F4") + " mm.", small);
        }

        void ExportMuraViews()
        {
            try
            {
                var s = Current();
                s.Validate();
                RenderMura(s, ((phase % 4) + 4) % 4);
                Directory.CreateDirectory(OutputDirectory);
                string prefix = Path.Combine(OutputDirectory,
                    DateTime.Now.ToString("yyyyMMdd-HHmmss") + "-backlight-mura");
                SaveRenderTexture(muraViews[0], prefix + "-direct-led.png");
                SaveRenderTexture(muraViews[1], prefix + "-image-plane-diffuser.png");
                SaveRenderTexture(muraViews[2], prefix + "-difference.png");
                File.WriteAllText(prefix + "-settings.json", JsonUtility.ToJson(s, true));
                File.WriteAllText(prefix + "-view.txt",
                    "observer=" + muraObserver + " (0 left, 1 centre, 2 right)\n" +
                    "pupilDiameterMm=" + muraPupilDiameter + "\n" +
                    "phase=" + muraPhase + "\ncycleMean=" + muraCycle + "\n" +
                    "gain=" + muraGain + "\nregionCentreMm=" + mapX + "," + mapY +
                    "\nregionWidthMm=" + zoomWidth + "\n");
                status = "Exported backlight mura comparison: " + OutputDirectory;
            }
            catch(Exception e) { status = "Mura export failed: " + e.Message; }
        }
        void DestroyMura()
        {
            if(muraMaterial != null) Destroy(muraMaterial);
            if(muraViews == null) return;
            foreach(var rt in muraViews) if(rt != null) { rt.Release(); Destroy(rt); }
        }
    }
}