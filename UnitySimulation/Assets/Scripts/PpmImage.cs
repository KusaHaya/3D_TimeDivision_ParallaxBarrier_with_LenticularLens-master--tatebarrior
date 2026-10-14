using System;
using System.Globalization;
using System.Text;

namespace ParallaxSimulation
{
    public static class PpmImage
    {
        static bool Space(byte b) {return b==32 || b==9 || b==10 || b==13;}
        static string Token(byte[] data,ref int p)
        {
            while(p<data.Length) {
                if(Space(data[p])) {p++;continue;}
                if(data[p]==35) {while(p<data.Length && data[p]!=10)p++;continue;}
                break;
            }
            int start=p;while(p<data.Length && !Space(data[p]) && data[p]!=35)p++;
            if(p==start)throw new FormatException("Incomplete PPM header.");
            return Encoding.ASCII.GetString(data,start,p-start);
        }
        // P6 RGB input, top row first. Unity LoadRawTextureData expects the bottom row first.
        public static byte[] Decode(byte[] data,out int width,out int height)
        {
            int p=0;if(Token(data,ref p)!="P6")throw new FormatException("Only binary P6 PPM is supported.");
            width=int.Parse(Token(data,ref p),CultureInfo.InvariantCulture);height=int.Parse(Token(data,ref p),CultureInfo.InvariantCulture);
            int max=int.Parse(Token(data,ref p),CultureInfo.InvariantCulture);
            if(width<1 || height<1 || width>16384 || height>16384 || max!=255)throw new FormatException("PPM must use 8-bit RGB (maxval 255).");
            if(p>=data.Length || !Space(data[p]))throw new FormatException("Missing PPM raster delimiter.");
            if(data[p++]==13 && p<data.Length && data[p]==10)p++;
            int size=checked(width*height*3);
            if(data.Length-p!=size)throw new FormatException("PPM raster size does not match its dimensions.");
            var rgb=new byte[size];int row=width*3;
            for(int y=0;y<height;y++)Buffer.BlockCopy(data,p+y*row,rgb,(height-1-y)*row,row);
            return rgb;
        }
    }
}
