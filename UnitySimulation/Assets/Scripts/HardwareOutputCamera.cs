using UnityEngine;

namespace ParallaxSimulation
{
    // Built-in pipeline: bypass all preview layouts and copy one LCD texel to one pixel.
    public sealed class HardwareOutputCamera : MonoBehaviour
    {
        public bool active;
        public RenderTexture output;
        public int renderedFrame=-1;
        void OnRenderImage(RenderTexture source,RenderTexture destination)
        {
            if(!active) { Graphics.Blit(source,destination);return; }
            bool valid=Screen.width==1920 && Screen.height==1080 && Screen.fullScreen &&
                output!=null && output.width==1920 && output.height==1080;
            Graphics.Blit(valid?(Texture)output:Texture2D.blackTexture,destination);
            renderedFrame=valid?Time.frameCount:-1;
        }
    }
}
